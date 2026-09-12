#!/usr/bin/env python3
"""b5_npc_proxy — procedural placeholder NPC sheet generator.

This produces valid sheet.png + sheet.json + palette.png + portrait.png
for each of the seven reserved B5 NPC folders (kinds 67..73). The output
satisfies the loader's hard rules (dirs==8, v1 schema, anchorY per draft)
and the QA gates (≤32 opaque colours, 1px outline, feet on anchor row).

Silhouettes are drawn procedurally with Pillow primitives (rectangles,
ellipses) in the BloodHollow palette language (mud/bone/soot/ember). They
are PLACEHOLDERS for the final B5 AI plates; when final art ships it
simply overwrites these PNGs with zero code changes.

Usage:
    python3 tools/atlaspack/b5_npc_proxy.py [--out assets/aigen/npcs]

Deterministic: same script version -> same bytes.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # reuse pipeline outline / dither primitives

# Direction order matches sim::kDx/kDy: E SE S SW W NW N NE (rows 0..7).
DIRS = ["E", "SE", "S", "SW", "W", "NW", "N", "NE"]

# Palette key. Each NPC gets a dict of these; bodies/cloaks/skins are swapped.
# Colours chosen to stay under luma 200 (per BRIEF) except for small accents.
PARCHMENT = (222, 205, 170, 255)
INK       = (40, 30, 32, 255)
OUTLINE   = (14, 10, 12, 255)
SHADOW    = (0, 0, 0, 70)
TRANSP    = (0, 0, 0, 0)


@dataclass
class NpcSpec:
    kind: int
    slug: str
    name: str
    cell: tuple[int, int]
    anchor_y: int
    palette: dict[str, tuple[int, int, int, int]]
    note: str = ""
    wide: bool = False  # 64-wide sheets (twins)
    # silhouette function: draw(ctx, cx, feet_y, dir_idx, frame)
    draw: object = field(default=None)


def _feet_y(cell_w: int, cell_h: int, anchor_y: int) -> int:
    """Foot-row Y (lowest opaque body row) == anchor_y per REGISTRY."""
    return anchor_y


# ---------------------------------------------------------------- drawing prims
def _ellipse(d: ImageDraw.ImageDraw, cx, cy, rx, ry, c, w=1):
    d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=c, outline=None, width=w)


def _rect(d, x0, y0, x1, y1, c):
    d.rectangle([x0, y0, x1, y1], fill=c)


def _outline_body(img: Image.Image) -> None:
    """Apply bhpix.outline(inside=True): recolours the outermost opaque ring to
    #1a1214. Does NOT grow the silhouette (so the lowest opaque body row stays
    exactly where the boot sole was drawn = anchor_y)."""
    outlined = P.outline(img, P.OUTLINE_RGB, inside=True)
    img.paste(outlined, (0, 0), outlined)


def _dither_band(img: Image.Image, x0, y0, x1, y1, dark, light) -> None:
    """Cheap ordered 2x2 Bayer dither for a band — gives the pixel-art look."""
    px = img.load()
    pat = [[0, 2], [3, 1]]
    for y in range(max(0, y0), min(img.size[1], y1)):
        for x in range(max(0, x0), min(img.size[0], x1)):
            r, g, b, a = px[x, y]
            if a < 16:
                continue
            if (r, g, b, a) == light:
                t = pat[(y - y0) % 2][(x - x0) % 2]
                if t >= 2:
                    px[x, y] = dark


def _shadow_ellipse(img, cx, cy, rx=9, ry=3):
    d = ImageDraw.Draw(img)
    _ellipse(d, cx, cy, rx, ry, SHADOW)


# ------------------------------------------------------------ silhouettes
def _draw_humanoid(img: Image.Image, cx, feet, facing: int, frame: int, pal, wide=False):
    """Draw a humanoid at (cx, feet) facing `facing` (0..7). frame = 0..3 idle bob.

    Feet are always painted down to row `feet` (anchor_y) — bob only moves the
    torso/head up, never pushes the sole off the anchor. Lowest opaque body
    pixel lands exactly on anchor_y to pass bh_qa_sheet feet_check (alpha 255).
    """
    d = ImageDraw.Draw(img)
    bob = (0, -1, 0, 0)[frame % 4]  # idle4 breath (frames 0/2 neutral, f1 up one)
    fdx = [1, 1, 0, -1, -1, -1, 0, 1][facing]
    fdy = [0, 1, 1, 1, 0, -1, -1, -1][facing]
    body = pal["body"]; trim = pal["trim"]; skin = pal["skin"]; hair = pal["hair"]
    cloak = pal.get("cloak", body)
    # legs: pants run from feet-8..feet-1 (soles = boot band ends ON feet row)
    lx = -5 if facing in (3, 4, 5) else (-4 if facing in (2, 6) else -3)
    rxo = -lx if facing in (0, 1, 7) else (0 if facing in (2, 6) else 3)
    _rect(d, int(cx) + lx, feet - 8, int(cx) + lx + 4, feet - 1, pal.get("pants", body))
    _rect(d, int(cx) + rxo, feet - 8, int(cx) + rxo + 4, feet - 1, pal.get("pants", body))
    # boot soles: full opacity 2px band ending on `feet` (lowest opaque body row = anchor_y)
    _rect(d, int(cx) + lx - 1, feet - 1, int(cx) + lx + 5, feet, pal.get("boot", OUTLINE))
    _rect(d, int(cx) + rxo - 1, feet - 1, int(cx) + rxo + 5, feet, pal.get("boot", OUTLINE))
    # torso (cloak) — bob shifts the torso/head only, not the boots
    tw = 12 if wide else 14
    _rect(d, int(cx) - tw // 2, feet - 24 + bob, int(cx) + tw // 2, feet - 9, cloak)
    # belt / trim
    _rect(d, int(cx) - tw // 2, feet - 13 + bob, int(cx) + tw // 2, feet - 11, trim)
    # head (facing offset pushes the face slightly forward)
    head_cx = int(cx) + fdx * 2
    head_cy = feet - 28 + bob + fdy
    _ellipse(d, head_cx, head_cy, 7, 7, skin)
    # hair / hood
    if pal.get("hood"):
        _ellipse(d, head_cx, head_cy - 2, 8, 7, pal["hood"])
        _rect(d, head_cx - 8, head_cy - 1, head_cx + 8, head_cy + 2, pal["hood"])
    else:
        _ellipse(d, head_cx, head_cy - 2, 7, 5, hair)
    # eye dots
    eye_col = OUTLINE
    if facing == 2:
        _rect(d, head_cx - 3, head_cy - 1, head_cx - 2, head_cy, eye_col)
        _rect(d, head_cx + 2, head_cy - 1, head_cx + 3, head_cy, eye_col)
    elif facing in (1, 3):
        ex = head_cx + (2 if facing == 1 else -2)
        _rect(d, ex, head_cy - 1, ex + 1, head_cy, eye_col)
    else:
        ex = head_cx + (1 if facing in (0, 7) else -1)
        _rect(d, ex, head_cy - 1, ex + 1, head_cy, eye_col)
    # arms (stubs at sides, bob with torso)
    arm_col = cloak
    _rect(d, int(cx) - tw // 2 - 1, feet - 22 + bob, int(cx) - tw // 2 + 1, feet - 13, arm_col)
    _rect(d, int(cx) + tw // 2 - 1, feet - 22 + bob, int(cx) + tw // 2 + 1, feet - 13, arm_col)


def _draw_hammer(img, cx, feet, side=-1, pal=None):
    """Small blacksmith's hammer held on one side (ends at feet, no bob)."""
    d = ImageDraw.Draw(img)
    bx = int(cx) + side * 10
    # handle descends to feet; head at the top
    _rect(d, bx, feet - 14, bx + 1, feet, pal.get("iron", (70, 72, 80, 255)))
    _rect(d, bx - 2, feet - 16, bx + 4, feet - 12, pal.get("iron", (70, 72, 80, 255)))


