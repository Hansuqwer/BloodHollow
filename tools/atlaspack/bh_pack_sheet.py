#!/usr/bin/env python3
"""bh_pack_sheet — pack a folder of cleaned cells into sheet.png + sheet.json (v1).

    python3 tools/atlaspack/bh_pack_sheet.py <cells_dir> <out_dir>
        [--cell 32x48] [--anchor-y 42] [--fps walk=10,attack=12,die=8]
        [--order walk,attack,die] [--pad-missing]

Input naming (one PNG per frame, already at cell size and quantised):
    <anim>_<DIR>_<frame>.png     e.g. walk_S_0.png, attack_NE_2.png, idle_E_0.png
DIR ∈ E SE S SW W NW N NE (sim::kDx order, rows 0..7). Frame index 0-based.

Rules enforced (engine/assets/atlas.cpp + bible §5):
  * every anim has all 8 dirs with the same frame count (or --pad-missing
    duplicates the nearest available dir — flagged in the JSON as "padded",
    for early WIP sheets only; never ship padded)
  * all cells exactly --cell; feet row check (lowest opaque body row within
    2 px of --anchor-y) is reported per frame, not enforced
  * writes anchorY into every anim block (ignored by the loader until T-ART-10)
  * runs validate_atlas (loader-rule replica) and bh_qa_sheet-style colour
    count; prints a summary; exit 1 on any hard failure

Deterministic: same cells -> same bytes.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402

NAME_RE = re.compile(r"^(?P<anim>[a-z_]+)_(?P<dir>E|SE|S|SW|W|NW|N|NE)_(?P<frame>\d+)\.png$")
DEFAULT_FPS = {"idle": 1, "walk": 10, "attack": 12, "cast": 10, "hurt": 12, "die": 8, "gib": 12, "summon": 8}
DEFAULT_ORDER = ["idle", "walk", "attack", "cast", "hurt", "die", "gib", "summon"]


def load_cells(d: Path, cell: tuple[int, int]) -> tuple[dict, list[str]]:
    anims: dict[str, dict[str, dict[int, Image.Image]]] = {}
    errs: list[str] = []
    for p in sorted(d.glob("*.png")):
        m = NAME_RE.match(p.name)
        if not m:
            errs.append(f"ignored (bad name): {p.name}")
            continue
        im = Image.open(p).convert("RGBA")
        if im.size != cell:
            errs.append(f"{p.name}: size {im.size} != cell {cell}")
            continue
        anims.setdefault(m["anim"], {}).setdefault(m["dir"], {})[int(m["frame"])] = im
    return anims, errs


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("cells_dir")
    ap.add_argument("out_dir")
    ap.add_argument("--cell", default="32x48")
    ap.add_argument("--anchor-y", type=int, default=42)
    ap.add_argument("--fps", default="")
    ap.add_argument("--order", default="")
    ap.add_argument("--pad-missing", action="store_true")
    ap.add_argument("--max-colours", type=int, default=32)
    args = ap.parse_args()

    cw, ch = (int(v) for v in args.cell.lower().split("x"))
    fps = dict(DEFAULT_FPS)
    for kv in filter(None, args.fps.split(",")):
        k, v = kv.split("="); fps[k] = float(v)
    src, out = Path(args.cells_dir), Path(args.out_dir)
    anims_raw, errs = load_cells(src, (cw, ch))
    if not anims_raw:
        print("no cells found in", src); return 2

    order = [a for a in (args.order.split(",") if args.order else DEFAULT_ORDER) if a in anims_raw]
    order += [a for a in anims_raw if a not in order]

    packed: dict[str, dict] = {}
    report: dict = {"anims": {}, "padded": [], "feet_warnings": [], "errors": errs}
    hard_fail = False
    for name in order:
        dirs = anims_raw[name]
        n = max((max(fr) + 1 for fr in dirs.values()), default=0)
        rows = []
        for d in P.DIR_ORDER:
            frames = dirs.get(d, {})
            if len(frames) != n or any(i not in frames for i in range(n)):
                if args.pad_missing:
                    # nearest available dir by angular distance
                    have = [k for k in P.DIR_ORDER if k in dirs and len(dirs[k]) == n]
                    if not have:
                        errs.append(f"{name}: no complete dir to pad from"); hard_fail = True; break
                    di = P.DIR_ORDER.index(d)
                    src_d = min(have, key=lambda k: min((P.DIR_ORDER.index(k) - di) % 8, (di - P.DIR_ORDER.index(k)) % 8))
                    frames = dirs[src_d]
                    report["padded"].append(f"{name}/{d} <- {src_d}")
                else:
                    errs.append(f"{name}: dir {d} has {len(frames)}/{n} frames (use --pad-missing for WIP)")
                    hard_fail = True
                    break
            row = [frames[i] for i in range(n)]
            for i, im in enumerate(row):
                a = np.asarray(im)
                ys = np.where((a[..., 3] == 255).any(axis=1))[0]
                if len(ys) and abs(int(ys.max()) - args.anchor_y) > 2:
                    report["feet_warnings"].append(f"{name}/{d}/{i}: lowest body row {int(ys.max())} vs anchorY {args.anchor_y}")
            rows.append(row)
        if len(rows) == 8:
            packed[name] = {"frames": rows, "fps": fps.get(name, 8)}
            report["anims"][name] = {"frames": n, "fps": fps.get(name, 8),
                                     "max_colours": max(P.count_colours(im) for r in rows for im in r)}
    if hard_fail or not packed:
        print(json.dumps(report, indent=1)); return 1

    out.mkdir(parents=True, exist_ok=True)
    meta = P.pack_atlas(packed, out / "sheet.png", out / "sheet.json", cell=(cw, ch))
    for a in meta["anims"].values():
        a["anchorY"] = args.anchor_y
    (out / "sheet.json").write_text(json.dumps(meta, indent=1) + "\n")
    v = P.validate_atlas(out / "sheet.png", out / "sheet.json")
    report["loader_rule_replica"] = v or "clean"
    report["sheet_px"] = list(Image.open(out / "sheet.png").size)
    report["colour_gate"] = all(a["max_colours"] <= args.max_colours for a in report["anims"].values())
    report["engine_validated"] = False
    (out / "pack_report.json").write_text(json.dumps(report, indent=1) + "\n")
    print(json.dumps(report, indent=1))
    return 0 if (not v and report["colour_gate"]) else 1


if __name__ == "__main__":
    raise SystemExit(main())
