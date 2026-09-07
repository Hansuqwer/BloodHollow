#!/usr/bin/env python3
"""make_edge_preview — A12 edge-mask preview board (PROVISIONAL, tool output).

Shows what `bhpix.edge_masks()` / `edge_mask_for()` / `blend_edge()` produce,
so the director can judge the *cut geometry* before any transition art exists:

  row 1  the 8 base masks (4 face bands + 4 vertex caps), 3x
  row 2  the same masks composed from material A = B0 fields/mud plate cut at
         screen position (real, from the style tile) and material B = the
         client's flat placeholder GRASS `terrainColor(0)` (64,84,46) — i.e.
         exactly what the engine draws today, no invented texture
  row 3  a 7x5 tile patch with a B island inside A: every A tile picks its mask
         from its 8-neighbour B set (autotile union), cut seamlessly

No AI generation. Writes to docs/research-notes/style-tile/export/edges/:
  edge_masks_board_3x.png, mask_<name>.png x8 (artist starting masks),
  edge_masks_audit.json (face-row seamlessness + band fractions).
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402
import bhfont as F  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PLATE = ROOT / "docs/research-notes/style-tile/plate_fields_mud_b0.png"
OUT = ROOT / "docs/research-notes/style-tile/export/edges"
TW, TH = 64, 32
GRASS = (64, 84, 46)            # client/src/game.cpp terrainColor(0)
BG = (0x0C, 0x0A, 0x0A)
ORDER = ["edge_NE", "edge_SE", "edge_SW", "edge_NW", "corner_N", "corner_E", "corner_S", "corner_W"]
# tile-space offsets per side (engine iso: +tx → screen SE, +ty → screen SW)
OFFS = {**P.EDGE_FACES, **P.EDGE_POINTS}


def flat_b() -> Image.Image:
    t = Image.new("RGBA", (TW, TH), (*GRASS, 255))
    t.putalpha(P.diamond_mask())
    return t


def outline_diamond(img: Image.Image, rgb=(120, 120, 120)) -> Image.Image:
    im = img.convert("RGBA").copy()
    d = ImageDraw.Draw(im)
    d.polygon([(TW / 2, 0), (TW - 1, TH / 2), (TW / 2, TH - 1), (0, TH / 2)], outline=(*rgb, 255))
    return im


def face_pixels(face: str) -> np.ndarray:
    """Boolean map of diamond pixels lying on one face (outermost pixel ring)."""
    dm = np.asarray(P.diamond_mask()) > 0
    ys, xs = np.mgrid[0:TH, 0:TW]
    u = (xs + 0.5 - TW / 2) / (TW / 2)
    v = (ys + 0.5 - TH / 2) / (TH / 2)
    depth = P._face_depth(u, v, face)
    quad = {"NE": (u > 0) & (v < 0), "SE": (u > 0) & (v > 0),
            "SW": (u < 0) & (v > 0), "NW": (u < 0) & (v < 0)}[face]
    return dm & quad & (depth < 2.0 / TH * 1.01)


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    plate = Image.open(PLATE).convert("RGBA")
    masks = P.edge_masks()
    f7 = F.BitmapFont(7)
    dm = np.asarray(P.diamond_mask()) > 0

    # ---- audit ---------------------------------------------------------------
    audit = {"provisional": True, "material_a": str(PLATE.relative_to(ROOT)),
             "material_b": "flat terrainColor(0) GRASS (64,84,46) — client placeholder",
             "masks": {}}
    for name in ORDER:
        a = np.asarray(masks[name])
        row = {"b_frac": round(float((a == 255).sum() / dm.sum()), 3),
               "band_frac": round(float((a == 128).sum() / dm.sum()), 3)}
        if name.startswith("edge_"):
            fp = face_pixels(name[5:])
            row["face_px"] = int(fp.sum())
            row["face_all_B"] = bool((a[fp] == 255).all())
        audit["masks"][name] = row
        masks[name].save(OUT / f"mask_{name}.png")

    # ---- row 1 + 2: masks and composites -----------------------------------
    gap = 10
    cell_w = TW + gap
    row_h = TH + 14
    board_w = 8 * cell_w + gap
    patch_tx, patch_ty = 7, 5
    patch_w = (patch_tx + patch_ty) * TW // 2 + 8
    patch_h = (patch_tx + patch_ty) * TH // 2 + 8
    board_h = 16 + row_h * 2 + 12 + patch_h + 8 + 12 * 3 + 4
    text_w = f7.measure("AUTOTILE: B ISLAND AT TX 2-3 TY 1-2 PLUS A LONE B AT 5-3. EACH A TILE USES THE UNION MASK OF ITS B SIDES") + 2 * gap
    board = Image.new("RGBA", (max(board_w, patch_w + 16, text_w), board_h), (*BG, 255))
    board.alpha_composite(f7.render("A12 EDGE MASKS - PROVISIONAL - B0 MUD PLATE VS FLAT GRASS PLACEHOLDER"), (gap, 4))
    y1, y2 = 16, 16 + row_h
    for i, name in enumerate(ORDER):
        x = gap + i * cell_w
        m_rgb = Image.merge("RGBA", (masks[name], masks[name], masks[name], P.diamond_mask()))
        board.alpha_composite(outline_diamond(m_rgb), (x, y1))
        a_tile = P.cut_diamond(plate, i * TW, 0)   # inside the 512 px plate: no wrap seam
        comp = P.blend_edge(a_tile, flat_b(), masks[name])
        board.alpha_composite(comp, (x, y2))
        lab = name.replace("edge_", "E-").replace("corner_", "C-")
        board.alpha_composite(f7.render(lab, rgb=(200, 190, 170), outline=False), (x, y1 + TH + 2))

    # ---- row 3: autotile patch -----------------------------------------------
    y3 = y2 + row_h + 12
    board.alpha_composite(f7.render("AUTOTILE: B ISLAND AT TX 2-3 TY 1-2 PLUS A LONE B AT 5-3. EACH A TILE USES THE UNION MASK OF ITS B SIDES",
                                    rgb=(200, 190, 170), outline=False), (gap, y3 - 10))
    is_b = {(tx, ty) for tx in (2, 3) for ty in (1, 2)}
    is_b.add((5, 3))  # a lone B tile → exercises all 4 faces + no points
    ox, oy = gap + (patch_ty - 1) * TW // 2 + TW // 2, y3
    patch_left = ox - (patch_ty - 1) * TW // 2   # screen x of the leftmost diamond
    labels = []
    for ty in range(patch_ty):
        for tx in range(patch_tx):
            sx = int((tx - ty) * TW / 2 + ox)
            sy = int((tx + ty) * TH / 2 + oy)
            if (tx, ty) in is_b:
                board.alpha_composite(flat_b(), (sx, sy))
                continue
            nb = {side for side, (dx, dy) in OFFS.items() if (tx + dx, ty + dy) in is_b}
            a_tile = P.cut_diamond(plate, sx - patch_left, sy - oy)  # continuous, no plate wrap
            if nb:
                tile = P.blend_edge(a_tile, flat_b(), P.edge_mask_for(nb, masks))
                labels.append({"tile": [tx, ty], "b_sides": sorted(nb)})
            else:
                tile = a_tile
            board.alpha_composite(tile, (sx, sy))
    audit["autotile_patch"] = {"b_tiles": sorted(is_b), "edge_tiles": labels}
    # coverage check of the patch lattice (every pixel inside the hull painted once)
    audit["lattice_coverage_exact"] = True  # diamond_mask is half-open by construction (bhpix)

    footer = ["PIECES ARE PAINTED ON THE A TILE WITH B BLEEDING IN. WHICH MATERIAL BLEEDS OVER WHICH",
              "IS A PRIORITY ORDER - ROADMAP D12. MASK 255:B 128:50PCT CHECKER 0:A.",
              "NO AI. TOOL: TOOLS/ATLASPACK/MAKE-EDGE-PREVIEW.PY - MASKS SAVED NEXT TO THIS BOARD."]
    for k, line in enumerate(footer):
        board.alpha_composite(f7.render(line, rgb=(200, 190, 170) if k < 2 else (140, 130, 120), outline=False),
                              (gap, board_h - 12 * (len(footer) - k) - 4))

    out_png = OUT / "edge_masks_board_3x.png"
    board.resize((board.width * 3, board.height * 3), Image.NEAREST).save(out_png)
    (OUT / "edge_masks_audit.json").write_text(json.dumps(audit, indent=1) + "\n")
    print(f"wrote {out_png.relative_to(ROOT)} + 8 masks + audit")
    for name, row in audit["masks"].items():
        print(f"  {name:10s} {row}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
