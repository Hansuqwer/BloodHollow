#!/usr/bin/env python3
"""map_manifest — per-map art manifest from data/maps-src/*.tmj (READ-ONLY).

    python3 tools/atlaspack/map_manifest.py            # all five maps
    python3 tools/atlaspack/map_manifest.py thornwall  # one

Writes assets/aigen/terrain/<zone_dir>/MAPS.md + MAPS.json per map with:
  * terrain-id histogram (which plates matter, in order)
  * adjacency pairs (which edge sets must exist), counted by boundary length
  * zones present, spawner rects (mob, maxAlive) -> scatter-density bands
    (Mir rule: least scatter inside spawner rects, ramping up outside)
  * portals (where the letterbox/fog band must NOT hide the exit)
  * furniture positions the server derives at runtime (spawnPoint spiral for
    Marta(64) / Wanted Board(66) / Anvil(65) in zone 1; anvil at (44,6) in
    zone 3) — reproduced from server/src/world.cpp:84-96, 350-392, 1025-1046
    so the art knows where the warm pools and stalls will actually sit
  * a 1-px-per-tile preview PNG (terrain ids + spawner rects + portals)

Maps are generator output (ADR-007: humans author real maps later); re-run
this when data/maps-src changes. Never writes outside assets/aigen/.
"""
from __future__ import annotations

import json
import sys
from collections import Counter
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "data/maps-src"
OUT = ROOT / "assets/aigen/terrain"

# zone dir per map (bible §17 layout)
ZONE_DIR = {"thornwall": "town", "fields_overflow": "fields", "bonehowl_mine": "mine",
            "drowned_crypt": "crypt", "thornwall_crypt": "crypt"}
MAP_ID = {"thornwall": 1, "fields_overflow": 2, "thornwall_crypt": 3, "bonehowl_mine": 4, "drowned_crypt": 5}
STD_IDS = {0: "GRASS", 1: "DIRT", 2: "WALL", 3: "WATER", 4: "WOOD", 5: "PATH", 6: "MUD", 7: "DARKGRASS"}
CRYPT_IDS = {0: "STONE", 1: "FLOOR", 2: "WALL", 3: "SLAB", 4: "BONEPIT", 5: "CANDLE"}
# placeholder colours from client terrainColor() for the preview
PREVIEW_RGB = {0: (64, 84, 46), 1: (94, 74, 52), 2: (120, 120, 128), 3: (36, 54, 88),
               4: (118, 90, 58), 5: (86, 84, 92), 6: (60, 54, 40), 7: (50, 66, 38)}
MOB_NAMES = {1001: "Marsh Rat", 1002: "Feral Ghoul", 1003: "Hollow Hound", 1004: "Plague Bat",
             1005: "Bonepicker Gnoll", 1006: "Charnel Widow", 1007: "Gravecaller",
             1008: "Revenant Sexton", 1009: "Gravemother", 1010: "Sepulcher Elite"}


def layer(m: dict, name: str) -> dict | None:
    return next((l for l in m["layers"] if l["name"] == name), None)


def spawn_point(ground: list[int], blocked: list[int], w: int, h: int) -> tuple[int, int]:
    """server/src/world.cpp:84-96 — first unblocked tile in a square spiral from (w/2, h/3)."""
    cx, cy = w // 2, h // 3
    for r in range(64):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if max(abs(dx), abs(dy)) != r:
                    continue
                x, y = cx + dx, cy + dy
                if 0 <= x < w and 0 <= y < h and not blocked[y * w + x]:
                    return x, y
    return cx, cy


def spiral_first_free(blocked: list[int], w: int, h: int, at: tuple[int, int], taken: set, rmax: int = 8):
    """world.cpp:350-392 / 1030-1046 spiral (full square, not ring) from `at`."""
    ax, ay = at
    for r in range(rmax):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                x, y = ax + dx, ay + dy
                if 0 <= x < w and 0 <= y < h and not blocked[y * w + x] and (x, y) not in taken:
                    return x, y
    return None


