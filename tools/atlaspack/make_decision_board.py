#!/usr/bin/env python3
"""A16 — decision-input board for the four open B0 questions (D1–D4) plus
the D5 elite-cell contradiction, rendered from the FOUR EXISTING B0 plates
(no new generation). Output is PROVISIONAL and does not sign the gate.

    python3 tools/atlaspack/make_decision_board.py
      -> docs/research-notes/style-tile/export/decision_board_3x.png
         docs/research-notes/style-tile/export/decision_board_audit.json

Rows:
  D1  Ravager rim OFF | rim ON (bhpix.rim_light, key light NW)     day / night
  D2  body 46 px in 32x48 | body 52 px in 32x56 (anchorY 50)       day / grey
  D3  terrain floor lift 0 | +15 | +30 (plate re-processed, sprites unchanged)
  D4  callout font cap 7 | cap 11 over the Ravager                  day / night
  D5  Sepulcher Elite stand-in: Ravager at 1.25x in 40x60 | 48x64 (scale only)
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402
import bhscene as S  # noqa: E402
import bhfont as F  # noqa: E402
import make_style_tile as M  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PLATES = ROOT / "docs/research-notes/style-tile/plates"
OUT = ROOT / "docs/research-notes/style-tile/export"


def plate_with_floor(ground_raw: Image.Image, pal: np.ndarray, lift: float) -> Image.Image:
    """Same chain as make_style_tile.make_ground_tiles, with an extra floor lift."""
    small = ground_raw.convert("RGB").resize((512, 280), Image.BOX).filter(ImageFilter.GaussianBlur(0.8))
    small = ImageEnhance.Contrast(small).enhance(0.70)
    arr = np.asarray(small).astype(np.float32)
    arr = np.clip(arr * 0.95 + 14.0 + lift, 0, 255)
    small = Image.fromarray(arr.astype(np.uint8), "RGB").convert("RGBA")
    return P.quantize(small, pal, dither="bayer2", strength=0.05)


def sprite(path: Path, target_h: int, pal: np.ndarray, cell, feet_y, **kw) -> Image.Image:
    raw = Image.open(path)
    k = P.crop_to_alpha(P.harden_alpha(P.key_out_green(raw)))
    s = P.harden_alpha(P.fit_to_cell(k, target_h=target_h))
    sa = np.asarray(s).astype(np.float32)
    lin = (sa[..., :3] / 255.0) ** kw.get("gamma", 0.66)
    lin = np.clip((lin - 0.5) * kw.get("contrast", 1.15) + 0.5, 0, 1)
    sa[..., :3] = lin * 255.0
    s = P.outline(P.quantize(Image.fromarray(sa.astype(np.uint8), "RGBA"), pal, dither="bayer2", strength=0.10))
    return P.place_in_cell(s, cell=cell, feet_y=feet_y, shadow_rx=kw.get("shadow_rx", 8))


def panel(img: Image.Image, label: str, scale: int = 3) -> Image.Image:
    up = P.upscale(img, scale)
    p = Image.new("RGBA", (up.width, up.height + 16), (0x0C, 0x0A, 0x0A, 255))
    p.alpha_composite(up, (0, 16))
    ImageDraw.Draw(p).text((3, 2), label, fill=(0xE6, 0xD2, 0xBE, 255))
    return p


def row(panels: list[Image.Image], title: str) -> Image.Image:
    w = sum(p.width for p in panels) + 8 * (len(panels) - 1)
    h = max(p.height for p in panels) + 18
    r = Image.new("RGBA", (w, h), (0x0C, 0x0A, 0x0A, 255))
    ImageDraw.Draw(r).text((2, 2), title, fill=(255, 200, 120, 255))
    x = 0
    for p in panels:
        r.alpha_composite(p, (x, 18)); x += p.width + 8
    return r


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    rav_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(PLATES / "ravager_4x_raw.png"))))
    rat_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(PLATES / "marsh_rat_4x_raw.png"))))
    ground_raw = Image.open(PLATES / "fields_ground_4x_raw.png").convert("RGBA")
    tree_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(PLATES / "dead_tree_4x_raw.png"))))
    pal_ply = P.build_palette([rav_raw.resize((rav_raw.width // 4, rav_raw.height // 4), Image.BOX)], 32)
    pal_mob = P.build_palette([rat_raw.resize((rat_raw.width // 4, rat_raw.height // 4), Image.BOX)], 32)
    pal_ter = P.build_palette([ground_raw.resize((352, 192), Image.BOX),
                               tree_raw.resize((tree_raw.width // 4, tree_raw.height // 4), Image.BOX)], 32)

    plate0 = plate_with_floor(ground_raw, pal_ter, 0.0)
    rav48 = sprite(PLATES / "ravager_4x_raw.png", 46, pal_ply, (32, 48), 42)
    rav48_rim = P.rim_light(rav48, side="NW")
    rav56 = sprite(PLATES / "ravager_4x_raw.png", 52, pal_ply, (32, 56), 50)
    rat = sprite(PLATES / "marsh_rat_4x_raw.png", 20, pal_mob, (32, 48), 42, gamma=0.80, contrast=1.0, shadow_rx=10)
    audit: dict = {"provisional": True, "signs_gate": False, "terrain_luma_plate0": round(P.luma_mean(plate0, body_only=False, exclude_outline=False), 1)}

    def scene(ents, plate, w=192, h=160, hour=None, fx=None):
        sc = S.Scene(w=w, h=h, tiles_x=5, tiles_y=5, plate=plate)
        sc.ents = ents
        if fx:
            sc.fx = fx
        return sc.render(hour=hour)

    # ---- D1 rim off/on (day + night)
    d1 = []
    for lbl, cell in (("D1a rim OFF", rav48), ("D1b rim ON (1px bone, NW)", rav48_rim)):
        ents = [S.Ent(cell, 2.0, 2.2, name="Ravager L7"), S.Ent(rat, 3.0, 1.6, name="Marsh Rat"), S.Ent(cell, 1.0, 3.0)]
        d1.append(panel(scene(ents, plate0), lbl + " day"))
        d1.append(panel(scene(ents, plate0, hour=2.0), lbl + " night 02:00"))
        audit[lbl] = {"body_luma": round(P.luma_mean(cell), 1),
                      "delta_vs_plate": round(P.luma_mean(cell) - audit["terrain_luma_plate0"], 1)}
    board_rows = [row(d1, "D1  Ravager R-LUMA margin: rim off vs on  (rulebook fix step 3; sprites otherwise identical)")]

    # ---- D2 cell 48 vs 56
    d2 = []
    for lbl, cell, ay in (("D2a 32x48 body 46 (1.45 tile-h)", rav48, 42), ("D2b 32x56 body 52 (1.63 tile-h, anchorY 50)", rav56, 50)):
        ents = [S.Ent(cell, 2.0, 2.2, anchor_y=ay, name="Ravager L7"), S.Ent(rat, 3.0, 1.6, name="Marsh Rat"), S.Ent(cell, 1.0, 3.0, anchor_y=ay)]
        day = scene(ents, plate0)
        d2.append(panel(day, lbl))
        d2.append(panel(P.greyscale(day), lbl + " grey"))
        audit[lbl] = {"cell": cell.size, "colours": P.count_colours(cell)}
    board_rows.append(row(d2, "D2  Player cell: 32x48 vs 32x56  (56 needs T-ART-10 anchorY; HB≈1.7, Soma≈2.4 tile-heights)"))

    # ---- D3 terrain floor lift
    d3 = []
    for lift in (0, 15, 30):
        pl = plate_with_floor(ground_raw, pal_ter, float(lift))
        ents = [S.Ent(rav48, 2.0, 2.2, name="Ravager L7"), S.Ent(rat, 3.0, 1.6, name="Marsh Rat")]
        tl = P.luma_mean(pl, body_only=False, exclude_outline=False)
        lbl = f"D3 floor +{lift}  terrain luma {tl:.0f}  dRav {P.luma_mean(rav48) - tl:+.0f}"
        d3.append(panel(scene(ents, pl), lbl))
        d3.append(panel(scene(ents, pl, hour=2.0), f"+{lift} night"))
        audit[f"D3 lift {lift}"] = {"terrain_luma": round(tl, 1), "ravager_delta": round(P.luma_mean(rav48) - tl, 1),
                                    "rat_delta": round(P.luma_mean(rat) - tl, 1),
                                    "plate_min_luma": round(float(np.asarray(pl.convert("L")).min()), 1)}
    board_rows.append(row(d3, "D3  Terrain floor: current (≈51, Vessalia-dark) vs +15 vs +30 (toward L1 Tower ≈95)"))

    # ---- D4 callout font cap 7 vs 11 (day + night, with plate on night)
    d4 = []
    for cap in (7, 11):
        f = F.BitmapFont(cap)
        for hour in (None, 2.0):
            ents = [S.Ent(rav48, 2.0, 2.2, name="Ravager L7"), S.Ent(rat, 3.0, 1.6, name="Marsh Rat")]
            sc = S.Scene(w=192, h=160, tiles_x=5, tiles_y=5, plate=plate0)
            sc.ents = ents
            day = sc.render()
            ax, ay = sc.anchor(2.0, 2.2)
            txt = f.render("POWER-SWING!", plate=(hour is not None))
            day.alpha_composite(txt, (ax - txt.width // 2, ay - 62 - (txt.height - 9)))
            if hour is not None:
                day = P.night_floor(day, hour)
            d4.append(panel(day, f"D4 cap {cap}px" + (" night+plate" if hour else " day")))
    board_rows.append(row(d4, "D4  Callout font: 5x7 (parity w/ raylib default @10) vs 7x11 (R-TEXT / HB scale); night uses the 2px plate"))

    # ---- D5 elite cell stand-in (scale only; the real elite is a Sexton variant)
    d5 = []
    for lbl, cell_sz, body, ay in (("D5a 40x60 (1.25x) anchorY 52", (40, 60), 58, 52), ("D5b 48x64 (T-ART-10 text) anchorY 56", (48, 64), 62, 56)):
        elite = sprite(PLATES / "ravager_4x_raw.png", body, pal_ply, cell_sz, ay, shadow_rx=10)
        ents = [S.Ent(rav48, 1.2, 2.8, name="Ravager L7"), S.Ent(elite, 2.4, 1.8, anchor_y=ay, name="Sepulcher Elite", name_rgb=S.CHAOTIC_NAME), S.Ent(rat, 3.4, 2.6)]
        day = scene(ents, plate0)
        d5.append(panel(day, lbl))
        d5.append(panel(P.greyscale(day), lbl + " grey"))
        audit[lbl] = {"cell": elite.size, "body_px": body}
    board_rows.append(row(d5, "D5  Elite cell contradiction: docs/art/20-mobs.md 40x60 vs T-ART-10 card 48x64 (stand-in body = Ravager scaled)"))

    # ---- assemble
    W = max(r.width for r in board_rows) + 16
    H = sum(r.height for r in board_rows) + 12 * len(board_rows) + 40
    board = Image.new("RGBA", (W, H), (0x0C, 0x0A, 0x0A, 255))
    d = ImageDraw.Draw(board)
    d.text((8, 6), "BLOODHOLLOW B0 decision board — PROVISIONAL, does not sign the gate. Rendered from the 4 existing B0 plates via tools/atlaspack (deterministic).", fill=(0xE6, 0xD2, 0xBE, 255))
    d.text((8, 20), "Night = engine daynight.cpp overlay at 02:00 (18,22,70,a145). All cells <=32 colours. R-LUMA numbers exclude the outline (rulebook definition).", fill=(0xA0, 0x98, 0x90, 255))
    y = 40
    for r in board_rows:
        board.alpha_composite(r, (8, y)); y += r.height + 12
    board.convert("RGB").save(OUT / "decision_board_3x.png")
    (OUT / "decision_board_audit.json").write_text(json.dumps(audit, indent=1) + "\n")
    print(json.dumps(audit, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