def _draw_tongs(img, cx, feet, side=1, pal=None):
    d = ImageDraw.Draw(img)
    bx = int(cx) + side * 10
    _rect(d, bx, feet - 13, bx + 1, feet, pal.get("iron", (70, 72, 80, 255)))
    # ember jaw tip
    _rect(d, bx - 1, feet - 1, bx + 2, feet, pal.get("ember", (200, 98, 42, 255)))


def _draw_anvil_between(img, cx, feet, pal):
    d = ImageDraw.Draw(img)
    # anvil base sitting ON the anchor row (bottom = feet)
    iron = pal.get("iron", (70, 72, 80, 255))
    _rect(d, int(cx) - 7, feet - 6, int(cx) + 7, feet, iron)
    _rect(d, int(cx) - 9, feet - 8, int(cx) + 9, feet - 6, iron)
    _rect(d, int(cx) - 3, feet - 11, int(cx) + 3, feet - 8, iron)
    # ember glow dot on top
    _rect(d, int(cx) - 1, feet - 12, int(cx) + 1, feet - 11, pal.get("ember", (200, 98, 42, 255)))


# ------------------------------------------------------------- NPC defs
def _twins_draw(img, cx, feet, facing, frame, pal):
    """Two sisters flanking a small anvil (64-wide cell)."""
    # Left sister (West half)
    _draw_humanoid(img, cx - 14, feet, facing, frame, pal, wide=False)
    _draw_hammer(img, cx - 14, feet, side=-1, pal=pal)
    # Right sister (East half)
    _draw_humanoid(img, cx + 14, feet, facing, frame, pal, wide=False)
    _draw_tongs(img, cx + 14, feet, side=1, pal=pal)
    # Anvil between them
    _draw_anvil_between(img, cx, feet, pal)


