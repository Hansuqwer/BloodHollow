#!/usr/bin/env python3
"""Generates data/maps-src/thornwall.tmj - the placeholder MVP town map.

Deterministic by construction (fixed LCG, no time/env input). CI regenerates
and diffs against the committed file, so hand-editing the .tmj without editing
this script fails the build (ADR-007). Humans will author the real maps in
Tiled using the same layer convention (see tools/mapconv/main.cpp).
"""
import argparse
import json
from pathlib import Path

W, H = 64, 48       # tiles
TW, TH = 64, 32     # 2:1 iso tiles in pixels

# terrain ids (ground layer stores id + 1; 0 = empty -> grass)
GRASS, DIRT, WALL, WATER, WOOD, PATH, MUD, DARKGRASS = 0, 1, 2, 3, 4, 5, 6, 7
# zone ids (zones layer stores id + 1; 0 = none)
ZONE_TOWN, ZONE_FIELDS, ZONE_MARSH = 1, 2, 3

# placeholder mob ids until shared/data/monsters.json lands in Phase 2
MOB_MARSH_RAT, MOB_FERAL_GHOUL, MOB_HOLLOW_HOUND = 1001, 1002, 1003
MOB_PLAGUE_BAT, MOB_BONEPICKER_GNOLL = 1004, 1005
MOB_CHARNEL_WIDOW, MOB_GRAVECALLER = 1006, 1007


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    ap = argparse.ArgumentParser()
    repo = Path(__file__).resolve().parents[2]
    ap.add_argument("--out", default=str(repo / "data" / "maps-src" / "thornwall.tmj"))
    args = ap.parse_args()

    ground = [[GRASS] * W for _ in range(H)]

    # river across the south, with a stone bridge on the south road
    rect(ground, 0, 34, W - 1, 37, WATER)
    rect(ground, 14, 34, 16, 37, PATH)

    # town plaza
    rect(ground, 8, 8, 20, 16, DIRT)

    # chapel: wall ring, wood floor, door gap on the south side
    rect(ground, 9, 9, 14, 13, WALL)
    rect(ground, 10, 10, 13, 12, WOOD)
    ground[13][11] = DIRT
    ground[13][12] = DIRT

    # the well (town landmark, blocked)
    ground[12][16] = WALL

    # roads: east road (to the fields), south road (to bridge and marsh)
    rect(ground, 21, 14, 62, 15, PATH)
    rect(ground, 14, 17, 15, 33, PATH)
    rect(ground, 14, 38, 15, 46, PATH)

    # border walls with an east gate on the road rows
    for x in range(W):
        ground[0][x] = WALL
        ground[H - 1][x] = WALL
    for y in range(H):
        ground[y][0] = WALL
        ground[y][W - 1] = WALL
    ground[14][W - 1] = PATH
    ground[15][W - 1] = PATH

    # graveyard west of the chapel: dark grass + gravestone rows (blocked)
    rect(ground, 4, 9, 7, 15, DARKGRASS)
    for gy in (10, 13):
        ground[gy][5] = WALL
        ground[gy][7] = WALL

    # market stalls on the plaza's north edge (blocked boxes)
    ground[8][10] = WALL
    ground[8][12] = WALL
    ground[8][14] = WALL

    # marsh mud banks + field grass variation (cosmetic terrain types)
    rect(ground, 8, 40, 24, 45, MUD)
    rect(ground, 26, 39, 34, 46, MUD)
    rect(ground, 44, 10, 50, 14, DARKGRASS)
    rect(ground, 55, 19, 60, 23, DARKGRASS)

    # field fence with a gap, plus a few scattered standing stones (fixed LCG)
    lcg = 0xC0FFEE

    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n

    for x in range(40, 52):
        ground[8][x] = WALL
    ground[8][45] = GRASS
    ground[8][46] = GRASS
    for _ in range(9):
        x = 40 + rnd(20)
        y = 9 + rnd(15)
        if ground[y][x] == GRASS:
            ground[y][x] = WALL

    # blockers: derived from terrain (walls, water)
    blocked = [[1 if ground[y][x] in (WALL, WATER) else 0 for x in range(W)] for y in range(H)]

    # zones: town safe zone, east fields, southern marsh
    zones = [[0] * W for _ in range(H)]
    rect(zones, 8, 8, 20, 16, ZONE_TOWN)
    rect(zones, 21, 7, 62, 24, ZONE_FIELDS)
    rect(zones, 4, 39, 58, 46, ZONE_MARSH)
    rect(zones, 4, 9, 7, 15, ZONE_TOWN)  # graveyard is chapel grounds (safe)

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

    # Density rule (S5 lesson + GDD): throughput >= concurrent hunters.
    # Bleak Fields pack: starters near town, mid-tier deeper, bat cave swarm.
    spawns = [
        spawn("rats_east", 42, 10, 8, 6, MOB_MARSH_RAT, 10, 400),
        spawn("rats_field", 18, 42, 8, 5, MOB_MARSH_RAT, 8, 400),
        spawn("bats_cryptyard", 12, 4, 7, 5, MOB_PLAGUE_BAT, 9, 360),
        spawn("ghouls_east", 52, 17, 6, 5, MOB_FERAL_GHOUL, 8, 500),
        spawn("ghouls_orchard", 44, 26, 7, 5, MOB_FERAL_GHOUL, 7, 500),  # S8 validator catch: was in the river
        spawn("ghouls_south_road", 44, 38, 6, 4, MOB_FERAL_GHOUL, 6, 500),
        spawn("gnolls_pits", 50, 42, 8, 4, MOB_BONEPICKER_GNOLL, 6, 700),
        spawn("hounds_marsh", 10, 40, 10, 6, MOB_HOLLOW_HOUND, 8, 700),
        spawn("widow_glade", 58, 42, 6, 4, MOB_CHARNEL_WIDOW, 5, 850),
        # T-068 (director option A): the L11 barricade moved OFF the bridge
        # approach.  Old rect (6,18) put the gravecaller kill zone (leash 12
        # around the anchor, aggro 7) over the south road x[14,15] -- the only
        # crossing to the L7+ content south of the river, which is what walled
        # campaign progression at L6/L7 (devlog 0029 "scope boundary").  New
        # rect x=1 y[43,45]: even the worst in-rect anchor leaves mob reach
        # (wander 5 + aggro 7) >= 8 tiles from every route tile, day and
        # night.  The L11 stays as a far-marsh hazard, not a roadblock.
        spawn("gravecaller_barricade", 1, 43, 1, 3, MOB_GRAVECALLER, 3, 1000),
        # T-071 night light: one night-bound ghoul pack in the quiet middle
        # fields (x36-41, y26-29: grass gap between town and the orchard,
        # off the east/south roads and the campaign line). By day the rect
        # stands empty; after 21:00 it refills and hunts (maxAlive 4).
        spawn("night_ghouls", 36, 26, 6, 4, MOB_FERAL_GHOUL, 4, 600, 1),
    ]
    portals = [
        portal("east_gate", 63, 14, 1, 2, 2, 2, 14),   # -> fields map (Phase 2)
        portal("crypt_hatch", 10, 10, 1, 1, 3, 24, 30),  # chapel interior -> crypt (Phase 3)
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
    print(f"make_thornwall: wrote {out} ({W}x{H}, walkable "
          f"{100.0 * walkable / total:.1f}%, spawners {len(spawns)}, portals {len(portals)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
