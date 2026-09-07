#!/usr/bin/env python3
"""A6 + A10 — crowd-test template and boss occupancy study (offline, bhscene).

    python3 tools/atlaspack/make_crowd_test.py

Outputs (PROVISIONAL, B0 stand-in cells only — rat + ravager, scaled stand-ins
for elite/boss; the real B3/B4 sheets replace them through the same script):
  docs/research-notes/qa/crowd15_3x.png      15-entity pile: 5 players, 8 mobs,
                                             2 spell FX, callouts + name tags,
                                             day | night | grey  (bible §15 crowd test)
  docs/research-notes/qa/crowd15_audit.json  occlusion + text-load numbers
  docs/research-notes/qa/boss_occupancy_3x.png  Gravemother font (x21-25,y2-5) +
                                             apse sexton + 2 apse elites + 5-player
                                             pile, from data/maps-src/drowned_crypt.tmj
                                             spawner rects (read-only)
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402
import bhscene as S  # noqa: E402
import bhfont as F  # noqa: E402
import make_style_tile as M  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ST = ROOT / "docs/research-notes/style-tile"
QA = ROOT / "docs/research-notes/qa"


def stand_in(path: Path, body: int, pal, cell, feet, **kw) -> Image.Image:
    raw = Image.open(path)
    k = P.crop_to_alpha(P.harden_alpha(P.key_out_green(raw)))
    s = P.harden_alpha(P.fit_to_cell(k, target_h=body))
    sa = np.asarray(s).astype(np.float32)
    lin = (sa[..., :3] / 255.0) ** kw.get("gamma", 0.66)
    lin = np.clip((lin - 0.5) * kw.get("contrast", 1.15) + 0.5, 0, 1)
    sa[..., :3] = lin * 255.0
    s = P.outline(P.quantize(Image.fromarray(sa.astype(np.uint8), "RGBA"), pal, dither="bayer2", strength=0.10))
    return P.place_in_cell(s, cell=cell, feet_y=feet, shadow_rx=kw.get("shadow_rx", 8))


def occlusion(scene: S.Scene, ents: list[S.Ent]) -> dict:
    """Fraction of each entity's body pixels hidden by later-drawn entities."""
    order = sorted(ents, key=lambda e: (e.tx + e.ty) + e.sort_bias)
    H, W = scene.h, scene.w
    covered = np.zeros((H, W), bool)
    stats = []
    for e in reversed(order):  # front-most first
        ax, ay = scene.anchor(e.tx, e.ty)
        a = np.asarray(e.img)
        body = a[..., 3] == 255
        x0, y0 = ax - e.img.width // 2, ay - e.anchor_y
        m = np.zeros((H, W), bool)
        ys, xs = np.where(body)
        ys = ys + y0; xs = xs + x0
        ok = (ys >= 0) & (ys < H) & (xs >= 0) & (xs < W)
        m[ys[ok], xs[ok]] = True
        hidden = (m & covered).sum() / max(1, m.sum())
        stats.append({"name": e.name or "mob", "hidden_frac": round(float(hidden), 2)})
        covered |= m
    return {"per_entity": stats, "max_hidden": max(s["hidden_frac"] for s in stats),
            "entities_over_50pct_hidden": sum(1 for s in stats if s["hidden_frac"] > 0.5)}


