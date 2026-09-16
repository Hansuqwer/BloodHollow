#!/usr/bin/env python3
"""Generates data/maps-src/weeping_castle.tmj - the placeholder MVP castle map.

H1 stub (mapId 6): outer wall ring with a south gate, dirt courtyard, a small
keep (wall ring + wood floor + door gap, chapel pattern from make_thornwall.py)
with a blocked Heartstone plinth placeholder, plus a one-way gate portal back
to Thornwall (H2 adds the return leg). Deterministic by construction (fixed
LCG, no time/env input); uses the same layer convention as the other
generators (see tools/mapconv/main.cpp) and the same terrain ids so
tools/mapgen/validate_links.py WALL_IDS stays valid.
"""
import argparse
import json
from pathlib import Path

W, H = 40, 30       # tiles
TW, TH = 64, 32     # 2:1 iso tiles in pixels

# terrain ids (ground layer stores id + 1; 0 = empty -> grass) — same as thornwall
GRASS, DIRT, WALL, WATER, WOOD, PATH, MUD, DARKGRASS = 0, 1, 2, 3, 4, 5, 6, 7
# zone ids (zones layer stores id + 1; 0 = none)
ZONE_CASTLE = 4

# placeholder mob ids (shared with make_thornwall.py until monsters.json v2)
MOB_FERAL_GHOUL, MOB_HOLLOW_HOUND = 1002, 1003

# keep footprint (wall ring) and interior — mirrored in World::spawnSteward
KEEP_X0, KEEP_Y0, KEEP_X1, KEEP_Y1 = 16, 4, 23, 9


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    ap = argparse.ArgumentParser()
    repo = Path(__file__).resolve().parents[2]
    ap.add_argument("--out", default=str(repo / "data" / "maps-src" / "weeping_castle.tmj"))
    args = ap.parse_args()

    ground = [[GRASS] * W for _ in range(H)]

    # outer wall ring with a south gate gap (2 wide, x 19-20)
    for x in range(W):
        ground[0][x] = WALL
        ground[H - 1][x] = WALL
    for y in range(H):
        ground[y][0] = WALL
        ground[y][W - 1] = WALL
    ground[H - 1][19] = PATH
    ground[H - 1][20] = PATH

    # courtyard dirt + gate road up to the keep door
    rect(ground, 12, 12, 27, 24, DIRT)
    rect(ground, 19, 10, 20, 28, PATH)

    # keep: wall ring, wood floor, door gap on the south side (chapel pattern)
    rect(ground, KEEP_X0, KEEP_Y0, KEEP_X1, KEEP_Y1, WALL)
    rect(ground, KEEP_X0 + 1, KEEP_Y0 + 1, KEEP_X1 - 1, KEEP_Y1 - 1, WOOD)
    ground[KEEP_Y1][19] = DIRT
    ground[KEEP_Y1][20] = DIRT
    # Heartstone plinth placeholder (blocked center tile; H2 makes it a capture)
    ground[6][19] = WALL

    # a few yard stones for cover (fixed LCG, kept off road/keep/courtyard heart)
    lcg = 0x5EED

    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n

    placed = 0
    while placed < 4:
        x = 2 + rnd(W - 4)
        y = 12 + rnd(14)
        if ground[y][x] == GRASS:
            ground[y][x] = WALL
            placed += 1

    # blockers: derived from terrain (walls, water)
    blocked = [[1 if ground[y][x] in (WALL, WATER) else 0 for x in range(W)] for y in range(H)]

    # zones: the whole interior is castle ground
    zones = [[ZONE_CASTLE] * W for _ in range(H)]

    ground_data = [v + 1 for row in ground for v in row]
    block_data = [1 if b else 0 for row in blocked for b in row]
    zone_data = [v + 1 if v else 0 for row in zones for v in row]

    def prop(k, v):
        return {"name": k, "type": "int", "value": v}

    oid = 0

    def next_oid():
        nonlocal oid
        oid += 1
        return oid

    def spawn(name, x, y, w, h, mob, alive, respawn, nightOnly=0):
        return {
            "id": next_oid(), "name": name, "type": "spawner",
            "x": x * TW, "y": y * TH, "width": w * TW, "height": h * TH,
            "rotation": 0, "visible": True,
            "properties": [prop("mobId", mob), prop("maxAlive", alive),
                           prop("respawnTicks", respawn),
                           prop("nightOnly", nightOnly)],
        }

    def portal(name, x, y, w, h, target_map, tx, ty):
        return {
            "id": next_oid(), "name": name, "type": "portal",
            "x": x * TW, "y": y * TH, "width": w * TW, "height": h * TH,
            "rotation": 0, "visible": True,
            "properties": [prop("targetMapId", target_map),
                           prop("targetX", tx), prop("targetY", ty)],
        }

    # rehearsal bodies: yard watch + kennel pack (small, era-cheap)
    spawns = [
        spawn("yard_watch", 12, 14, 8, 6, MOB_FERAL_GHOUL, 4, 500),
        spawn("kennel_hounds", 24, 18, 6, 5, MOB_HOLLOW_HOUND, 3, 700),
    ]
    # one-way gate leg back to Thornwall east road (21,14 is PATH there);
    # H2 adds the Thornwall->castle return portal.
    portals = [
        portal("castle_gate", 19, 27, 2, 2, 1, 21, 14),
    ]

    def tilelayer(i, name, data):
        return {"data": data, "height": H, "id": i, "name": name, "opacity": 1,
                "type": "tilelayer", "visible": True, "width": W, "x": 0, "y": 0}

    def objectgroup(i, name, objects):
        return {"draworder": "topdown", "id": i, "name": name, "objects": objects,
                "opacity": 1, "type": "objectgroup", "visible": True, "x": 0, "y": 0}

    tmj = {
        "compressionlevel": -1, "height": H, "infinite": False,
        "layers": [
            tilelayer(1, "ground", ground_data),
            tilelayer(2, "blockers", block_data),
            tilelayer(3, "zones", zone_data),
            objectgroup(4, "spawns", spawns),
            objectgroup(5, "portals", portals),
        ],
        "nextlayerid": 6, "nextobjectid": oid + 1,
        "orientation": "isometric", "renderorder": "right-down",
        "tiledversion": "1.10.2", "tileheight": TH,
        "tilesets": [{"columns": 8, "firstgid": 1, "margin": 0,
                      "name": "placeholder_terrain", "spacing": 0,
                      "tilecount": 8, "tileheight": TH, "tilewidth": TW}],
        "tilewidth": TW, "type": "map", "version": "1.10", "width": W,
    }

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump(tmj, f, separators=(",", ":"))
        f.write("\n")

    total = W * H
    walkable = sum(1 for row in blocked for b in row if not b)
    print(f"make_weeping_castle: wrote {out} ({W}x{H}, walkable "
          f"{100.0 * walkable / total:.1f}%, spawners {len(spawns)}, portals {len(portals)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