def _priest_draw(img, cx, feet, facing, frame, pal):
    _draw_humanoid(img, cx, feet, facing, frame, pal)
    d = ImageDraw.Draw(img)
    # cowl shadow over face (already drawn by hood=pal["hood"]); add a small
    # choir-gold inlay on the chest.
    _rect(d, int(cx) - 2, feet - 20 + (0, -1, 0, 1)[frame % 4], int(cx) + 2,
          feet - 17 + (0, -1, 0, 1)[frame % 4], pal.get("gold", (220, 180, 60, 255)))


def _fence_draw(img, cx, feet, facing, frame, pal):
    """Sable the fence — dark coat, red headscarf, small coin pouch."""
    _draw_humanoid(img, cx, feet, facing, frame, pal)
    d = ImageDraw.Draw(img)
    # coin pouch at belt
    _rect(d, int(cx) + 3, feet - 13, int(cx) + 6, feet - 10, pal.get("gold", (220, 180, 60, 255)))


def _guard_draw(img, cx, feet, facing, frame, pal):
    _draw_humanoid(img, cx, feet, facing, frame, pal)
    d = ImageDraw.Draw(img)
    # spear / halberd raised to the right side
    sx = int(cx) + 7
    _rect(d, sx, feet - 36, sx + 1, feet - 6, pal.get("iron", (70, 72, 80, 255)))
    _rect(d, sx - 2, feet - 38, sx + 3, feet - 35, pal.get("trim", (150, 40, 40, 255)))


def _registrar_draw(img, cx, feet, facing, frame, pal):
    _draw_humanoid(img, cx, feet, facing, frame, pal)
    d = ImageDraw.Draw(img)
    # parchment + quill in the left hand
    _rect(d, int(cx) - 11, feet - 20, int(cx) - 5, feet - 15, PARCHMENT)
    _rect(d, int(cx) - 10, feet - 22, int(cx) - 9, feet - 20, INK)


def _steward_draw(img, cx, feet, facing, frame, pal):
    _draw_humanoid(img, cx, feet, facing, frame, pal)
    d = ImageDraw.Draw(img)
    # key ring at belt
    _ellipse(d, int(cx) + 5, feet - 11, 2, 2, pal.get("gold", (220, 180, 60, 255)))
    _rect(d, int(cx) + 4, feet - 11, int(cx) + 5, feet - 8, pal.get("gold", (220, 180, 60, 255)))


