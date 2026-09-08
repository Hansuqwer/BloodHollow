#!/usr/bin/env python3
"""B4 boss_occupancy gate — the B0 study (make_crowd_test.py) re-run with the REAL
B4 cells instead of the ravager stand-ins.

Scene = drowned_crypt spawn rects (read-only numbers from the B0 audit):
  gravemother_font x21-25 y2-5 · apse_sexton x21-24 y5-8 · elite_apse_l x16-20 y4-7 ·
  elite_apse_r x25-29 y4-7.  Five B0 ravager cells stand in for the player pile
  (players are B6 — not generated yet).

Gates (same as B0): no entity > 50 % hidden; boss max_hidden reported; PLUS the
bell rule from 20-mobs.md — the lower 16 px of the boss cell (rows 43..58) must
be low-detail so player heads read against the dome: we measure the colour count
and the luma std-dev of those rows and require ≤ 6 colours (excl. outline/shadow)
and std-dev ≤ 22.

Outputs docs/research-notes/qa/b4_boss_occupancy_3x.png + _audit.json.
Offline only — UNVALIDATED in engine.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).parent))
import bhscene as S  # noqa: E402
import bhfont as F  # noqa: E402
import make_crowd_test as C  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
QA = ROOT / "docs/research-notes/qa"
ST = ROOT / "docs/research-notes/style-tile"
MOBS = ROOT / "assets/aigen/mobs"


def cell(mob: str, name: str) -> Image.Image:
    return Image.open(MOBS / mob / "cells" / f"{name}.png").convert("RGBA")


def bell_lower_16(boss_cell: Image.Image, anchor_y: int = 58) -> dict:
    a = np.asarray(boss_cell)
    band = a[anchor_y - 15: anchor_y + 1]
    body = band[(band[..., 3] == 255)]
    body = body[~((body[:, 0] == 0x1A) & (body[:, 1] == 0x12) & (body[:, 2] == 0x14))]
    if len(body) == 0:
        return {"colours": 0, "luma_std": 0.0}
    cols = {tuple(int(v) for v in c[:3]) for c in body}
    lum = 0.299 * body[:, 0] + 0.587 * body[:, 1] + 0.114 * body[:, 2]
    return {"colours": len(cols), "luma_std": round(float(lum.std()), 1), "rows": [anchor_y - 15, anchor_y]}


def main() -> int:
    plate = Image.open(ST / "plate_fields_mud_b0.png").convert("RGBA")
    rav = Image.open(ST / "cell_ravager_S.png").convert("RGBA")
    boss = cell("1009_gravemother", "walk_S_0")
    boss_atk = cell("1009_gravemother", "attack_S_1")
    elite = cell("1010_sepulcher_elite", "walk_S_0")
    sexton = cell("1008_revenant_sexton", "walk_S_0")
    celebrant = cell("1007_gravecaller", "walk_S_0")
    f7 = F.BitmapFont(7)

    sc = S.Scene(w=352, h=256, tiles_x=9, tiles_y=9, plate=plate, origin=(176 - 32, 8), plate_fallback=(60, 58, 66))
    ox_t, oy_t = 17, 1

    def T(x, y):
        return (x - ox_t, y - oy_t)

    ents = [S.Ent(boss, *T(23.0, 3.5), anchor_y=58, name="The Gravemother", name_rgb=S.CHAOTIC_NAME, hp=0.9),
            S.Ent(elite, *T(18.0, 5.5), anchor_y=52, name="Sepulcher Elite", hp=1.0),
            S.Ent(elite, *T(27.0, 5.5), anchor_y=52, name="Sepulcher Elite", hp=1.0),
            S.Ent(sexton, *T(22.5, 6.5), name="Revenant Sexton", hp=1.0),
            S.Ent(celebrant, *T(24.5, 5.2), name="Waxen Celebrant", hp=1.0)]
    for i, (dx, dy) in enumerate(((0.0, 0.0), (0.7, 0.4), (-0.6, 0.5), (0.3, 1.0), (-0.3, 1.2))):
        ents.append(S.Ent(rav, *T(22.0 + dx, 7.2 + dy), name=["Ravager", "Cultist", "Gravecaller", "Ravager", "Cultist"][i] + f" L1{i}",
                          hp=0.6, name_rgb=S.LAWFUL_NAME if i % 2 else S.NEUTRAL_NAME))
    sc.ents = ents
    tel = Image.new("RGBA", (192, 96), (0, 0, 0, 0))
    ImageDraw.Draw(tel).ellipse([2, 2, 189, 93], outline=(0x2A, 0x1A, 0x30, 255), width=3)  # rot ring, violet-black (curse licence)
    sc.decals = [(tel, *T(23.0, 3.5))]
    day = sc.render()
    x, y = sc.anchor(*T(23.0, 3.5))
    t = f7.render("BLOOD BOLT!", rgb=(235, 40, 160))
    day.alpha_composite(t, (x - t.width // 2, y - 80))
    board = S.triptych(day, labels=("BOSS FONT day (B4 cells + B0 player stand-ins)", "night 02:00", "grey"))
    board.save(QA / "b4_boss_occupancy_3x.png")

    occ = C.occlusion(sc, ents)
    # attack sweep must stay inside the cell: compare attack_S_1 footprint with the cell bounds
    aa = np.asarray(boss_atk)
    ys, xs = np.where(aa[..., 3] == 255)
    sweep_inside = bool(xs.min() >= 0 and xs.max() <= 63 and ys.min() >= 0 and ys.max() <= 63)
    bell = bell_lower_16(boss)
    gates = {"no_entity_over_50pct_hidden": occ["entities_over_50pct_hidden"] == 0,
             "boss_max_hidden_le_0_30": occ["max_hidden"] <= 0.30,
             "attack_sweep_inside_64x64": sweep_inside,
             "bell_lower_16px_low_detail": bell["colours"] <= 6 and bell["luma_std"] <= 22}
    audit = {"engine_validated": False, "status": "UNVALIDATED in engine (offline bhscene composite)",
             "source_rects": "data/maps-src/drowned_crypt.tmj spawns (numbers copied from B0 boss_occupancy_audit.json, file not re-read)",
             "cells": {"boss": "1009 walk_S_0 / attack_S_1 (64x64 a58)", "elite": "1010 walk_S_0 (40x60 a52)",
                       "sexton": "1008 walk_S_0", "celebrant": "1007 walk_S_0", "players": "B0 ravager cell stand-ins (B6 not generated)"},
             "occlusion": occ, "bell_lower_16px": bell, "gates": gates, "pass": all(gates.values())}
    (QA / "b4_boss_occupancy_audit.json").write_text(json.dumps(audit, indent=1) + "\n")
    print(json.dumps({"occlusion": occ, "bell": bell, "gates": gates}, indent=1))
    return 0 if all(gates.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
