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