NPCS: list[NpcSpec] = [
    # Palettes tuned so body mean luma clears R-LUMA (≥25 over terrain-luma=51
    # day; ≥15 at 02:00 night overlay). Era-muted but readable; final B5 art
    # will overwrite the PNGs (palette kept per BRIEF).
    NpcSpec(67, "bonesmith_twins", "Bonesmith Twins", (64, 48), 42,
            {"body": (130, 104, 96, 255), "cloak": (110, 92, 86, 255),
             "hood": (58, 44, 44, 255), "trim": (240, 150, 58, 255),
             "skin": (216, 196, 164, 255), "hair": (48, 36, 36, 255),
             "iron": (140, 144, 156, 255), "ember": (240, 150, 58, 255),
             "pants": (76, 62, 56, 255), "boot": (40, 28, 26, 255)},
            note="two soot-veiled sisters flanking anvil (ember rim-lit)",
            wide=True, draw=_twins_draw),
    NpcSpec(68, "confessor", "Confessor", (32, 48), 42,
            {"body": (96, 94, 142, 255), "cloak": (78, 76, 122, 255),
             "hood": (50, 42, 72, 255), "trim": (230, 200, 110, 255),
             "skin": (228, 212, 196, 255), "hair": (56, 42, 48, 255),
             "gold": (240, 205, 80, 255),
             "pants": (60, 56, 88, 255), "boot": (30, 24, 28, 255)},
            note="chapel penitent priest, choir-gold inlay", draw=_priest_draw),
    NpcSpec(69, "cove_fence", "Sable (Cove Fence)", (32, 48), 42,
            {"body": (130, 82, 68, 255), "cloak": (108, 66, 54, 255),
             "hood": (176, 54, 48, 255),  # red headscarf
             "trim": (210, 162, 76, 255),
             "skin": (216, 178, 142, 255), "hair": (176, 54, 48, 255),
             "gold": (240, 205, 80, 255),
             "pants": (74, 52, 42, 255), "boot": (42, 28, 22, 255)},
            note="smuggler's fence, dark coat, red scarf, coin pouch",
            draw=_fence_draw),
    NpcSpec(70, "guard_ashen", "Ashen Guard", (32, 48), 42,
            {"body": (112, 112, 120, 255), "cloak": (86, 84, 94, 255),
             "hood": None, "trim": (170, 50, 46, 255),
             "skin": (216, 196, 176, 255), "hair": (72, 60, 56, 255),
             "iron": (120, 126, 140, 255),
             "pants": (70, 70, 80, 255), "boot": (36, 30, 32, 255)},
            note="Ashen-cape town guard with spear", draw=_guard_draw),
    NpcSpec(71, "guard_synod", "Synod Guard", (32, 48), 42,
            {"body": (86, 90, 140, 255), "cloak": (72, 74, 122, 255),
             "hood": None, "trim": (220, 190, 80, 255),
             "skin": (216, 196, 176, 255), "hair": (54, 52, 80, 255),
             "iron": (120, 126, 150, 255),
             "pants": (58, 58, 96, 255), "boot": (28, 26, 40, 255)},
            note="Synod-blue town guard with spear", draw=_guard_draw),
    NpcSpec(72, "pledge_registrar", "Pledge Registrar", (32, 48), 42,
            {"body": (112, 94, 70, 255), "cloak": (96, 76, 54, 255),
             "hood": None, "trim": (200, 160, 70, 255),
             "skin": (226, 210, 186, 255), "hair": (100, 72, 46, 255),
             "pants": (70, 56, 40, 255), "boot": (34, 24, 18, 255)},
            note="quill-and-parchment clerk", draw=_registrar_draw),
    NpcSpec(73, "castle_steward", "Castle Steward", (32, 48), 42,
            {"body": (100, 84, 66, 255), "cloak": (86, 70, 54, 255),
             "hood": None, "trim": (224, 186, 84, 255),
             "skin": (216, 196, 176, 255), "hair": (66, 52, 36, 255),
             "gold": (230, 195, 70, 255),
             "pants": (62, 48, 36, 255), "boot": (30, 22, 18, 255)},
            note="keeper of keys, gold-trimmed tabard", draw=_steward_draw),
]


