#!/usr/bin/env python3
"""bh_terrain_preview — offline stand-in for the T-ART-12 renderer: draws a window of
the real map (data/maps-src/*.tmj ground layer, READ-ONLY) with the B1 plates cut at
world px, D12 edge overlays picked per 8-neighbour set, WALL prisms skinned, then the
engine night overlay + greyscale → docs/research-notes/qa/b1_<zone>_map_3x.png and
b1_<zone>_map_audit.json (R-LUMA of the B3 rat/ghoul cells dropped on the ground).

Nothing here is the engine; it is the proposal T-ART-12 would implement.
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
SRC = ROOT / "data/maps-src"
QA = ROOT / "docs/research-notes/qa"
TW, TH, PRISM_H = 64, 32, 28
MOBS = ROOT / "assets/aigen/mobs"


def iso(tx, ty, ox, oy):
    return ((tx - ty) * TW * 0.5 + ox, (tx + ty) * TH * 0.5 + oy)


def shear_face(strip: Image.Image, side: str) -> Image.Image:
    """Engine faces are parallelograms: left face spans bL→bB (drops TH/2 over 32 px),
    right face bB→bR (rises TH/2). Shear the upright 32×28 strip accordingly."""
    w, h = strip.size
    out = Image.new("RGBA", (w, h + TH // 2), (0, 0, 0, 0))
    a = np.asarray(strip)
    o = np.asarray(out).copy()
    for x in range(w):
        dy = int(round((x + 0.5) / w * (TH / 2))) if side == "left" else int(round((1 - (x + 0.5) / w) * (TH / 2)))
        o[dy:dy + h, x] = a[:, x]
    return Image.fromarray(o, "RGBA")


def render_map_preview(zone: str, plates: dict[int, Image.Image], names: dict[int, str], mf: dict,
                       skin: dict[str, Image.Image], zdir: Path, pal: np.ndarray,
                       *, prism_variants: int = 1, qa_prefix: str = "b1") -> dict:
    m = json.loads((SRC / f"{mf['map']}.tmj").read_text())
    W, H = m["width"], m["height"]
    fg = m["tilesets"][0]["firstgid"]
    ground = [g - fg for g in next(l for l in m["layers"] if l["name"] == "ground")["data"]]
    # Window scorer: distinct ids plus a small bonus for the material that dominates the
    # zone's visual read. Thornwall crypts explicitly reward the rare BONEPIT/CANDLE ids.
    n = 16
    reward_names = {"WALL", "WATER"}
    if zone == "crypt_thornwall":
        reward_names.update({"BONEPIT", "CANDLE"})
    best = None
    for yy in range(0, H - n + 1, 2):
        for xx in range(0, W - n + 1, 2):
            ids = [ground[(yy + j) * W + xx + i] for j in range(n) for i in range(n)]
            score = len(set(ids)) * 100 + sum(1 for v in ids if names.get(v) in reward_names) * 0.5
            if best is None or score > best[0]:
                best = (score, xx, yy)
    _, x0, y0 = best
    cw, ch = (n + 1) * TW, (n + 1) * TH + PRISM_H + 8
    scene = Image.new("RGBA", (cw, ch), (0x12, 0x10, 0x10, 255))
    ox, oy = cw / 2 - TW / 2, PRISM_H + 4
    edges_dir = zdir / "edges"
    edge_cache: dict[str, dict[str, Image.Image]] = {}
    tj = json.loads((zdir / "terrain.json").read_text()) if (zdir / "terrain.json").exists() else {}
    pair_variants = {(p.get("base_tile"), p.get("overlay")): int(p.get("variants", 1)) for p in tj.get("pairs", []) if "dir" in p}

    def tile_id(tx, ty):
        if 0 <= tx < W and 0 <= ty < H:
            return ground[ty * W + tx]
        return None

    def load_edge(base: str, over: str, variant: int):
        key = f"{base}_{over}/v{variant}"
        if key not in edge_cache:
            d = edges_dir / f"{base}_{over}" / f"v{variant}"
            edge_cache[key] = {p.stem: Image.open(p).convert("RGBA") for p in d.glob("*.png")} if d.exists() else {}
        return edge_cache[key]

    def prism_for(v: int):
        if not skin:
            return None
        if prism_variants <= 1:
            return skin
        d = zdir / "prism" / f"v{v}"
        pv = {name: Image.open(d / f"{name}.png").convert("RGBA") for name in ("top", "left", "right")}
        pv.update({name: im for name, im in skin.items() if name.startswith("skirt_")})
        return pv

    used_edges = 0
    walls = []
    # ground pass (painter order: rows of tx+ty)
    for ty in range(n):
        for tx in range(n):
            wx, wy = x0 + tx, y0 + ty
            t = tile_id(wx, wy)
            if t is None:
                continue
            x, y = iso(tx, ty, ox, oy)
            px, py = int(x), int(y)
            base_id = t
            if names.get(t) == "WALL":
                walls.append((tx, ty, px, py, wx, wy))
                base_id = 0 if 0 in plates else next(iter(plates))
            if base_id not in plates:
                base_id = 5 if 5 in plates else next(iter(plates))  # WOOD → PATH stand-in
            wpx, wpy = int((wx - wy) * TW * 0.5), int((wx + wy) * TH * 0.5)   # world px for the cut
            scene.alpha_composite(P.cut_diamond(plates[base_id], wpx, wpy), (px, py))
            # D12 overlays: for every neighbour material that bleeds over this tile
            if names.get(t) != "WALL":
                nb_sets: dict[str, set] = {}
                skirt_sides: set = set()
                for side, (dx, dy) in {**P.EDGE_FACES, **P.EDGE_POINTS}.items():
                    nt = tile_id(wx + dx, wy + dy)
                    if nt is None or nt == t:
                        continue
                    nname = names.get(nt)
                    if nname == "WALL":
                        skirt_sides.add(side); continue
                    d = edges_dir / f"{names[t]}_{nname}"
                    if d.exists():
                        nb_sets.setdefault(nname, set()).add(side)
                for over, sides in nb_sets.items():
                    vcount = pair_variants.get((names[t], over), 1)
                    variant = (wx * 7 + wy * 13) % max(1, vcount)
                    pieces = load_edge(names[t], over, variant)
                    if not pieces:
                        continue
                    faces = [f for f in P.EDGE_FACES if f in sides]
                    pts = [p for p, (fa, fb) in P._POINT_FACES.items() if p in sides and fa not in sides and fb not in sides]
                    for f in faces:
                        scene.alpha_composite(pieces[f"edge_{f}"], (px, py)); used_edges += 1
                    for p in pts:
                        scene.alpha_composite(pieces[f"corner_{p}"], (px, py)); used_edges += 1
                # WALL footing skirt on ground tiles that touch a wall (faces only)
                for f in [f for f in P.EDGE_FACES if f in skirt_sides]:
                    if skin:
                        sk = skin.get(f"skirt_edge_{f}")
                        if sk is not None:
                            scene.alpha_composite(sk, (px, py))
    # wall pass (after ground, painter order); crypt_drowned has no WALL/prism.
    if skin and walls:
        for tx, ty, px, py, wx, wy in sorted(walls, key=lambda w: w[0] + w[1]):
            ps = prism_for((wx * 7 + wy * 13) % max(1, prism_variants))
            left = shear_face(ps["left"], "left"); right = shear_face(ps["right"], "right")
            scene.alpha_composite(left, (px, py + TH // 2 - PRISM_H))
            scene.alpha_composite(right, (px + TW // 2, py + TH // 2 - PRISM_H))
            scene.alpha_composite(ps["top"], (px, py - PRISM_H))
    # B2 scale witnesses: hound + gnoll for the mine, sexton + celebrant for both crypt maps.
    if zone == "mine":
        witnesses = (("1003_hollow_hound", "walk_S_0", (5, 6)), ("1005_bonepicker_gnoll", "walk_S_0", (8, 7)))
    elif zone.startswith("crypt_"):
        witnesses = (("1008_revenant_sexton", "walk_S_0", (5, 6)), ("1007_gravecaller", "walk_S_0", (8, 7)))
    else:
        # Preserve the established B1 board evidence for town/fields; B2 uses its
        # own crypt/mine scale witnesses and QA prefix.
        witnesses = (("1001_marsh_rat", "walk_S_0", (5, 6)), ("1002_feral_ghoul", "walk_S_0", (8, 7)))
    ents = []
    for mob, cell, (tx, ty) in witnesses:
        p = MOBS / mob / "cells" / f"{cell}.png"
        if p.exists():
            im = Image.open(p).convert("RGBA")
            x, y = iso(tx, ty, ox, oy)           # tile top-left; feet on the tile centre
            scene.alpha_composite(im, (int(x) + TW // 2 - im.width // 2, int(y) + TH // 2 - 42))
            ents.append((mob, im))
    day = scene
    night = P.night_floor(day, 2.0)
    grey = day.convert("L").convert("RGBA")
    f7 = F.BitmapFont(7)
    lab = 12
    board = Image.new("RGBA", (cw * 3 + 8, ch + lab), (0, 0, 0, 255))
    for i, (im, t) in enumerate(((day, f"{zone.upper()} {mf['map']} window {x0},{y0} day"), (night, "night 02:00 (engine overlay)"), (grey, "grey"))):
        board.alpha_composite(im, (i * (cw + 4), lab))
        board.alpha_composite(f7.render(t), (i * (cw + 4) + 2, 2))
    board = board.resize((board.width * 2, board.height * 2), Image.NEAREST)
    board.save(QA / f"{qa_prefix}_{zone}_map_3x.png")
    # R-LUMA on the composed ground window (this remains an offline board, not engine proof).
    ga = np.asarray(day).astype(np.float32)
    gl = ga[..., 0] * .299 + ga[..., 1] * .587 + ga[..., 2] * .114
    audit = {"engine_validated": False, "window": [x0, y0, n, n], "edge_pieces_drawn": used_edges, "walls_drawn": len(walls),
             "ground_luma_mean_window": round(float(gl[oy + 40: oy + (n) * TH // 2 + 40, cw // 4: 3 * cw // 4].mean()), 1),
             "night_ground_luma_mean": round(float((np.asarray(night).astype(np.float32)[..., :3] @ [.299, .587, .114])[oy + 40: oy + n * TH // 2 + 40, cw // 4: 3 * cw // 4].mean()), 1),
             "cells_dropped": [m for m, _ in ents],
             "scale_witnesses": (["1003_hollow_hound", "1005_bonepicker_gnoll"] if zone == "mine" else
                                 (["1008_revenant_sexton", "1007_gravecaller"] if zone.startswith("crypt_") else
                                  ["1001_marsh_rat", "1002_feral_ghoul"])),
             "note": "offline composite of the real map ground layer; proposal for T-ART-12, not the engine"}
    if qa_prefix == "b1":
        audit.pop("scale_witnesses", None)
    (QA / f"{qa_prefix}_{zone}_map_audit.json").write_text(json.dumps(audit, indent=1) + "\n")
    return audit
