#!/usr/bin/env python3
"""bhpix — BLOODHOLLOW sprite cleanup + packing primitives (bible §14, §5).

Pure Pillow + numpy (no new C++ deps; python tooling is already the repo's
mapgen convention). Every step is deterministic: same input -> same bytes.

Pipeline (one raw AI plate -> engine-ready cell):
  1. key_out_green()   chroma-key #00FF00 plates to straight alpha, despill
  2. crop_to_alpha()   tight bbox
  3. fit_to_cell()     nearest-neighbour downscale so the sprite occupies the
                       target height (feet on the anchor row), no smoothing
  4. quantize()        <=32 colours (median cut, per family ramp) + ordered
                       2x2/4x4 Bayer dither (era checkerboard, not diffusion)
  5. outline()         1px hard outline in the #1a1214 family on alpha edge
  6. place_in_cell()   32x48 (or boss) cell, feet at y=42, painted soft
                       contact ellipse like engine/assets/placeholder.cpp
  7. pack_atlas()      uniform grid PNG rows=8 dirs, cols=frames + JSON v1
                       sidecar in the exact schema engine/assets/atlas.cpp reads

Also: night_floor() applies the engine's nightOverlay(hour) blend for the QA
screenshot (render/daynight.cpp keys), palette_strip() writes the QA strip.
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Iterable

import numpy as np
from PIL import Image

OUTLINE_RGB = (0x1A, 0x12, 0x14)  # bible §5 "~#1a1214 family"
CELL_W, CELL_H, FEET_Y = 32, 48, 42  # engine/assets/placeholder.cpp
DIR_ORDER = ["E", "SE", "S", "SW", "W", "NW", "N", "NE"]  # sim::kDx/kDy

# 4x4 Bayer matrix, normalised to [-0.5, 0.5)
_BAYER4 = (np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]],
                    dtype=np.float32) / 16.0) - 0.5
_BAYER2 = (np.array([[0, 2], [3, 1]], dtype=np.float32) / 4.0) - 0.5


# --------------------------------------------------------------------------
# 1. chroma key
# --------------------------------------------------------------------------
def key_out_green(img: Image.Image, *, tol: float = 0.55, despill: bool = True) -> Image.Image:
    """Key a flat #00FF00 plate to straight alpha.

    Works in a green-dominance space so anti-aliased fringe pixels get a
    partial alpha (which we later harden), and despills the green cast that
    bleeds into outlines — the classic AI-plate defect.
    """
    a = np.asarray(img.convert("RGB")).astype(np.float32) / 255.0
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    dom = g - np.maximum(r, b)  # 1.0 on pure key, <=0 on non-green
    alpha = np.clip(1.0 - (dom / tol), 0.0, 1.0)
    # anything that is plainly the key colour goes fully clear
    alpha[(g > 0.85) & (r < 0.35) & (b < 0.35)] = 0.0
    if despill:
        spill = np.clip(g - (r + b) * 0.5, 0.0, 1.0)
        g = g - spill * 0.9
    rgb = np.stack([r, g, b], axis=-1)
    out = np.dstack([np.clip(rgb, 0, 1), alpha])
    return Image.fromarray((out * 255.0 + 0.5).astype(np.uint8), "RGBA")


def harden_alpha(img: Image.Image, cut: int = 128) -> Image.Image:
    """Straight alpha, no premultiplied halos: alpha is 0 or 255 (bible §5)."""
    a = np.asarray(img.convert("RGBA")).copy()
    a[..., 3] = np.where(a[..., 3] >= cut, 255, 0).astype(np.uint8)
    a[a[..., 3] == 0] = 0
    return Image.fromarray(a, "RGBA")


def crop_to_alpha(img: Image.Image, pad: int = 0) -> Image.Image:
    bbox = img.getchannel("A").getbbox()
    if bbox is None:
        return img
    l, t, r, b = bbox
    return img.crop((max(0, l - pad), max(0, t - pad), min(img.width, r + pad), min(img.height, b + pad)))


# --------------------------------------------------------------------------
# 3. nearest downscale
# --------------------------------------------------------------------------
def fit_to_cell(img: Image.Image, *, target_h: int | None = None, target_w: int | None = None) -> Image.Image:
    """Nearest-neighbour scale so height==target_h (or width==target_w).

    Bible §14.1: generate at 4x, downscale NEAREST. We do a box-filter
    pre-pass only when the ratio is > 3x to avoid dropping 1px features (claws,
    teeth), then finish with NEAREST so edges stay hard.
    """
    w, h = img.size
    if target_h is not None:
        s = target_h / h
    elif target_w is not None:
        s = target_w / w
    else:
        raise ValueError("target_h or target_w required")
    nw, nh = max(1, round(w * s)), max(1, round(h * s))
    src = img
    if s < 1 / 3:
        # gentle pre-reduce (area) to 2x target, keeps thin features from aliasing out
        pre = (max(nw * 2, 1), max(nh * 2, 1))
        src = img.resize(pre, Image.BOX)
    return src.resize((nw, nh), Image.NEAREST)


# --------------------------------------------------------------------------
# 4. quantize + ordered dither
# --------------------------------------------------------------------------
def build_palette(imgs: Iterable[Image.Image], n: int = 32, reserve_outline: bool = True) -> np.ndarray:
    """Median-cut a shared ramp across a *family* of images (bible §4.2:
    <=32 colours per sprite-sheet / tileset family). Returns (k,3) uint8."""
    px = []
    for im in imgs:
        a = np.asarray(im.convert("RGBA"))
        m = a[..., 3] > 0
        px.append(a[m][:, :3])
    allpx = np.concatenate(px, axis=0) if px else np.zeros((1, 3), np.uint8)
    k = n - 1 if reserve_outline else n
    tmp = Image.fromarray(allpx.reshape(1, -1, 3), "RGB")
    q = tmp.quantize(colors=k, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = np.array(q.getpalette()[: k * 3], dtype=np.uint8).reshape(-1, 3)
    if reserve_outline:
        pal = np.vstack([pal, np.array(OUTLINE_RGB, np.uint8)])
    # dedupe while preserving order
    seen, out = set(), []
    for c in map(tuple, pal):
        if c not in seen:
            seen.add(c)
            out.append(c)
    return np.array(out, dtype=np.uint8)


def quantize(img: Image.Image, palette: np.ndarray, *, dither: str = "bayer4", strength: float = 0.12) -> Image.Image:
    """Map to `palette` with ordered (Bayer) dither. Ordered dither gives the
    era 2x2 checkerboard look; error-diffusion (Floyd–Steinberg) reads
    'modern indie' and is rejected by §4.2."""
    a = np.asarray(img.convert("RGBA")).astype(np.float32)
    rgb, alpha = a[..., :3], a[..., 3]
    h, w = alpha.shape
    if dither == "bayer4":
        m = np.tile(_BAYER4, (math.ceil(h / 4), math.ceil(w / 4)))[:h, :w]
    elif dither == "bayer2":
        m = np.tile(_BAYER2, (math.ceil(h / 2), math.ceil(w / 2)))[:h, :w]
    else:
        m = np.zeros((h, w), np.float32)
    rgb = np.clip(rgb + m[..., None] * (255.0 * strength), 0, 255)
    pal = palette.astype(np.float32)
    # nearest palette entry (perceptual-ish weights)
    wgt = np.array([0.30, 0.59, 0.11], np.float32) * 3.0
    d = ((rgb[..., None, :] - pal[None, None, :, :]) ** 2 * wgt).sum(-1)
    idx = d.argmin(-1)
    out = pal[idx].astype(np.uint8)
    res = np.dstack([out, alpha.astype(np.uint8)])
    res[alpha == 0] = 0
    return Image.fromarray(res, "RGBA")


# --------------------------------------------------------------------------
# 5. outline
# --------------------------------------------------------------------------
def outline(img: Image.Image, rgb=OUTLINE_RGB, *, inside: bool = True) -> Image.Image:
    """1px hard outline on the alpha edge. inside=True recolours the outermost
    opaque ring (keeps the cell footprint); inside=False grows by one pixel."""
    a = np.asarray(img.convert("RGBA")).copy()
    al = a[..., 3] > 0
    pad = np.pad(al, 1)
    nb = (pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    if inside:
        edge = al & ~nb
        a[edge, :3] = rgb
    else:
        grow = (pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:])
        edge = grow & ~al
        a[edge, :3] = rgb
        a[edge, 3] = 255
    return Image.fromarray(a, "RGBA")


# --------------------------------------------------------------------------
# 6. cell placement (feet anchor + contact shadow)
# --------------------------------------------------------------------------
def _draw_ellipse(arr: np.ndarray, cx: int, cy: int, rx: int, ry: int, rgba):
    h, w = arr.shape[:2]
    for y in range(-ry, ry + 1):
        yn = y / ry
        half = int(math.sqrt(max(0.0, 1.0 - yn * yn)) * rx)
        yy = cy + y
        if 0 <= yy < h:
            x0, x1 = max(0, cx - half), min(w - 1, cx + half)
            arr[yy, x0:x1 + 1] = rgba


def place_in_cell(sprite: Image.Image, *, cell=(CELL_W, CELL_H), feet_y: int = FEET_Y,
                  shadow: bool = True, shadow_rx: int = 9, shadow_ry: int = 3,
                  foot_inset: int = 0) -> Image.Image:
    """Centre horizontally, put the sprite's bottom row on feet_y (minus
    foot_inset for sprites whose lowest pixels are a tail/dangling cloth), paint
    the placeholder.cpp-style contact shadow underneath (alpha 70)."""
    cw, ch = cell
    canvas = np.zeros((ch, cw, 4), np.uint8)
    if shadow:
        _draw_ellipse(canvas, cw // 2, feet_y + 3, shadow_rx, shadow_ry, (0, 0, 0, 70))
    s = np.asarray(sprite.convert("RGBA"))
    sh, sw = s.shape[:2]
    x0 = (cw - sw) // 2
    y0 = feet_y - sh + 1 + foot_inset
    # clip
    sx0, sy0 = max(0, -x0), max(0, -y0)
    dx0, dy0 = max(0, x0), max(0, y0)
    wcp = min(sw - sx0, cw - dx0)
    hcp = min(sh - sy0, ch - dy0)
    if wcp > 0 and hcp > 0:
        src = s[sy0:sy0 + hcp, sx0:sx0 + wcp]
        dst = canvas[dy0:dy0 + hcp, dx0:dx0 + wcp]
        m = src[..., 3] > 0
        dst[m] = src[m]
    return Image.fromarray(canvas, "RGBA")


# --------------------------------------------------------------------------
# 7. atlas packing (schema v1 as read by engine/assets/atlas.cpp)
# --------------------------------------------------------------------------
def pack_atlas(anims: dict[str, dict], out_png: Path, out_json: Path, *, cell=(CELL_W, CELL_H)) -> dict:
    """anims = {"walk": {"frames": [[dir0_f0, dir0_f1..], ... 8 lists], "fps": 10}}
    Blocks are laid out left-to-right per anim; each block is 8 rows x N cols.
    Rows = direction in sim::kDx order: E, SE, S, SW, W, NW, N, NE. Never mirror."""
    cw, ch = cell
    total_cols = sum(len(a["frames"][0]) for a in anims.values())
    sheet = Image.new("RGBA", (cw * total_cols, ch * 8), (0, 0, 0, 0))
    meta = {"anims": {}}
    ox = 0
    for name, a in anims.items():
        frames = a["frames"]
        assert len(frames) == 8, f"{name}: need 8 direction rows, got {len(frames)}"
        n = len(frames[0])
        for d in range(8):
            assert len(frames[d]) == n, f"{name}: ragged frame count in dir {d}"
            for f in range(n):
                im = frames[d][f]
                assert im.size == (cw, ch), f"{name}[{d}][{f}] is {im.size}, cell is {cell}"
                sheet.paste(im, (ox + f * cw, d * ch))
        meta["anims"][name] = {"frameW": cw, "frameH": ch, "frames": n, "dirs": 8,
                               "fps": a.get("fps", 8), "offsetX": ox, "offsetY": 0}
        ox += n * cw
    out_png.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out_png, optimize=True)
    out_json.write_text(json.dumps(meta, indent=1) + "\n")
    return meta


def validate_atlas(png: Path, js: Path) -> list[str]:
    """Mirror of the checks in bh::loadAtlas so CI can lint without raylib."""
    errs = []
    im = Image.open(png)
    j = json.loads(Path(js).read_text())
    if "anims" not in j:
        return ["missing 'anims'"]
    for name, a in j["anims"].items():
        for k in ("frameW", "frameH", "frames"):
            if k not in a:
                errs.append(f"{name}: missing {k}")
        if errs:
            continue
        fw, fh, n = a["frameW"], a["frameH"], a["frames"]
        dirs, ox, oy = a.get("dirs", 8), a.get("offsetX", 0), a.get("offsetY", 0)
        if dirs != 8 or n <= 0 or fw <= 0 or fh <= 0:
            errs.append(f"{name}: dirs must be 8 and dims positive")
        if ox + n * fw > im.width or oy + 8 * fh > im.height:
            errs.append(f"{name}: block exceeds texture ({im.width}x{im.height})")
    return errs


# --------------------------------------------------------------------------
# QA helpers
# --------------------------------------------------------------------------
def night_overlay(hour: float) -> tuple[int, int, int, int]:
    """Port of engine/render/daynight.cpp nightOverlay() keyframes."""
    keys = [(0.0, 18, 22, 70, 140), (4.0, 18, 22, 70, 150), (6.0, 80, 60, 60, 60),
            (8.0, 0, 0, 0, 0), (16.0, 0, 0, 0, 0), (18.0, 90, 60, 50, 60),
            (20.0, 18, 22, 70, 140), (24.01, 18, 22, 70, 140)]
    hour %= 24.0
    for (h0, *c0), (h1, *c1) in zip(keys, keys[1:]):
        if h0 <= hour <= h1:
            t = (hour - h0) / (h1 - h0) if h1 > h0 else 0.0
            return tuple(int(a + (b - a) * t) for a, b in zip(c0, c1))  # type: ignore
    return (0, 0, 0, 0)


def night_floor(img: Image.Image, hour: float = 2.0) -> Image.Image:
    """Composite the engine's fullscreen night rectangle over a scene, exactly
    like Game::render() (DrawRectangle with nightOverlay colour, normal blend)."""
    r, g, b, a = night_overlay(hour)
    base = img.convert("RGBA")
    ov = Image.new("RGBA", base.size, (r, g, b, a))
    return Image.alpha_composite(base, ov)


def palette_strip(palette: np.ndarray, out: Path, swatch: int = 8):
    strip = Image.new("RGB", (swatch * len(palette), swatch))
    for i, c in enumerate(palette):
        strip.paste(tuple(int(v) for v in c), (i * swatch, 0, (i + 1) * swatch, swatch))
    out.parent.mkdir(parents=True, exist_ok=True)
    strip.save(out)


def count_colours(img: Image.Image) -> int:
    a = np.asarray(img.convert("RGBA"))
    px = a[a[..., 3] > 0][:, :3]
    return len(np.unique(px, axis=0)) if len(px) else 0


def greyscale(img: Image.Image) -> Image.Image:
    a = np.asarray(img.convert("RGBA")).copy()
    g = (a[..., 0] * 0.3 + a[..., 1] * 0.59 + a[..., 2] * 0.11).astype(np.uint8)
    a[..., 0] = a[..., 1] = a[..., 2] = g
    return Image.fromarray(a, "RGBA")


def upscale(img: Image.Image, k: int) -> Image.Image:
    return img.resize((img.width * k, img.height * k), Image.NEAREST)


# --------------------------------------------------------------------------
# B0.5 additions (docs/art/PARALLEL-ROADMAP.md A12/A13/A16)
# --------------------------------------------------------------------------
def luma_mean(img: Image.Image, *, body_only: bool = True, exclude_outline: bool = True) -> float:
    """Mean Rec.601 luma of opaque pixels. body_only excludes the alpha-70
    contact shadow; exclude_outline drops the #1a1214 contour so R-LUMA measures
    the *body* (the rulebook's definition), not the dark rim."""
    a = np.asarray(img.convert("RGBA")).astype(np.float32)
    m = a[..., 3] == 255 if body_only else a[..., 3] > 0
    if exclude_outline:
        m &= ~((a[..., 0] == OUTLINE_RGB[0]) & (a[..., 1] == OUTLINE_RGB[1]) & (a[..., 2] == OUTLINE_RGB[2]))
    if not m.any():
        return 0.0
    px = a[m]
    return float((px[:, 0] * 0.299 + px[:, 1] * 0.587 + px[:, 2] * 0.114).mean())


def rim_light(cell: Image.Image, rgb=(0xC9, 0xBF, 0xAE), *, side: str = "NW", strength: int = 1) -> Image.Image:
    """R-LUMA fix step 3: a 1px bone rim on the lit edge. Replaces the body
    pixel just *inside* the outline on the lit side (never widens the sprite,
    never touches the outline itself). side = compass of the key light."""
    a = np.asarray(cell.convert("RGBA")).copy()
    h, w = a.shape[:2]
    outline = (a[..., 3] == 255) & (a[..., 0] == OUTLINE_RGB[0]) & (a[..., 1] == OUTLINE_RGB[1]) & (a[..., 2] == OUTLINE_RGB[2])
    body = (a[..., 3] == 255) & ~outline
    dx = {"W": 1, "NW": 1, "N": 0, "NE": -1, "E": -1, "SW": 1, "S": 0, "SE": -1}[side]
    dy = {"W": 0, "NW": 1, "N": 1, "NE": 1, "E": 0, "SW": -1, "S": -1, "SE": -1}[side]
    lit = np.zeros_like(body)
    # a body pixel is 'lit' if stepping toward the light lands on outline
    for k in range(1, strength + 1):
        src_y = np.clip(np.arange(h)[:, None] - dy * k, 0, h - 1)
        src_x = np.clip(np.arange(w)[None, :] - dx * k, 0, w - 1)
        lit |= body & outline[src_y, src_x]
    a[lit, :3] = rgb
    return Image.fromarray(a, "RGBA")


def diamond_mask(w: int = 64, h: int = 32) -> Image.Image:
    """Seamless 2:1 diamond: pixel-centre test |dx|/(w/2) + |dy|/(h/2) < 1 with
    a half-open rule so diamonds laid at (±w/2, ±h/2) offsets cover the plane
    exactly once (like iso::drawDiamond's two triangles; PIL's polygon() left
    dotted seams)."""
    ys, xs = np.mgrid[0:h, 0:w]
    px = xs + 0.5 - w / 2.0
    py = ys + 0.5 - h / 2.0
    d = np.abs(px) / (w / 2.0) + np.abs(py) / (h / 2.0)
    inside = d < 1.0
    # half-open tie-break on the exact boundary: keep the pixel for the diamond
    # whose centre is up-left of it (so each boundary pixel belongs to one tile)
    tie = np.isclose(d, 1.0) & ((px < 0) | ((px == 0) & (py < 0)))
    return Image.fromarray(((inside | tie) * 255).astype(np.uint8), "L")


def cut_diamond(plate: Image.Image, x: int, y: int, *, w: int = 64, h: int = 32) -> Image.Image:
    """Paint-then-cut (Soma/Mir): cut the diamond at its own screen position so
    neighbours are continuous by construction. Plate coords wrap."""
    pw, ph = plate.size
    x %= max(1, pw); y %= max(1, ph)
    if x + w <= pw and y + h <= ph:
        t = plate.crop((x, y, x + w, y + h)).copy()
    else:  # wrap by tiling 2x2
        big = Image.new("RGBA", (pw * 2, ph * 2))
        for i in range(2):
            for j in range(2):
                big.paste(plate.convert("RGBA"), (i * pw, j * ph))
        t = big.crop((x, y, x + w, y + h)).copy()
    t = t.convert("RGBA")
    t.putalpha(diamond_mask(w, h))
    return t


# --- A12 edge/transition masks -------------------------------------------------
# Tile-space adjacency → screen side, from engine/render/iso.cpp tileToWorld
# (x = (tx-ty)*w/2, y = (tx+ty)*h/2): +tx moves screen SE, +ty moves screen SW.
EDGE_FACES = {"NE": (0, -1), "SE": (1, 0), "SW": (0, 1), "NW": (-1, 0)}      # share a face
EDGE_POINTS = {"N": (-1, -1), "E": (1, -1), "S": (1, 1), "W": (-1, 1)}      # touch at a point
_POINT_FACES = {"N": ("NE", "NW"), "E": ("NE", "SE"), "S": ("SE", "SW"), "W": ("SW", "NW")}


def _face_depth(u: np.ndarray, v: np.ndarray, face: str) -> np.ndarray:
    """Signed depth measured inward from one diamond face, in diamond units
    (0 on the face, 2 at the opposite face). u,v = pixel centre / half-size."""
    return {"NE": 1.0 - (u - v), "SE": 1.0 - (u + v),
            "SW": 1.0 - (-u + v), "NW": 1.0 - (-u - v)}[face]


def edge_masks(w: int = 64, h: int = 32, *, depth: float = 0.6, cap: float = 0.5,
               feather: int = 2, seed: int = 1999) -> dict[str, Image.Image]:
    """A12: the 8 transition masks per adjacency pair as 'L' images — 255 =
    material B over material A, 128 = 50 % checker band, 0 = A.

    * edge_<face>  — B is the tile across that face: a band parallel to the
      face, `depth` (diamond units, 0.6 ≈ 10 px tall at 64×32) deep, boundary
      jittered ±1 px per scanline (fixed seed). The face row itself is always
      B, so the piece butts seamlessly against the full-B neighbour.
    * corner_<pt>  — B only touches at that point (diagonal neighbour): a
      mini-diamond cap = intersection of the two adjoining face bands at
      depth `cap`; its inner edges stay parallel to the tile's own faces.
    Any 8-neighbour configuration is a union of these (see edge_mask_for)."""
    rng = np.random.default_rng(seed)
    ys, xs = np.mgrid[0:h, 0:w]
    u = (xs + 0.5 - w / 2.0) / (w / 2.0)
    v = (ys + 0.5 - h / 2.0) / (h / 2.0)
    dm = np.asarray(diamond_mask(w, h)) > 0
    px_v = 2.0 / h                      # one vertical pixel in diamond units
    band = feather * px_v               # width of the checker band
    # per-scanline jitter: ±1 horizontal px (= px_v/2 in depth units) + slow wobble
    jit = (rng.integers(-1, 2, size=h) * (px_v / 2.0)
           + 0.5 * px_v * np.sin(np.arange(h) / 2.7 + rng.uniform(0, 6.28))).astype(np.float32)
    jit2d = jit[:, None]

    def three_step(s: np.ndarray, thr: float) -> np.ndarray:
        m = np.zeros((h, w), np.uint8)
        m[s <= thr + jit2d + band] = 128
        m[s <= thr + jit2d] = 255
        m[~dm] = 0
        return m

    out: dict[str, Image.Image] = {}
    for face in EDGE_FACES:
        out[f"edge_{face}"] = Image.fromarray(three_step(_face_depth(u, v, face), depth), "L")
    for pt, (fa, fb) in _POINT_FACES.items():
        s = np.maximum(_face_depth(u, v, fa), _face_depth(u, v, fb))   # inside both bands
        out[f"corner_{pt}"] = Image.fromarray(three_step(s, cap), "L")
    return out


def edge_mask_for(b_neighbours: set[str] | list[str], masks: dict[str, Image.Image] | None = None,
                  **kw) -> Image.Image:
    """Mask for an A tile whose B neighbours are the given sides ("NE","SE",
    "SW","NW" faces; "N","E","S","W" points). Points are only added when
    neither adjoining face is B (the face band already covers the vertex).
    Returns the pixel-wise max of the selected base masks (empty set → all 0)."""
    masks = masks or edge_masks(**kw)
    nb = set(b_neighbours)
    sel = [masks[f"edge_{f}"] for f in EDGE_FACES if f in nb]
    for pt, (fa, fb) in _POINT_FACES.items():
        if pt in nb and fa not in nb and fb not in nb:
            sel.append(masks[f"corner_{pt}"])
    w, h = next(iter(masks.values())).size
    acc = np.zeros((h, w), np.uint8)
    for m in sel:
        acc = np.maximum(acc, np.asarray(m))
    return Image.fromarray(acc, "L")


def blend_edge(tile_a: Image.Image, tile_b: Image.Image, mask: Image.Image, *, palette: np.ndarray | None = None) -> Image.Image:
    """Compose an edge tile from two cut diamonds + an edge mask; the 128 band
    is resolved by an exact 50 % checkerboard (era-correct), other grey levels
    by Bayer-2 ordered dither, then optionally quantised to the *union* family
    palette. Alpha = max(a, b) so the diamond stays fully opaque."""
    a = np.asarray(tile_a.convert("RGBA")).astype(np.float32)
    b = np.asarray(tile_b.convert("RGBA")).astype(np.float32)
    m = np.asarray(mask).astype(np.float32) / 255.0
    h, w = m.shape
    # thresholds {0.125, 0.625, 0.875, 0.375}: m=0.5 → exactly 2 of 4 cells
    bay = np.tile(_BAYER2 + 0.5 + 0.125, (h // 2 + 1, w // 2 + 1))[:h, :w]
    use_b = (m >= 1.0) | ((m > 0.0) & (bay < m))
    out = np.where(use_b[..., None], b, a)
    out[..., 3] = np.maximum(a[..., 3], b[..., 3])
    im = Image.fromarray(out.astype(np.uint8), "RGBA")
    if palette is not None:
        im = quantize(im, palette, dither="none")
        im.putalpha(Image.fromarray(out[..., 3].astype(np.uint8), "L"))
    return im