# ---------------------------------------------------------- sheet assembly
def make_sheet(spec: NpcSpec) -> tuple[Image.Image, dict]:
    cw, ch = spec.cell
    cols = 4  # idle4
    rows = 8
    sheet = Image.new("RGBA", (cw * cols, ch * rows), TRANSP)
    for row in range(rows):
        for col in range(cols):
            cell = Image.new("RGBA", (cw, ch), TRANSP)
            cx = cw // 2
            feet = _feet_y(cw, ch, spec.anchor_y)
            spec.draw(cell, cx, feet, row, col, spec.palette)
            # rim light for readability (REGISTRY: bone/ember 1px on lit side),
            # then inside-only outline (recolours outermost opaque ring, no
            # growth), then contact shadow (alpha 70) last.
            rim_rgb = spec.palette.get("rim", spec.palette.get("skin", (201, 191, 174, 255)))
            strength = 2 if spec.kind in (67, 69) else 1
            cell = P.rim_light(cell, rim_rgb[:3], side="NW", strength=strength)
            _outline_body(cell)
            _shadow_ellipse(cell, cx, feet + 2, rx=9 if cw >= 64 else 7, ry=3)
            sheet.paste(cell, (col * cw, row * ch), cell)
    meta = {
        "anims": {
            "idle": {
                "frameW": cw,
                "frameH": ch,
                "frames": cols,
                "dirs": rows,
                "fps": 4,
                "offsetX": 0,
                "offsetY": 0,
                "anchorY": spec.anchor_y,
            }
        },
        "_b5_proxy": {
            "kind": spec.kind,
            "name": spec.name,
            "note": spec.note,
            "generator": "tools/atlaspack/b5_npc_proxy.py (procedural placeholder; final B5 AI plates overwrite these PNGs)",
            "date": "2026-09-12",
        },
    }
    return sheet, meta


def make_palette_strip(spec: NpcSpec, sheet: Image.Image) -> Image.Image:
    """Extract up to 32 unique opaque colours from the sheet; tile as a strip."""
    a = np.asarray(sheet.convert("RGBA"))
    mask = a[..., 3] > 0
    if not mask.any():
        colours = []
    else:
        # sort by (luma, r, g, b) for deterministic ordering
        px = a[mask]
        lum = (px[:, 0].astype(np.int32) * 30 + px[:, 1] * 59 + px[:, 2] * 11) // 100
        order = np.lexsort((px[:, 2], px[:, 1], px[:, 0], lum))
        colours = [tuple(px[i]) for i in order]
    # dedupe near-identical (quantize to 5-bit channels) to stay <= 32
    seen = set()
    kept = []
    for c in colours:
        key = (c[0] >> 3, c[1] >> 3, c[2] >> 3, c[3] >> 3)
        if key in seen:
            continue
        seen.add(key); kept.append(c)
        if len(kept) >= 32:
            break
    sw = 8
    strip = Image.new("RGBA", (sw * len(kept), sw), TRANSP)
    d = ImageDraw.Draw(strip)
    for i, c in enumerate(kept):
        d.rectangle([i * sw, 0, (i + 1) * sw - 1, sw - 1], fill=c)
    return strip


def make_portrait(spec: NpcSpec) -> Image.Image:
    """96x96 bust portrait in matching palette, on near-black background."""
    img = Image.new("RGBA", (96, 96), (12, 10, 14, 255))
    d = ImageDraw.Draw(img)
    cx, cy = 48, 50
    pal = spec.palette
    # shoulders / cloak
    _ellipse(d, cx, cy + 34, 36, 20, pal.get("cloak", pal["body"]))
    _rect(d, cx - 36, cy + 24, cx + 36, cy + 40, pal.get("cloak", pal["body"]))
    # neck
    _rect(d, cx - 5, cy + 10, cx + 5, cy + 22, pal["skin"])
    # head
    _ellipse(d, cx, cy + 2, 18, 20, pal["skin"])
    # hair/hood
    if pal.get("hood"):
        _ellipse(d, cx, cy - 4, 20, 16, pal["hood"])
        _rect(d, cx - 20, cy - 4, cx + 20, cy + 6, pal["hood"])
    else:
        _ellipse(d, cx, cy - 4, 18, 12, pal["hair"])
    # eyes
    _rect(d, cx - 7, cy + 2, cx - 5, cy + 4, OUTLINE)
    _rect(d, cx + 5, cy + 2, cx + 7, cy + 4, OUTLINE)
    # mouth
    _rect(d, cx - 3, cy + 12, cx + 3, cy + 13, (80, 40, 40, 255))
    # small trim accent at throat
    _rect(d, cx - 3, cy + 20, cx + 3, cy + 22, pal["trim"])
    # frame 1px
    d.rectangle([0, 0, 95, 95], outline=OUTLINE)
    return img


