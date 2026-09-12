#!/usr/bin/env python3
"""Cross-map validator: every portal lands on a walkable tile of a known map.

Reads data/maps-src/*.tmj, builds the map graph, checks:
  1. every portal targetMapId resolves to an existing map (id table below)
  2. every portal's own rect is >=1 walkable tile (you can stand on it)
  3. every portal's arrival tile is walkable on the target map
  4. spawner rects: >= 40% walkable (dead spawners stderr the generator)
Used by CI/pre-commit: `python3 tools/mapgen/validate_links.py` exits 1 on any
failure, printing all problems at once.
"""
import json
import sys
from pathlib import Path

MAPIDS = {1: "thornwall.tmj", 2: "fields_overflow.tmj", 3: "thornwall_crypt.tmj",
          4: "bonehowl_mine.tmj", 5: "drowned_crypt.tmj",
          6: "weeping_castle.tmj"}
WALL_IDS = {2, 3}  # thornwall terrain constants: WALL=2, WATER=3 blocked


def load(path: Path):
    m = json.loads(path.read_text())
    w, h = m["width"], m["height"]
    layers = {l["name"]: l for l in m["layers"]}
    ground = layers["ground"]["data"]
    walkable = lambda x, y: 0 <= x < w and 0 <= y < h and (ground[y * w + x] - 1) not in WALL_IDS
    portal_objs = []
    for lname in ("portals", "markers"):
        if lname in layers and layers[lname].get("type") == "objectgroup":
            for o in layers[lname]["objects"]:
                if o["type"] == "portal":
                    props = {p["name"]: p["value"] for p in o["properties"]}
                    portal_objs.append({
                        "name": o["name"],
                        "x0": int(o["x"]) // m["tilewidth"], "y0": int(o["y"]) // m["tileheight"],
                        "xw": max(1, int(o["width"]) // m["tilewidth"]),
                        "yh": max(1, int(o["height"]) // m["tileheight"]),
                        "targetMapId": props.get("targetMapId"),
                        "targetX": props.get("targetX"), "targetY": props.get("targetY"),
                    })
    spawners = []
    for lname in ("spawns", "markers"):
        if lname in layers and layers[lname].get("type") == "objectgroup":
            for o in layers[lname]["objects"]:
                if o["type"] in ("spawner", "spawn"):
                    spawners.append({"name": o["name"], "x0": int(o["x"]) // m["tilewidth"],
                                     "y0": int(o["y"]) // m["tileheight"],
                                     "xw": max(1, int(o["width"]) // m["tilewidth"]),
                                     "yh": max(1, int(o["height"]) // m["tileheight"])})
    return w, h, walkable, portal_objs, spawners


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    src = repo / "data" / "maps-src"
    problems = []
    maps = {}
    for mid, fname in MAPIDS.items():
        p = src / fname
        if not p.exists():
            problems.append(f"mapId {mid}: file missing ({p.name})")
            continue
        maps[mid] = load(p)

    for mid, (w, h, walk, portals, spw) in maps.items():
        for pt in portals:
            stand = [walk(pt["x0"] + dx, pt["y0"] + dy) for dx in range(pt["xw"]) for dy in range(pt["yh"])]
            if not any(stand):
                problems.append(f"map {mid} portal '{pt['name']}': no standable tile in rect")
            t = pt["targetMapId"]
            if t not in maps:
                problems.append(f"map {mid} portal '{pt['name']}': target map {t} unknown")
                continue
            tw, th, twalk, _, _ = maps[t]
            tx, ty = pt["targetX"], pt["targetY"]
            if not (0 <= tx < tw and 0 <= ty < th):
                problems.append(f"map {mid} portal '{pt['name']}': arrival ({tx},{ty}) out of bounds on map {t}")
            elif not twalk(tx, ty):
                problems.append(f"map {mid} portal '{pt['name']}': arrival tile ({tx},{ty}) blocked on map {t}")
        for sp in spw:
            total = sp["xw"] * sp["yh"]
            ok = sum(1 for dx in range(sp["xw"]) for dy in range(sp["yh"])
                     if walk(sp["x0"] + dx, sp["y0"] + dy))
            if total == 0 or ok / total < 0.4:
                problems.append(f"map {mid} spawner '{sp['name']}': only {ok}/{total} tiles walkable")

    for p in problems:
        print(f"FAIL: {p}", file=sys.stderr)
    print(f"validate_links: {len(problems)} problems across {len(maps)} maps")
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
