#!/usr/bin/env python3
"""bh_qa_sheet — offline §15 QA for any cell, packed sheet, or ground plate.

    python3 tools/atlaspack/bh_qa_sheet.py <png> [--json sheet.json]
        [--plate ground_plate.png] [--terrain-luma 51] [--cell 32x48]
        [--anchor-y 42] [--hour 2] [--out docs/research-notes/qa/<name>]
        [--gate 25] [--night-gate 15] [--max-colours 32]

Writes <out>_qa_3x.png (day | engine night | greyscale) and <out>_audit.json.
Exit code 0 = all gates pass, 1 = a gate failed (numbers in the JSON), 2 = bad input.

Gates (docs/research-notes/helbreath/readability-rulebook.md):
  R-LUMA   sprite body mean luma - terrain mean >= --gate (default 25)
  R-NIGHT  same delta after the engine overlay at --hour >= --night-gate (15)
  palette  opaque colours <= --max-colours (32); VFX pass --max-colours 16
  tech     JSON sidecar passes the loadAtlas rule replica (dirs 8, bounds),
           cell dims match --cell, feet row has opaque pixels within 2px of anchor

Nothing is marked "passed" for the engine: this is a Python replica of the
loader rules (engine/assets/atlas.cpp:18-40); in-engine load is a separate
proof (T-ART-01/05).
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402
import bhscene as S  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
QA_DIR = ROOT / "docs/research-notes/qa"


def parse_cell(s: str) -> tuple[int, int]:
    w, h = s.lower().split("x")
    return int(w), int(h)


def frames_from_sheet(img: Image.Image, meta: dict) -> list[tuple[str, int, int, Image.Image]]:
    """Yield (anim, dir, frame, cell) for every cell in a v1 sheet."""
    out = []
    for name, a in meta["anims"].items():
        fw, fh, n = a["frameW"], a["frameH"], a["frames"]
        ox, oy = a.get("offsetX", 0), a.get("offsetY", 0)
        for d in range(8):
            for f in range(n):
                out.append((name, d, f, img.crop((ox + f * fw, oy + d * fh, ox + (f + 1) * fw, oy + (d + 1) * fh))))
    return out


def feet_check(cell: Image.Image, anchor_y: int, tol: int = 2) -> bool:
    a = np.asarray(cell.convert("RGBA"))
    rows = np.where((a[..., 3] == 255).any(axis=1))[0]
    if len(rows) == 0:
        return False
    # lowest opaque body row should sit on the anchor row (±tol); the shadow is alpha 70 so excluded
    return abs(int(rows.max()) - anchor_y) <= tol


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("png")
    ap.add_argument("--json")
    ap.add_argument("--plate", help="ground plate PNG the asset will stand on (for R-LUMA); else --terrain-luma")
    ap.add_argument("--terrain-luma", type=float, default=None)
    ap.add_argument("--cell", default="32x48")
    ap.add_argument("--anchor-y", type=int, default=42)
    ap.add_argument("--hover", type=int, default=0, help="hoverers (REGISTRY: bat body bottom ≈ anchor-12): flight anims check anchor-hover; die/corpse frames must land on the anchor")
    ap.add_argument("--hour", type=float, default=2.0)
    ap.add_argument("--out")
    ap.add_argument("--gate", type=float, default=25.0)
    ap.add_argument("--night-gate", type=float, default=15.0)
    ap.add_argument("--max-colours", type=int, default=32)
    ap.add_argument("--kind", choices=["cell", "sheet", "plate", "auto"], default="auto")
    args = ap.parse_args()

    src = Path(args.png)
    if not src.exists():
        print("no such file", src); return 2
    img = Image.open(src).convert("RGBA")
    cw, ch = parse_cell(args.cell)
    kind = args.kind
    if kind == "auto":
        kind = "sheet" if args.json else ("cell" if img.size == (cw, ch) else "plate")
    out = Path(args.out) if args.out else QA_DIR / src.stem
    out.parent.mkdir(parents=True, exist_ok=True)

    audit: dict = {"input": str(src.relative_to(ROOT)) if src.is_relative_to(ROOT) else str(src),
                   "kind": kind, "cell": [cw, ch], "anchor_y": args.anchor_y, "hour": args.hour,
                   "night_overlay": P.night_overlay(args.hour), "hover": args.hover, "gates": {}, "engine_validated": False}

    plate = Image.open(args.plate).convert("RGBA") if args.plate else None
    if plate is not None:
        terrain_luma = P.luma_mean(plate, body_only=False, exclude_outline=False)
    elif args.terrain_luma is not None:
        terrain_luma = args.terrain_luma
    else:
        terrain_luma = 51.0  # B0 fields plate mean (style_tile_audit.json)
        audit["note"] = "terrain luma defaulted to B0 fields plate (51.0); pass --plate for the real zone"
    audit["terrain_luma"] = round(terrain_luma, 1)

    failures = []

    if kind == "plate":
        lum = P.luma_mean(img, body_only=False, exclude_outline=False)
        a = np.asarray(img)
        lmin = float((a[..., 0] * .299 + a[..., 1] * .587 + a[..., 2] * .114).min())
        cols = P.count_colours(img)
        audit["plate"] = {"luma_mean": round(lum, 1), "luma_min": round(lmin, 1), "colours": cols}
        g = audit["gates"]
        g["plate_luma_mean_45_70"] = 45 <= lum <= 70
        g["plate_luma_min_ge_24"] = lmin >= 24
        g["colours_le_max"] = cols <= args.max_colours
        failures += [k for k, v in g.items() if not v]
        # a 4x4 patch scene for the triptych
        sc = S.Scene(w=320, h=200, tiles_x=5, tiles_y=5, plate=img)
        day = sc.render()
    else:
        if kind == "sheet":
            if not args.json:
                print("--json required for sheets"); return 2
            meta = json.loads(Path(args.json).read_text())
            errs = P.validate_atlas(src, Path(args.json))
            audit["loader_rule_replica"] = errs or "clean"
            if errs:
                failures.append("loader_rules")
            cells = frames_from_sheet(img, meta)
            audit["frames_total"] = len(cells)
            # representative cell = S-facing idle/walk frame 0 if present
            rep = next((c for c in cells if c[1] == 2), cells[0])[3]
            per_anim = {}
            for name, a in meta["anims"].items():
                these = [c[3] for c in cells if c[0] == name]
                per_anim[name] = {"frames": a["frames"], "fps": a.get("fps"),
                                  "max_colours": max(P.count_colours(c) for c in these),
                                  "feet_ok_all_dirs": all(feet_check(c, args.anchor_y - (0 if name == "die" else args.hover)) for c in these)}
            audit["anims"] = per_anim
            g = audit["gates"]
            g["colours_le_max_all_frames"] = all(v["max_colours"] <= args.max_colours for v in per_anim.values())
            g["feet_on_anchor_all_frames"] = all(v["feet_ok_all_dirs"] for v in per_anim.values())
            failures += [k for k, v in g.items() if not v]
        else:
            rep = img
            g = audit["gates"]
            g["cell_dims"] = img.size == (cw, ch)
            g["feet_on_anchor"] = feet_check(img, args.anchor_y - args.hover)
            g["colours_le_max"] = P.count_colours(img) <= args.max_colours
            audit["colours"] = P.count_colours(img)
            failures += [k for k, v in g.items() if not v]

        body = P.luma_mean(rep)
        night_rep = P.night_floor(rep, args.hour)
        # night delta: apply the overlay to a flat terrain patch of the same luma
        tl = int(terrain_luma)
        night_terr = P.luma_mean(P.night_floor(Image.new("RGBA", (8, 8), (tl, tl, tl, 255)), args.hour), body_only=False, exclude_outline=False)
        # body pixels after the overlay (outline stays dark; mask by the *day* body)
        a_day = np.asarray(rep); a_n = np.asarray(night_rep).astype(np.float32)
        m = (a_day[..., 3] == 255) & ~((a_day[..., 0] == 0x1A) & (a_day[..., 1] == 0x12) & (a_day[..., 2] == 0x14))
        night_body = float((a_n[m][:, 0] * .299 + a_n[m][:, 1] * .587 + a_n[m][:, 2] * .114).mean()) if m.any() else 0.0
        body_incl_outline = P.luma_mean(rep, exclude_outline=False)
        audit["r_luma"] = {"body_mean": round(body, 1), "terrain_mean": round(terrain_luma, 1),
                           "delta_day": round(body - terrain_luma, 1),
                           "body_mean_incl_outline": round(body_incl_outline, 1),
                           "delta_day_incl_outline": round(body_incl_outline - terrain_luma, 1),
                           "definition": "rulebook R-LUMA = opaque body pixels EXCLUDING the #1a1214 outline; "
                                         "the B0 style_tile_audit.json included the outline (hence its lower 23.7)",
                           "body_night": round(night_body, 1), "terrain_night": round(night_terr, 1),
                           "delta_night": round(night_body - night_terr, 1)}
        g = audit["gates"]
        g["r_luma_day_ge_gate"] = (body - terrain_luma) >= args.gate
        g["r_luma_night_ge_gate"] = (night_body - night_terr) >= args.night_gate
        failures += [k for k in ("r_luma_day_ge_gate", "r_luma_night_ge_gate") if not g[k]]
        # scene: the cell on the plate (or flat terrain) beside two copies for the crowd read
        sc = S.Scene(w=256, h=192, tiles_x=5, tiles_y=5, plate=plate,
                     plate_fallback=(tl, tl, tl))
        sc.ents = [S.Ent(rep, 2.0, 2.0, anchor_y=args.anchor_y),
                   S.Ent(rep, 3.0, 1.6, anchor_y=args.anchor_y),
                   S.Ent(rep, 1.2, 3.1, anchor_y=args.anchor_y)]
        day = sc.render()

    audit["failures"] = failures
    board = S.triptych(day, hour=args.hour)
    # stamp the verdict on the board
    d = ImageDraw.Draw(board)
    verdict = "ALL GATES PASS (offline replica; engine load UNVALIDATED)" if not failures else "FAIL: " + ", ".join(failures)
    d.text((4, board.height - 12), verdict, fill=(120, 235, 120) if not failures else (255, 120, 90))
    board.save(str(out) + "_qa_3x.png")
    Path(str(out) + "_audit.json").write_text(json.dumps(audit, indent=1) + "\n")
    print(json.dumps(audit, indent=1))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