def qa_check(spec: NpcSpec, sheet: Image.Image) -> list[str]:
    """Reproduce the bh_qa_sheet --kind cell gates; return a list of warnings."""
    cw, ch = spec.cell
    out: list[str] = []
    # colour count (alpha==255 body pixels — matches bhpix.count_colours semantics)
    a = np.asarray(sheet.convert("RGBA"))
    body = a[(a[..., 3] == 255) & ~((a[..., 0] == P.OUTLINE_RGB[0]) & (a[..., 1] == P.OUTLINE_RGB[1]) & (a[..., 2] == P.OUTLINE_RGB[2]))]
    n_col_body = len(np.unique(body.reshape(-1, 4), axis=0)) if len(body) else 0
    n_col_total = P.count_colours(sheet)
    if n_col_total > 32:
        out.append(f"FAIL colour count {n_col_total} > 32 (body {n_col_body})")
    # per-cell feet-row check, mirroring bh_qa_sheet.feet_check: only alpha==255
    # pixels count (alpha-70 contact shadow excluded); lowest such pixel must
    # be within 2 px of anchor_y.
    arr = np.asarray(sheet.convert("RGBA"))
    fails = 0
    for row in range(8):
        for col in range(4):
            cell = arr[row * ch:(row + 1) * ch, col * cw:(col + 1) * cw]
            rows_full = np.where((cell[..., 3] == 255).any(axis=1))[0]
            if len(rows_full) == 0:
                fails += 1; continue
            lowest = int(rows_full.max())
            if abs(lowest - spec.anchor_y) > 2:
                fails += 1
    if fails:
        out.append(f"FAIL feet-row anchor on {fails} cells (expected ~{spec.anchor_y})")
    # outline: count OUTLINE_RGB alpha-255 pixels (should be present on every cell)
    out_mask = (a[..., 3] == 255) & (a[..., 0] == P.OUTLINE_RGB[0]) & (a[..., 1] == P.OUTLINE_RGB[1]) & (a[..., 2] == P.OUTLINE_RGB[2])
    outline_px = int(out_mask.sum())
    if outline_px < 8:
        out.append(f"WARN too few outline pixels ({outline_px})")
    out.append(f"OK {n_col_total} opaque colours (body {n_col_body}), {outline_px} outline px")
    return out


# -------------------------------------------------------------- main
def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="assets/aigen/npcs")
    ap.add_argument("--root", default=".")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    out_root = (root / args.out).resolve()
    print(f"[b5] writing NPC sheets under {out_root}")

    for spec in NPCS:
        folder = out_root / spec.slug
        folder.mkdir(parents=True, exist_ok=True)
        sheet, meta = make_sheet(spec)
        pal_strip = make_palette_strip(spec, sheet)
        portrait = make_portrait(spec)

        sheet_path = folder / "sheet.png"
        json_path = folder / "sheet.json"
        pal_path = folder / "palette.png"
        port_path = folder / "portrait.png"

        sheet.save(sheet_path, "PNG", optimize=True)
        pal_strip.save(pal_path, "PNG", optimize=True)
        portrait.save(port_path, "PNG", optimize=True)
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(meta, f, indent=1, sort_keys=False)
            f.write("\n")

        msgs = qa_check(spec, sheet)
        for m in msgs:
            print(f"  [{spec.kind:02d} {spec.slug:<20}] {m}")
        if any(x.startswith("FAIL") for x in msgs):
            print(f"  *** QA FAIL for {spec.slug}", file=sys.stderr)
            return 2
    print("[b5] all sheets written, QA green.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
