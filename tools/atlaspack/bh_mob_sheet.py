#!/usr/bin/env python3
"""bh_mob_sheet — B3/B4 production chain: native plates → 8-dir × N-frame cells → sheet.

    python3 tools/atlaspack/bh_mob_sheet.py <mob_id> [--plates-dir DIR] [--out DIR]
        [--body-h 20] [--family vermin] [--anims walk4,attack3,die3] [--hover 12]
        [--no-derive]  (only cells for plates that exist; for QA of a native plate)

Inputs (assets/aigen/mobs/<id>_<slug>/plates/ by default):
    <slug>_S_4x_raw.png   <slug>_SE_4x_raw.png   <slug>_E_4x_raw.png
  — the three NATIVE generations the prompt.md asks for (bible §14: native S/SE/E,
  the rest derived). Any missing native plate falls back to the nearest one.

Direction derivation (documented, deterministic, no mirroring of *asymmetric*
gear per bible §7 — see `--asym` below):
    S   ← native S              N   ← S, head/face rows re-shaded (back view)
    SE  ← native SE             NE  ← E, torso rows re-shaded (3/4 back)
    E   ← native E              SW  ← SE mirrored  ┐ allowed only when the mob is
    W   ← E mirrored            NW  ← NE mirrored  ┘ symmetric (`--asym` off)
  For asymmetric mobs (`--asym`, e.g. Gnoll club on the right shoulder) the W-side
  dirs are built from the E-side plates by a *left-right re-composition*: the body
  is mirrored but the asymmetric attachment (a rectangle given by --asym-box, in
  4× plate px, from the E plate) is pasted back UN-mirrored on the right side.
  This is the honest offline stand-in for the img2img pass the prompt specifies;
  the pack report flags every derived direction so the hand-fix list stays true.

Frame derivation per anim (20-mobs.md anim notes, applied as pixel ops on the cell):
    walk  4f  bob 0/-1/0/-1 px + leg alternation (bottom 6 rows shear ±1 px) + tail whip
    attack 3f rear (-1 px back) → lunge (+N px toward facing, contact) → recoil
    die   3f  lean → flop (rotate 90° toward facing side, squash 60 %) → still + spatter
    fly (bat) wing rows squash on f1/f3 (blur allowed), body hovers `--hover` px

Each cell goes through the B0 chain (key → harden → fit → gamma → quantize to the
FAMILY ramp (assets/aigen/palettes/<family>.png, ≤32) → 1px outline → place in
cell with alpha-70 shadow). Output:
    <out>/cells/<anim>_<DIR>_<f>.png     (80 cells for walk4/attack3/die3)
    <out>/sheet.png + sheet.json         (via bh_pack_sheet, v1 + anchorY)
    <out>/derivation.json                (which dirs/frames are native vs derived)
Deterministic. Everything is UNVALIDATED in engine until T-ART-01/05.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
DIRS = P.DIR_ORDER  # E SE S SW W NW N NE
FACING_DX = {"E": 1, "SE": 1, "S": 0, "SW": -1, "W": -1, "NW": -1, "N": 0, "NE": 1}
FACING_DY = {"E": 0, "SE": 1, "S": 1, "SW": 1, "W": 0, "NW": -1, "N": -1, "NE": -1}
OUT = np.array(P.OUTLINE_RGB, np.uint8)


# ----------------------------------------------------------------------------
# palette
# ----------------------------------------------------------------------------
def family_palette(name: str) -> np.ndarray:
    js = json.loads((ROOT / "assets/aigen/palettes/families.json").read_text())
    cols = js["families"][name]["colours"]
    return np.array([[int(h[i:i + 2], 16) for i in (1, 3, 5)] for h in cols], dtype=np.uint8)


# ----------------------------------------------------------------------------
# plate → sprite (B0 make_sprite, parameterised)
# ----------------------------------------------------------------------------
def plate_to_sprite(plate: Image.Image, body_h: int, pal: np.ndarray | None, *, gamma=0.85, contrast=1.0, cell_w: int = 32) -> Image.Image:
    """B0 make_sprite chain up to (optionally) the family quantize; outline is
    applied per frame later so shears/flops do not break it."""
    k = P.key_out_green(plate)
    k = P.crop_to_alpha(P.harden_alpha(k))
    s = P.fit_to_cell(k, target_h=body_h)
    if s.width > cell_w:  # never wider than the cell; long profiles get width-limited (flagged in derivation.json)
        s = P.fit_to_cell(k, target_w=cell_w)
    s = P.harden_alpha(s)
    sa = np.asarray(s).astype(np.float32)
    lin = (sa[..., :3] / 255.0) ** gamma
    lin = np.clip((lin - 0.5) * contrast + 0.5, 0, 1)
    sa[..., :3] = lin * 255.0
    s = Image.fromarray(sa.astype(np.uint8), "RGBA")
    if pal is not None:
        s = P.quantize(s, pal, dither="bayer2", strength=0.10)
    return s


def pin_pixels(sprite: Image.Image, rule: str, hex_rgb: str) -> tuple[Image.Image, int]:
    """Deterministic stand-in for the brief's 'paint by hand' steps. rule:
      'lum>N'  – body pixels brighter than N luma → colour (elite bindings #e6e0d4, ≤ 8 % gate checked by caller)
      'ember'  – warm-orange pixels (r>140, 50<g<150, b<90, r-b>70) → colour (censer ember #c8622a, survives gamma+quantize)
    Returns (image, pixels_pinned)."""
    a = np.asarray(sprite).copy()
    m = a[..., 3] > 0
    r, g, b = (a[..., i].astype(int) for i in range(3))
    if rule.startswith("lum>"):
        t = float(rule[4:])
        sel = m & ((0.299 * r + 0.587 * g + 0.114 * b) > t)
    elif rule == "ember":
        # hue test (works after any gamma): clearly orange = r > g > b with wide gaps
        sel = m & (r > 140) & (r > g + 25) & (g > b + 20) & ((r - b) > 70)
        # grow by one pixel inside the body so the 1-px outline pass cannot eat the whole ember (brief: 2 px permanent)
        for _ in range(2):
            pad = np.pad(sel, 1)
            sel = (pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:] | sel) & m
    else:
        raise SystemExit(f"unknown pin rule {rule}")
    rgb = tuple(int(hex_rgb.lstrip("#")[i:i + 2], 16) for i in (0, 2, 4))
    a[sel, :3] = rgb
    return Image.fromarray(a, "RGBA"), int(sel.sum())


def quiet_band(sprite: Image.Image, rows: int, tones: int = 3, pal: np.ndarray | None = None) -> Image.Image:
    """Boss rule (20-mobs.md 1009 / boss_occupancy finding): the lowest `rows` px of
    the body are flattened to `tones` luma steps of ONE hue (the band's mean
    chroma) so player heads read against the bell dome. Applied before the
    family quantize; a 1-px darker rim is kept at the very bottom for the lip."""
    a = np.asarray(sprite).copy()
    h = a.shape[0]
    band = a[max(0, h - rows):]
    m = band[..., 3] > 0
    if m.sum() == 0:
        return sprite
    px = band[m][:, :3].astype(np.float32)
    lum = 0.299 * px[:, 0] + 0.587 * px[:, 1] + 0.114 * px[:, 2]
    mean_rgb = px.mean(axis=0)
    mean_lum = lum.mean()
    chroma = mean_rgb - mean_lum  # hue offset of the band
    # tones sit tightly around the band mean (±14 luma): low contrast by construction,
    # so the crack / rim detail drops out of the lower dome while the shading direction survives
    steps = mean_lum + np.linspace(-14.0, 14.0, tones)
    lo, hi = np.percentile(lum, 8), np.percentile(lum, 92)
    src = np.clip((lum - lo) / max(1.0, hi - lo), 0, 1) * (tones - 1)
    new = steps[np.rint(src).astype(int)]
    out = np.clip(new[:, None] + chroma[None, :], 0, 255)
    if pal is not None:  # snap the tones to family colours now, so the per-frame dithered quantize is idempotent here
        tone_rgb = np.clip(steps[:, None] + chroma[None, :], 0, 255)
        near = pal[np.argmin(((tone_rgb[:, None, :] - pal[None, :, :].astype(np.float32)) ** 2).sum(-1), axis=1)]
        out = near[np.rint(src).astype(int)]
    band[m, :3] = out.astype(np.uint8)
    # feather: the 2 rows ABOVE the band get a checkerboard of the band's top tone so the boundary reads as dither, not a stripe
    y0 = max(0, h - rows)
    if y0 >= 2:
        top_tone = out[np.argsort(lum)[-max(1, len(lum) // 3):]].mean(axis=0).astype(np.uint8)
        for yy in (y0 - 2, y0 - 1):
            for xx in range(a.shape[1]):
                if a[yy, xx, 3] > 0 and (yy + xx) % 2 == 0:
                    a[yy, xx, :3] = top_tone
    # lip: darken the last opaque row so the rim still reads
    last = a[h - 1]
    lm = last[..., 3] > 0
    last[lm, :3] = (last[lm, :3].astype(np.float32) * 0.72).astype(np.uint8)
    return Image.fromarray(a, "RGBA")


def match_colour(sprite: Image.Image, ref: Image.Image) -> Image.Image:
    """Unify hue drift between separate generations of the same creature:
    per-channel mean/std of the opaque body pixels are matched to `ref`
    (Reinhard-style transfer in RGB). Deterministic; applied before the family
    quantize so all 8 dirs land on the same ramp entries."""
    a = np.asarray(sprite).astype(np.float32); r = np.asarray(ref).astype(np.float32)
    ma, mr = a[..., 3] > 0, r[..., 3] > 0
    if ma.sum() < 16 or mr.sum() < 16:
        return sprite
    out = a.copy()
    for c in range(3):
        xa, xr = a[..., c][ma], r[..., c][mr]
        sa, sr = max(xa.std(), 1.0), max(xr.std(), 1.0)
        out[..., c][ma] = np.clip((xa - xa.mean()) * (sr / sa) * 0.7 + xa.mean() * 0.3 + xr.mean() * 0.7, 0, 255)
    return Image.fromarray(out.astype(np.uint8), "RGBA")


def finish(sprite: Image.Image, pal: np.ndarray, *, cell=(32, 48), feet_y=42, hover=0, shadow_rx=9) -> Image.Image:
    s = P.quantize(sprite, pal, dither="none")
    s = P.outline(s)
    out = P.place_in_cell(s, cell=cell, feet_y=feet_y - hover, shadow_rx=shadow_rx, shadow_ry=max(3, shadow_rx // 3))
    if hover:  # shadow must stay on the ground row, body lifted
        c = np.asarray(out).copy()
        c[..., 3][c[..., 3] == 70] = 0  # drop lifted shadow
        out = Image.fromarray(c, "RGBA")
        base = P.place_in_cell(Image.new("RGBA", (1, 1)), cell=cell, feet_y=feet_y, shadow_rx=shadow_rx, shadow_ry=max(3, shadow_rx // 3))
        base.alpha_composite(out)
        out = base
    return out


# ----------------------------------------------------------------------------
# direction derivation
# ----------------------------------------------------------------------------
def mirror(img: Image.Image) -> Image.Image:
    return img.transpose(Image.FLIP_LEFT_RIGHT)


def reshade_back(sprite: Image.Image, rows_frac=(0.0, 0.45), amount=0.82) -> Image.Image:
    """Back views: the face/front detail rows get darkened + de-contrasted so the
    read is 'turned away' (HB back sprites are visibly plainer). Pixel-op stand-in
    for a native N plate; flagged as derived."""
    a = np.asarray(sprite).astype(np.float32)
    h = a.shape[0]
    y0, y1 = int(h * rows_frac[0]), int(h * rows_frac[1])
    band = a[y0:y1, :, :3]
    mean = band.mean(axis=(0, 1), keepdims=True) if band.size else 0
    a[y0:y1, :, :3] = np.clip((band - mean) * 0.6 + mean * amount, 0, 255)
    return Image.fromarray(a.astype(np.uint8), "RGBA")


def recompose_asym(mirrored: Image.Image, e_sprite: Image.Image, box: tuple[int, int, int, int],
                   *, min_luma: float = 105.0) -> Image.Image:
    """W-side stand-in for asymmetric gear (bible §7: never *just* mirror it).

    Geometry: facing E the right shoulder is the NEAR side, so shoulder gear is
    drawn in front of the body and points backward (screen-left). A mirror gives
    a W-facing body whose gear still points backward (correct) but sits on the
    near = LEFT shoulder (wrong hand). The true right-shoulder W view has the
    gear on the FAR shoulder: same direction, drawn BEHIND the body. So: keep
    the mirror, but push the attachment behind the body — attachment pixels
    survive only where they clear the body silhouette; the body pixels the
    attachment used to cover are in-painted from the nearest body pixel below
    (shoulder fur). Attachment = bright pixels (luma ≥ min_luma: bone/iron) in
    `box` (E-sprite px), grown by 1 px for its outline ring."""
    x0, y0, x1, y1 = box
    e = np.asarray(e_sprite).astype(np.float32)
    lum = e[..., 0] * .299 + e[..., 1] * .587 + e[..., 2] * .114
    att = np.zeros(lum.shape, bool)
    att[y0:y1, x0:x1] = (e[y0:y1, x0:x1, 3] > 0) & (lum[y0:y1, x0:x1] >= min_luma)
    pad = np.pad(att, 1)
    att = (att | pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:]) & (e[..., 3] > 0)
    att_m = att[:, ::-1]                                  # mirrored attachment mask
    m = np.asarray(mirrored).copy()
    body = (m[..., 3] > 0) & ~att_m
    h, w = body.shape
    # body hull per column: rows between the first and last body pixel
    hull = np.zeros_like(body)
    for x in range(w):
        ys = np.where(body[:, x])[0]
        if len(ys):
            hull[ys.min():ys.max() + 1, x] = True
    covered = att_m & hull                                # attachment pixels that overlap the body → body shows instead
    out = m.copy()
    for y, x in zip(*np.where(covered)):
        below = np.where(body[y:, x])[0]
        above = np.where(body[:y, x])[0]
        src_y = y + below[0] if len(below) else above[-1]
        out[y, x] = m[src_y, x]
    return Image.fromarray(out, "RGBA")


def hide_face(sprite: Image.Image, head_frac: float) -> Image.Image:
    """Back views of hooded/masked mobs: pale face/mask pixels in the head rows are
    replaced by the darkest frequent body colour (the hood), so N/NW do not show a face."""
    a = np.asarray(sprite).copy()
    h = a.shape[0]
    rows = a[: max(1, int(h * head_frac))]
    lum = 0.299 * rows[..., 0] + 0.587 * rows[..., 1] + 0.114 * rows[..., 2]
    body = a[a[..., 3] > 0][:, :3]
    if len(body) == 0:
        return sprite
    dark = body[(0.299 * body[:, 0] + 0.587 * body[:, 1] + 0.114 * body[:, 2]) < 60]
    fill = np.median(dark if len(dark) else body, axis=0).astype(np.uint8)
    m = (rows[..., 3] > 0) & (lum > 105)
    rows[m, :3] = fill
    return Image.fromarray(a, "RGBA")


def derive_dirs(native: dict[str, Image.Image], *, asym_box=None, hide_face_frac: float = 0.0) -> tuple[dict[str, Image.Image], dict[str, str]]:
    """native: subset of {'S','SE','E','N'} sprites (already fitted+quantised, no outline)."""
    prov = {}
    S = native.get("S") or native.get("SE") or native["E"]
    SE = native.get("SE") or S
    E = native.get("E") or SE
    prov["S"] = "native" if "S" in native else "fallback"
    prov["SE"] = "native" if "SE" in native else "fallback"
    prov["E"] = "native" if "E" in native else "fallback"
    if "N" in native:
        N = native["N"]; prov["N"] = "native"
    else:
        N = reshade_back(S, (0.0, 0.45)); prov["N"] = "derived: S re-shaded (back view)"
        if hide_face_frac:
            N = hide_face(N, hide_face_frac); prov["N"] += " + face hidden (hood)"
    NE = reshade_back(E, (0.0, 0.35), 0.9); prov["NE"] = "derived: E re-shaded (3/4 back)"
    if hide_face_frac:
        NE = hide_face(NE, hide_face_frac); prov["NE"] += " + face hidden (hood)"
    if asym_box is None:
        W, SW, NW = mirror(E), mirror(SE), mirror(NE)
        prov.update(W="derived: E mirrored (symmetric mob)", SW="derived: SE mirrored", NW="derived: NE mirrored")
    else:
        W = recompose_asym(mirror(E), E, asym_box)
        SW = recompose_asym(mirror(SE), SE, asym_box)
        NW = recompose_asym(mirror(NE), NE, asym_box)
        prov.update(W="derived: E mirrored, shoulder gear pushed behind the body (far = right shoulder)",
                    SW="derived: SE mirrored, gear behind body (asym)",
                    NW="derived: NE mirrored, gear behind body (asym)")
    return {"E": E, "SE": SE, "S": S, "SW": SW, "W": W, "NW": NW, "N": N, "NE": NE}, prov


# ----------------------------------------------------------------------------
# frame derivation (pixel ops on the un-outlined sprite)
# ----------------------------------------------------------------------------
def shift(img: Image.Image, dx: int, dy: int) -> Image.Image:
    a = np.asarray(img)
    out = np.zeros_like(a)
    h, w = a.shape[:2]
    ys, yd = (slice(0, h - dy), slice(dy, h)) if dy >= 0 else (slice(-dy, h), slice(0, h + dy))
    xs, xd = (slice(0, w - dx), slice(dx, w)) if dx >= 0 else (slice(-dx, w), slice(0, w + dx))
    out[yd, xd] = a[ys, xs]
    return Image.fromarray(out, "RGBA")


def shear_rows(img: Image.Image, y0: int, dx: int) -> Image.Image:
    """Shift rows y0.. by dx (legs alternate)."""
    a = np.asarray(img).copy()
    a[y0:] = np.asarray(shift(Image.fromarray(a[y0:], "RGBA"), dx, 0))
    return Image.fromarray(a, "RGBA")


def squash(img: Image.Image, fy: float, fx: float = 1.0) -> Image.Image:
    w, h = img.size
    nw, nh = max(1, round(w * fx)), max(1, round(h * fy))
    return img.resize((nw, nh), Image.NEAREST)


def rotate_flop(img: Image.Image, toward_dx: int) -> Image.Image:
    """Die frame: rotate 90° so the body lies along the ground, feet toward the facing side."""
    r = img.transpose(Image.ROTATE_90 if toward_dx >= 0 else Image.ROTATE_270)
    return squash(r, 0.6, 1.0)


def add_spatter(img: Image.Image, rgb=(0x3A, 0x08, 0x0C), n=6, seed=7) -> Image.Image:
    rng = np.random.default_rng(seed)
    a = np.asarray(img).copy()
    h, w = a.shape[:2]
    for _ in range(n):
        x, y = int(rng.integers(0, w)), int(rng.integers(h * 2 // 3, h))
        a[y, x, :3] = rgb; a[y, x, 3] = 255
    return Image.fromarray(a, "RGBA")


def flash_white(img: Image.Image, rgb=(0xF4, 0xEC, 0xE0)) -> Image.Image:
    """Hit-flash frame: body → bone-white silhouette (palette slot hit_flash), outline kept."""
    a = np.asarray(img).copy()
    m = a[..., 3] > 0
    a[m, :3] = rgb
    return Image.fromarray(a, "RGBA")


def attack_fx(img: Image.Image, d: str, fx: str) -> Image.Image:
    """Contact-frame attachment drawn INTO the sprite (bible: strikes read at 1x).
    line8 = 8 px silk line (bone_lt) from the body edge toward the facing;
    dots5 = 5 ember dots on a censer arc toward the facing."""
    fdx, fdy = FACING_DX[d], FACING_DY[d]
    pad = 9
    a = np.asarray(img)
    h, w = a.shape[:2]
    c = np.zeros((h + pad, w + 2 * pad, 4), np.uint8)
    c[pad:, pad:pad + w] = a
    cy = pad + h // 2
    if fx == "line8":
        x0 = pad + (w if fdx > 0 else -1 if fdx < 0 else w // 2)
        for i in range(8):
            x, y = x0 + i * (fdx if fdx else 0), cy - 2 + (i * fdy if fdy else 0)
            if fdx == 0:  # N/S: vertical line over/under the body centre
                y = pad + (h + i if fdy > 0 else -1 - i) if fdy else cy
                x = pad + w // 2
            if 0 <= x < c.shape[1] and 0 <= y < c.shape[0]:
                c[y, x] = (0xE6, 0xE0, 0xD4, 255)
    elif fx == "dots5":
        for i in range(5):
            t = i / 4.0
            x = pad + w // 2 + int(round((w // 2 + 2 + i * 1.4) * (fdx if fdx else 0.6 * (1 - 2 * (i % 2)))))
            y = cy - 3 + int(round(6 * t * t - 1 + (4 * fdy * t if fdy else 0)))
            if 0 <= x < c.shape[1] and 0 <= y < c.shape[0]:
                c[y, x] = (0xC8, 0x62, 0x2A, 255)
    return Image.fromarray(c, "RGBA")


def frames_for(anim: str, n: int, base: Image.Image, d: str, *, kind: str, lunge_px: int, walk_style: str = "bob", fx: str | None = None) -> list[Image.Image]:
    fdx, fdy = FACING_DX[d], FACING_DY[d]
    h = base.height
    out = []
    if anim == "walk":
        for f in range(n):
            im = base
            if kind == "fly":
                if f % 2 == 1:  # wing blur frames: squash wing rows
                    im = squash(base, 0.85, 1.0)
                im = shift(im, 0, -1 if f % 2 == 0 else 0)
            elif walk_style == "glide":          # robes / spiders: no vertical bob, legs hidden or tetrapod
                im = shear_rows(base, int(h * 0.8), 1 if f in (0, 1) else -1) if kind == "spider" else shift(base, 0, -1 if f == 1 else 0)
            elif walk_style == "drag":           # Sexton: 0/1 lift, 2/3 drag the LEFT foot (asymmetric — never mirror)
                im = shift(base, 0, -1 if f in (0, 1) else 0)
                if f in (2, 3):
                    im = shear_rows(im, int(h * 0.85), -1)   # trailing foot lags
            elif walk_style == "bell":           # boss: the bell drags, 2-frame rock
                im = shift(base, 1 if f in (1, 2) else 0, 0)
            else:
                bob = -1 if f % 2 == 1 else 0
                im = shift(base, 0, bob)
                im = shear_rows(im, int(h * 0.75), 1 if f in (0, 1) else -1)  # legs alternate
            out.append(im)
    elif anim == "attack":
        seq = [(-fdx, 0), (fdx * lunge_px, fdy * max(1, lunge_px // 2)), (0, 0)]  # rear, contact, recoil
        for f in range(n):
            dx, dy = seq[min(f, 2)]
            im = shift(base, dx, dy)
            if f == 0 and kind not in ("fly", "boss"):  # wind-up: slight crouch
                im = squash(im, 0.94, 1.0)
            if f == 1 and kind == "fly":  # belly-slam: drop
                im = shift(im, 0, 4)
            if f == 1 and fx:
                im = attack_fx(im, d, fx)
            out.append(im)
    elif anim == "hurt":
        out = [shift(base, -fdx * 2, 0), flash_white(shift(base, -fdx * 2, 0))][:n]
        while len(out) < n:
            out.append(base)
    elif anim == "cast":
        # hands to chest (squash 0.96) → hold → release (lean +1 toward facing) → recover
        seq = [squash(base, 0.96, 1.0), squash(base, 0.96, 1.0), shift(base, fdx, 0), base]
        out = [seq[min(f, 3)] for f in range(n)]
    elif anim == "summon":
        # grip → heave (up 2) → tilt (shear whole body 2 px toward facing) → settle
        tilt = shear_rows(base, int(h * 0.4), fdx * 2)
        seq = [base, shift(base, 0, -2), tilt, base]
        out = [seq[min(f, 3)] for f in range(n)]
    elif anim == "die":
        if kind == "spider":
            # legs fold under the coffin, lid splits: flatten, widen, then ichor spatter (no 90° flop — spiders don't lie on a side)
            fold = squash(base, 0.8, 1.0)
            flat = squash(base, 0.55, 1.1)
            still = add_spatter(squash(base, 0.45, 1.15), rgb=(0x14, 0x10, 0x14), n=8)
            seq = [fold, flat, still]
        elif walk_style == "glide" and kind == "ground":
            # robes: the mask falls, the robe collapses into a heap (never a lying body — there is none inside)
            drop = shift(squash(base, 0.9, 1.0), 0, 0)
            heap = squash(base, 0.55, 1.15)
            still = add_spatter(squash(base, 0.4, 1.25), rgb=(0x2A, 0x1A, 0x30), n=4)
            seq = [drop, heap, still]
        elif kind == "boss":
            # bell cracks open: torso sinks (crop rows from top), bell settles (squash) — the corpse is a split bell
            sink = shift(squash(base, 0.92, 1.0), 0, 0)
            sink2 = squash(base, 0.8, 1.06)
            settle = squash(base, 0.7, 1.1)
            still = add_spatter(settle, rgb=(0x2A, 0x1A, 0x30), n=10)  # violet-black rot, curse licence
            seq = [sink, sink2, settle, still]
        else:
            lean = shift(squash(base, 0.92, 1.0), fdx, 0)
            flop = rotate_flop(base, fdx)
            still = add_spatter(squash(flop, 0.9, 1.05))
            seq = [lean, flop, still]
        out = [seq[min(f, len(seq) - 1)] for f in range(n)]
    else:
        out = [base for _ in range(n)]
    return out


# ----------------------------------------------------------------------------
def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("mob_id", type=int)
    ap.add_argument("--plates-dir")
    ap.add_argument("--out")
    ap.add_argument("--body-h", type=int, required=True)
    ap.add_argument("--family", required=True)
    ap.add_argument("--anims", default="walk4,attack3,die3")
    ap.add_argument("--kind", choices=["ground", "fly", "spider", "boss"], default="ground")
    ap.add_argument("--cell", default="32x48", help="32x48 common · 40x60 elite (D5) · 64x64 boss")
    ap.add_argument("--anchor-y", type=int, default=None, help="feet row; default 42 / 52 / 58 by cell")
    ap.add_argument("--walk-style", choices=["bob", "glide", "drag", "bell"], default="bob")
    ap.add_argument("--attack-fx", choices=["line8", "dots5"], default=None, help="contact-frame attachment drawn into the sprite")
    ap.add_argument("--n-hide-face", type=float, default=0.0, help="hooded/masked mobs: hide pale face pixels in the top FRAC rows of derived N/NE")
    ap.add_argument("--quiet-band", type=int, default=0, help="boss: flatten the lowest N body rows to 3 tones (bell dome rule)")
    ap.add_argument("--pin", action="append", default=[], help="RULE=HEX, applied after gamma before quantize: lum>150=e6e0d4 (elite bindings) · ember=c8622a (censer)")
    ap.add_argument("--quiet-ramp", default=None, help="palette indices (comma list) the quiet band may use, e.g. the bronze+verdigris ramps; default = whole family")
    ap.add_argument("--hover", type=int, default=0, help="fly: body lift in px (shadow stays on anchor)")
    ap.add_argument("--lunge", type=int, default=3)
    ap.add_argument("--match-to", default="S", help="native dir whose colour statistics the other natives are matched to (hue drift between generations)")
    ap.add_argument("--asym-box", help="x0,y0,x1,y1 in E-sprite px of the asymmetric attachment (enables asym mode)")
    ap.add_argument("--shadow-rx", type=int, default=9)
    ap.add_argument("--contrast", type=float, default=1.0)
    ap.add_argument("--gamma", type=float, default=0.85, help="palette lift (B0 = 0.85); lower = brighter body for R-LUMA")
    args = ap.parse_args(argv)

    folder = next(ROOT.glob(f"assets/aigen/mobs/{args.mob_id}_*"))
    slug = folder.name.split("_", 1)[1]
    plates_dir = Path(args.plates_dir) if args.plates_dir else folder / "plates"
    out = Path(args.out) if args.out else folder
    cells_dir = out / "cells"
    cells_dir.mkdir(parents=True, exist_ok=True)
    for old in cells_dir.glob("*.png"):
        old.unlink()
    pal = family_palette(args.family)
    cw, ch = (int(v) for v in args.cell.lower().split("x"))
    anchor_y = args.anchor_y if args.anchor_y is not None else {48: 42, 60: 52, 64: 58}.get(ch, ch - 6)

    native, fit_notes, pin_notes = {}, {}, {}
    for d in ("S", "SE", "E", "N"):
        p = plates_dir / f"{slug}_{d}_4x_raw.png"
        if p.exists():
            raw = plate_to_sprite(Image.open(p), args.body_h, None, gamma=args.gamma, contrast=args.contrast, cell_w=cw)
            if args.quiet_band:
                ramp = pal[[int(i) for i in args.quiet_ramp.split(",")]] if args.quiet_ramp else pal
                raw = quiet_band(raw, args.quiet_band, pal=ramp)
            for rule_hex in args.pin:
                rule, hx = rule_hex.split("=")
                raw, n = pin_pixels(raw, rule, hx)
                pin_notes.setdefault(d, {})[rule_hex] = {"pixels": n, "pct_of_body": round(100.0 * n / max(1, int((np.asarray(raw)[..., 3] > 0).sum())), 1)}
            fit_notes[d] = {"sprite_px": list(raw.size),
                            "width_limited": raw.height < args.body_h - 1}
            native[d] = raw
    if not native:
        print(f"no native plates in {plates_dir}"); return 2
    ref = native.get(args.match_to)
    for d in list(native):
        if ref is not None and d != args.match_to:
            native[d] = match_colour(native[d], ref)
        native[d] = P.quantize(native[d], pal, dither="bayer2", strength=0.10)
    asym = tuple(int(v) for v in args.asym_box.split(",")) if args.asym_box else None
    dirs, prov = derive_dirs(native, asym_box=asym, hide_face_frac=args.n_hide_face)

    anims = [(a.rstrip("0123456789"), int(a[len(a.rstrip("0123456789")):])) for a in args.anims.split(",")]
    for anim, n in anims:
        for d in DIRS:
            fr = frames_for(anim, n, dirs[d], d, kind=args.kind, lunge_px=args.lunge, walk_style=args.walk_style, fx=args.attack_fx)
            for f, im in enumerate(fr):
                cell = finish(im, pal, cell=(cw, ch), feet_y=anchor_y,
                              hover=args.hover if args.kind == "fly" and anim != "die" else 0,
                              shadow_rx=args.shadow_rx)
                cell.save(cells_dir / f"{anim}_{d}_{f}.png")

    deriv = {"mob_id": args.mob_id, "slug": slug, "family": args.family, "body_h": args.body_h,
             "cell": [cw, ch], "anchor_y": anchor_y, "kind": args.kind, "walk_style": args.walk_style,
             "attack_fx": args.attack_fx, "n_hide_face": args.n_hide_face, "quiet_band_rows": args.quiet_band, "quiet_ramp": args.quiet_ramp, "pins": pin_notes,
             "gamma": args.gamma, "contrast": args.contrast, "hover": args.hover if args.kind == "fly" else 0,
             "asym_box": list(asym) if asym else None, "colour_matched_to": args.match_to if ref is not None else None,
             "native_fit": fit_notes,
             "native_plates": sorted(native), "direction_provenance": prov,
             "frame_provenance": "walk/attack/die frames are pixel-op derivations of the per-direction base "
                                 "(bob/shear/lunge/flop) — first-pass for T-ART-01/05 bring-up; hand-fix list in prompt.md applies",
             "status": "UNVALIDATED in engine"}
    (out / "derivation.json").write_text(json.dumps(deriv, indent=1) + "\n")

    fps = ",".join(f"{a}={ {'walk':10,'attack':12,'cast':10,'hurt':12,'die':8,'summon':8}.get(a,8)}" for a, _ in anims)
    order = ",".join(a for a, _ in anims)
    r = subprocess.run([sys.executable, str(Path(__file__).parent / "bh_pack_sheet.py"), str(cells_dir), str(out),
                        "--fps", fps, "--order", order, "--cell", f"{cw}x{ch}", "--anchor-y", str(anchor_y)],
                       capture_output=True, text=True)
    print(r.stdout.strip()[-600:])
    if r.returncode != 0:
        print(r.stderr[-800:]); return 1
    print(f"wrote {out.relative_to(ROOT)}/sheet.png + sheet.json + derivation.json ({sum(n for _, n in anims) * 8} cells)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
