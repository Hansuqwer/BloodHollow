#!/usr/bin/env python3
"""b1_edge_board — QA board for the D12 edge sets of one zone: per pair, the 8 overlay
pieces (variant 0) composited on their base tile, then a 5×5 autotile patch (a bleeder
island inside the base material, variants rotating) at 3×. Writes
docs/research-notes/qa/b1_<zone>_edges_3x.png + _audit.json (seam re-check per piece)."""
from __future__ import annotations
import json, sys
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P
import bhfont as F
ROOT = Path(__file__).resolve().parents[2]
TW, TH = 64, 32
ORDER = ["edge_NE", "edge_SE", "edge_SW", "edge_NW", "corner_N", "corner_E", "corner_S", "corner_W"]

def main(zone: str) -> int:
    zdir = ROOT / "assets/aigen/terrain" / zone
    tj = json.loads((zdir / "terrain.json").read_text())
    names = {int(k): v for k, v in tj["terrain_ids"].items()}
    plates = {v: Image.open(ROOT / tj["plates"][v]["file"]).convert("RGBA") for v in tj["plates"] if tj["plates"][v].get("file")}
    pairs = [p for p in tj["pairs"] if "dir" in p]
    f7 = F.BitmapFont(7)
    row_h = TH + 14 + 5 * TH // 2 + 8
    W = 8 * (TW + 4) + 12 + 6 * TW
    board = Image.new("RGBA", (W, row_h * len(pairs) + 4), (0x12, 0x10, 0x10, 255))
    audit = {}
    for r, p in enumerate(pairs):
        base, over = p["base_tile"], p["overlay"]
        y0 = 4 + r * row_h
        board.alpha_composite(f7.render(f"{base} < {over}  ({p['boundary_len']})"), (4, y0))
        d = ROOT / p["dir"]
        pieces = {k: Image.open(d / "v0" / f"{k}.png").convert("RGBA") for k in ORDER}
        for i, k in enumerate(ORDER):
            t = P.cut_diamond(plates[base], 200 + i * 40, 100)
            t.alpha_composite(pieces[k])
            board.alpha_composite(t, (4 + i * (TW + 4), y0 + 12))
        # 5x5 patch: island of `over` at (2,2),(2,3),(3,2) + lone (4,0); base elsewhere
        ox, oy = 8 * (TW + 4) + 12 + 3 * TW - TW // 2, y0 + 12
        island = {(2, 2), (2, 3), (3, 2), (4, 0)}
        allb = 0
        for ty in range(5):
            for tx in range(5):
                x, y = int((tx - ty) * TW / 2 + ox), int((tx + ty) * TH / 2 + oy)
                wx, wy = (tx + 20 * r) * 37, (ty + 3 * r) * 23
                if (tx, ty) in island:
                    board.alpha_composite(P.cut_diamond(plates[over], wx, wy), (x, y)); continue
                board.alpha_composite(P.cut_diamond(plates[base], wx, wy), (x, y))
                sides = {s for s, (dx, dy) in {**P.EDGE_FACES, **P.EDGE_POINTS}.items() if (tx + dx, ty + dy) in island}
                if not sides:
                    continue
                v = (tx * 7 + ty * 13) % p["variants"]
                pv = {k: Image.open(d / f"v{v}" / f"{k}.png").convert("RGBA") for k in ORDER}
                for f in [f for f in P.EDGE_FACES if f in sides]:
                    board.alpha_composite(pv[f"edge_{f}"], (x, y)); allb += 1
                for pt, (fa, fb) in P._POINT_FACES.items():
                    if pt in sides and fa not in sides and fb not in sides:
                        board.alpha_composite(pv[f"corner_{pt}"], (x, y)); allb += 1
        audit[f"{base}<{over}"] = {"pieces_in_patch": allb, "variants": p["variants"],
                                   "seam": tj["edge_seam_audit"].get(f"{base}_{over}/v0")}
    board = board.resize((board.width * 3, board.height * 3), Image.NEAREST)
    board.save(ROOT / "docs/research-notes/qa" / f"b1_{zone}_edges_3x.png")
    (ROOT / "docs/research-notes/qa" / f"b1_{zone}_edges_audit.json").write_text(json.dumps({"zone": zone, "engine_validated": False, "pairs": audit}, indent=1) + "\n")
    print(zone, len(pairs), "pairs on board")
    return 0

if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1]))