def manifest(name: str) -> dict:
    m = json.loads((SRC / f"{name}.tmj").read_text())
    w, h = m["width"], m["height"]
    fg = m["tilesets"][0]["firstgid"]
    ground = [g - fg for g in layer(m, "ground")["data"]]
    ids = CRYPT_IDS if name == "thornwall_crypt" else STD_IDS
    # blocked: bhmap derives blockers from WALL id (+ explicit blockers layer in thornwall)
    blocked = [1 if t == 2 else 0 for t in ground]
    bl = layer(m, "blockers")
    if bl:
        blocked = [1 if (b or (g - fg) >= 0 and bl["data"][i] != 0) else 0 for i, (b, g) in enumerate(zip(blocked, layer(m, "ground")["data"]))]
    zl = layer(m, "zones")
    zones = sorted(set(zl["data"])) if zl else []

    hist = Counter(ground)
    total = w * h
    pairs = Counter()
    for y in range(h):
        for x in range(w):
            a = ground[y * w + x]
            if x + 1 < w:
                b = ground[y * w + x + 1]
                if a != b:
                    pairs[tuple(sorted((a, b)))] += 1
            if y + 1 < h:
                b = ground[(y + 1) * w + x]
                if a != b:
                    pairs[tuple(sorted((a, b)))] += 1

    spawners, portals = [], []
    for l in m["layers"]:
        if l["type"] != "objectgroup":
            continue
        for o in l["objects"]:
            props = {p["name"]: p["value"] for p in o.get("properties", [])}
            rect = {"x0": int(o["x"] // 64), "y0": int(o["y"] // 32),
                    "x1": int((o["x"] + o["width"]) // 64), "y1": int((o["y"] + o["height"]) // 32)}
            if l["name"] == "spawns":
                spawners.append({"name": o.get("name"), "mobId": props.get("mobId"),
                                 "mob": MOB_NAMES.get(props.get("mobId"), "?"),
                                 "maxAlive": props.get("maxAlive"), "respawnTicks": props.get("respawnTicks"), **rect})
            elif l["name"] == "portals":
                portals.append({"name": o.get("name"), "targetMapId": props.get("targetMapId"),
                                "targetX": props.get("targetX"), "targetY": props.get("targetY"), **rect})

    # runtime furniture (server-derived; art needs positions for pools/stalls)
    furniture = []
    sp = spawn_point(ground, blocked, w, h)
    if name == "thornwall":
        taken = set()
        marta = spiral_first_free(blocked, w, h, sp, taken); taken.add(marta)
        board = spiral_first_free(blocked, w, h, sp, taken); taken.add(board)
        anvil = spiral_first_free(blocked, w, h, sp, taken)
        furniture = [{"kind": 64, "name": "Marta", "x": marta[0], "y": marta[1]},
                     {"kind": 66, "name": "Wanted Board", "x": board[0], "y": board[1]},
                     {"kind": 65, "name": "Widow Anvil", "x": anvil[0], "y": anvil[1],
                      "note": "order of the three spirals is Marta -> Board -> Anvil per world.cpp; exact tie-breaks UNVERIFIED without running the server"}]
    if name == "thornwall_crypt":
        a = spiral_first_free(blocked, w, h, (44, 6), set())
        furniture = [{"kind": 65, "name": "Widow Anvil (bone barrow)", "x": a[0], "y": a[1]}]

    # scatter-density bands: 0 inside spawner rects, 1 within 3 tiles, 2 elsewhere
    band = [2] * total
    for s in spawners:
        for y in range(max(0, s["y0"] - 3), min(h, s["y1"] + 3)):
            for x in range(max(0, s["x0"] - 3), min(w, s["x1"] + 3)):
                inside = s["x0"] <= x < s["x1"] and s["y0"] <= y < s["y1"]
                band[y * w + x] = min(band[y * w + x], 0 if inside else 1)
    band_hist = Counter(band)

    # preview
    pv = Image.new("RGB", (w, h))
    px = pv.load()
    for y in range(h):
        for x in range(w):
            t = ground[y * w + x]
            px[x, y] = PREVIEW_RGB.get(t, (80, 80, 88)) if name != "thornwall_crypt" else \
                {0: (40, 40, 44), 1: (86, 84, 92), 2: (120, 120, 128), 3: (140, 130, 120), 4: (90, 70, 60), 5: (200, 150, 80)}[t]
    d = ImageDraw.Draw(pv)
    for s in spawners:
        d.rectangle([s["x0"], s["y0"], s["x1"] - 1, s["y1"] - 1], outline=(220, 60, 60))
    for p in portals:
        d.rectangle([p["x0"], p["y0"], p["x1"] - 1, p["y1"] - 1], outline=(80, 200, 255))
    for f in furniture:
        d.point((f["x"], f["y"]), fill=(255, 200, 60))
    d.point(sp, fill=(255, 255, 255))
    pv = pv.resize((w * 6, h * 6), Image.NEAREST)

    return {
        "map": name, "mapId": MAP_ID[name], "size": [w, h], "zone_dir": ZONE_DIR[name],
        "terrain_ids": {str(k): ids.get(k, str(k)) for k in sorted(hist)},
        "terrain_pct": {ids.get(k, str(k)): round(100 * v / total, 1) for k, v in hist.most_common()},
        "adjacency_pairs": [{"a": ids.get(a, str(a)), "b": ids.get(b, str(b)), "boundary_len": n}
                            for (a, b), n in pairs.most_common() if n >= 4],
        "zones": zones, "spawn_point": list(sp), "spawners": spawners, "portals": portals,
        "furniture_runtime": furniture,
        "scatter_bands_tiles": {"0_inside_spawner (least scatter)": band_hist[0],
                                "1_within_3_tiles": band_hist[1], "2_elsewhere (full scatter)": band_hist[2]},
        "preview": pv,
    }


def write(mf: dict) -> None:
    out = OUT / mf["zone_dir"]
    out.mkdir(parents=True, exist_ok=True)
    pv = mf.pop("preview")
    pv.save(out / f"MAPS_{mf['map']}_preview.png")
    (out / f"MAPS_{mf['map']}.json").write_text(json.dumps(mf, indent=1) + "\n")
    L = [f"# Art manifest — `{mf['map']}` (mapId {mf['mapId']}, {mf['size'][0]}×{mf['size'][1]})",
         "", "Generated by `tools/atlaspack/map_manifest.py` from `data/maps-src/" + mf["map"] + ".tmj` (read-only). "
         "Re-run when the map changes. Preview: `MAPS_" + mf["map"] + "_preview.png` (red = spawner rects, cyan = portals, yellow = runtime furniture, white = zone spawn point).", "",
         "## Plates needed (terrain id → share of tiles)", "",
         "| id | name | % |", "|---|---|---|"]
    inv = {v: k for k, v in mf["terrain_ids"].items()}
    for nm, pct in mf["terrain_pct"].items():
        L.append(f"| {inv.get(nm, '?')} | {nm} | {pct} |")
    L += ["", "## Edge sets needed (boundary length ≥ 4, descending)", "", "| pair | boundary tiles |", "|---|---|"]
    for p in mf["adjacency_pairs"]:
        L.append(f"| {p['a']} ↔ {p['b']} | {p['boundary_len']} |")
    L += ["", "## Spawner rects → scatter density (Mir rule: packs on plain ground)", "",
          "| spawner | mob | maxAlive | tiles x0..x1 / y0..y1 |", "|---|---|---|---|"]
    for s in mf["spawners"]:
        L.append(f"| {s['name']} | {s['mob']} ({s['mobId']}) | {s['maxAlive']} | {s['x0']}..{s['x1']} / {s['y0']}..{s['y1']} |")
    b = mf["scatter_bands_tiles"]
    L += ["", f"Scatter bands: inside spawners **{b['0_inside_spawner (least scatter)']}** tiles (band 0, ≤ 1 scatter per 16 tiles) · "
          f"within 3 tiles **{b['1_within_3_tiles']}** (band 1, ≤ 1 per 8) · elsewhere **{b['2_elsewhere (full scatter)']}** (band 2, composed clusters).", ""]
    L += ["## Portals (keep clear of fog band / letterbox)", "", "| portal | to map | target | tiles |", "|---|---|---|---|"]
    for p in mf["portals"]:
        L.append(f"| {p['name']} | {p['targetMapId']} | ({p['targetX']},{p['targetY']}) | {p['x0']}..{p['x1']} / {p['y0']}..{p['y1']} |")
    if mf["furniture_runtime"]:
        L += ["", "## Runtime furniture (server-placed; positions reproduced from world.cpp)", "", "| kind | name | tile |", "|---|---|---|"]
        for f in mf["furniture_runtime"]:
            L.append(f"| {f['kind']} | {f['name']} | ({f['x']},{f['y']}) |")
        L += ["", "Every warm source above gets a painted pool decal (R-NIGHT) in this folder's `pools/`."]
    L += ["", f"Zones present: {mf['zones']} · zone spawn point {tuple(mf['spawn_point'])} (world.cpp:84–96 spiral from (w/2, h/3))."]
    (out / f"MAPS_{mf['map']}.md").write_text("\n".join(L) + "\n")


def main(argv: list[str]) -> int:
    names = argv or list(MAP_ID)
    for n in names:
        mf = manifest(n)
        write(mf)
        print(f"{n}: {len(mf['spawners'])} spawners, {len(mf['portals'])} portals, "
              f"{len(mf['adjacency_pairs'])} edge pairs, furniture {[(f['name'], f['x'], f['y']) for f in mf['furniture_runtime']]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