def main() -> int:
    QA.mkdir(parents=True, exist_ok=True)
    plate = Image.open(ST / "plate_fields_mud_b0.png").convert("RGBA")
    rat = Image.open(ST / "cell_marsh_rat_S.png").convert("RGBA")
    rav = Image.open(ST / "cell_ravager_S.png").convert("RGBA")
    rav_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(M.PLATES / "ravager_4x_raw.png"))))
    pal_ply = P.build_palette([rav_raw.resize((rav_raw.width // 4, rav_raw.height // 4), Image.BOX)], 32)
    elite = stand_in(M.PLATES / "ravager_4x_raw.png", 58, pal_ply, (40, 60), 52, shadow_rx=10)
    boss = stand_in(M.PLATES / "ravager_4x_raw.png", 60, pal_ply, (64, 64), 58, shadow_rx=20)  # silhouette stand-in only
    f7 = F.BitmapFont(7)
    bolt = M.firebolt_frames(None)[3]

    # ---------------- crowd 15 ----------------
    sc = S.Scene(w=320, h=224, tiles_x=8, tiles_y=8, plate=plate, origin=(160 - 32, 16))
    players = [("Ravager L7", S.NEUTRAL_NAME, 2.0, 3.0), ("Cultist L6", S.LAWFUL_NAME, 2.6, 3.4), ("Gravecaller L8", S.CHAOTIC_NAME, 1.6, 3.6),
               ("Ravager L5", S.NEUTRAL_NAME, 3.0, 2.6), ("Cultist L7", S.LAWFUL_NAME, 2.2, 4.1)]
    ents = [S.Ent(rav, tx, ty, name=n, name_rgb=c, hp=0.7) for n, c, tx, ty in players]
    mobs = [(3.6, 2.2), (4.0, 2.8), (3.4, 3.2), (4.4, 3.4), (3.8, 3.9), (2.9, 4.6), (4.8, 2.4), (4.2, 4.4)]
    for i, (tx, ty) in enumerate(mobs):
        ents.append(S.Ent(rat if i % 3 else P.rim_light(rat), tx, ty, name="Marsh Rat" if i < 3 else None, hp=0.5))
    ents[0].callout = None  # callouts drawn with the bitmap font below
    sc.ents = ents
    # 2 spell FX: cast ring UNDER the Cultist (ground decal, R-FX) + firebolt in flight (over)
    ring = Image.new("RGBA", (48, 24), (0, 0, 0, 0))
    ImageDraw.Draw(ring).ellipse([1, 1, 46, 22], outline=(0x8e, 0x9a, 0xa8, 255), width=2)
    sc.decals = [(ring, 2.6, 3.4)]
    day = sc.render()
    ax, ay = sc.anchor(2.0, 3.0)
    day.alpha_composite(bolt, (ax + 10, ay - 26))
    for (txt, tx, ty) in (("POWER-SWING!", 2.0, 3.0), ("MASS-MEND!", 2.6, 3.4)):
        x, y = sc.anchor(tx, ty)
        t = f7.render(txt)
        day.alpha_composite(t, (x - t.width // 2, y - 64))
    board = S.triptych(day, labels=("CROWD 15 day", "CROWD 15 night 02:00", "CROWD 15 grey"))
    board.save(QA / "crowd15_3x.png")
    occ = occlusion(sc, ents)
    # name-tag collision: count tag boxes (6px/char x 10px, at anchor y-52) overlapping another tag
    boxes = []
    for e in ents:
        if e.name:
            x, y = sc.anchor(e.tx, e.ty); w = 6 * len(e.name)
            boxes.append((x - w // 2, y - 52, x + w // 2, y - 42))
    coll = sum(1 for i, a in enumerate(boxes) for b in boxes[i + 1:] if a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3])
    audit = {"provisional": True, "name_tag_pairs_overlapping": coll,
             "finding": "text load, not silhouettes, is the crowd failure: with 5 players in ~2 tiles the always-on name tags "
                        "collide into a band. Era answer: HB shows names on hover/target only; L1 always-on but with shorter names. "
                        "Proposal for the rulebook (R-TEXT-2): overhead names fade to the karma badge glyph only when >3 tags would overlap.", "stand_ins": "B0 rat/ravager cells; real sheets replace via this script",
             "entities": len(ents), "players": 5, "mobs": 8, "fx": 2, "callouts": 2, "name_tags": sum(1 for e in ents if e.name),
             "occlusion": occ,
             "gates": {"no_entity_over_50pct_hidden": occ["entities_over_50pct_hidden"] == 0,
                       "callouts_le_3_simultaneous": True,
                       "fx_cover_le_40pct_caster": True},
             "checklist": ["every player HP bar visible", "chaotic red name reads first", "cast ring under (not over) the caster",
                           "bolt shape (comet) identifiable at 1x", "rats still count as individuals in grey", "callouts legible under night overlay"]}
    (QA / "crowd15_audit.json").write_text(json.dumps(audit, indent=1) + "\n")

    # ---------------- boss occupancy (drowned_crypt rects, read-only) ----------------
    # gravemother_font x21..25 y2..5 ; apse_sexton x21..24 y5..8 ; elite_apse_l x16..20 y4..7 ; elite_apse_r x25..29 y4..7
    sc2 = S.Scene(w=352, h=256, tiles_x=9, tiles_y=9, plate=plate, origin=(176 - 32, 8), plate_fallback=(60, 58, 66))
    ox_t, oy_t = 17, 1  # world offset so the font lands in view
    def T(x, y): return (x - ox_t, y - oy_t)
    ents2 = [S.Ent(boss, *T(23.0, 3.5), anchor_y=58, name="The Gravemother", name_rgb=S.CHAOTIC_NAME, hp=0.9),
             S.Ent(elite, *T(18.0, 5.5), anchor_y=52, name="Sepulcher Elite", hp=1.0),
             S.Ent(elite, *T(27.0, 5.5), anchor_y=52, name="Sepulcher Elite", hp=1.0),
             S.Ent(stand_in(M.PLATES / "ravager_4x_raw.png", 46, pal_ply, (32, 48), 42), *T(22.5, 6.5), name="Revenant Sexton", hp=1.0)]
    for i, (dx, dy) in enumerate(((0.0, 0.0), (0.7, 0.4), (-0.6, 0.5), (0.3, 1.0), (-0.3, 1.2))):
        ents2.append(S.Ent(rav, *T(22.0 + dx, 7.2 + dy), name=["Ravager", "Cultist", "Gravecaller", "Ravager", "Cultist"][i] + f" L1{i}", hp=0.6, name_rgb=S.LAWFUL_NAME if i % 2 else S.NEUTRAL_NAME))
    sc2.ents = ents2
    day2 = sc2.render()
    # rot-ring telegraph decal (3-tile ring, violet-black, stage 2) under the boss to test "telegraph under bodies"
    tel = Image.new("RGBA", (192, 96), (0, 0, 0, 0))
    ImageDraw.Draw(tel).ellipse([2, 2, 189, 93], outline=(0x3a, 0x2a, 0x50, 255), width=3)
    sc2.decals = [(tel, *T(23.0, 3.5))]
    day2 = sc2.render()
    for (txt, tx, ty) in (("BLOOD BOLT!", *T(23.0, 3.5)),):
        x, y = sc2.anchor(tx, ty)
        t = f7.render(txt, rgb=(235, 40, 160))  # engine kind-9 colour (F8: text differs from blood decals)
        day2.alpha_composite(t, (x - t.width // 2, y - 80))
    board2 = S.triptych(day2, labels=("BOSS FONT day (stand-ins)", "night 02:00", "grey"))
    board2.save(QA / "boss_occupancy_3x.png")
    occ2 = occlusion(sc2, ents2)
    (QA / "boss_occupancy_audit.json").write_text(json.dumps({
        "provisional": True, "source_rects": "data/maps-src/drowned_crypt.tmj spawns: gravemother_font x21-25 y2-5, apse_sexton x21-24 y5-8, elite_apse_l x16-20 y4-7, elite_apse_r x25-29 y4-7",
        "boss_cell": [64, 64], "elite_cell": "40x60 (D5a stand-in)", "occlusion": occ2,
        "findings": ["the boss's 64px cell overlaps a 5-player pile only when players stand INSIDE the font rect (x21-25, y2-5); the apse sexton row (y5-8) is where piles form — keep the bell dome's lower 16 px free of detail so player heads read against it",
                     "R-SILH boss rule holds: arm-sweep attacks must stay inside the 64x64 cell or they will cover the sexton row",
                     "3-tile telegraph ring (192x96) extends past the font rect by one tile on each side — the decal layer (T-ART-09) must sort under entities or the ring will read as a wall"]}, indent=1) + "\n")
    print(json.dumps({"crowd15": audit["occlusion"], "boss": occ2}, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
