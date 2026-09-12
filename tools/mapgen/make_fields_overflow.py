#!/usr/bin/env python3
"""Bleak Fields East (mapId 2) - overflow hunting map reached via east_gate.

64x48 open grassland with a hedge maze, a standing-stone circle, and two
harder camps westward; west portal returns to thornwall, east edge carries
the mine mouth (S18) and the Weeping Castle road (T-123). Deterministic
(fixed LCG) like the other gens.
"""
import json
from pathlib import Path

W, H = 64, 48
TW, TH = 64, 32
GRASS, DIRT, WALL, WATER, WOOD, PATH, MUD, DARKGRASS = 0, 1, 2, 3, 4, 5, 6, 7
ZONE_FIELDS = 2
MOB_PLAGUE_BAT, MOB_BONEPICKER_GNOLL = 1004, 1005
MOB_CHARNEL_WIDOW, MOB_GRAVECALLER = 1006, 1007
MOB_OLD_MAW = 1012  # T-101 fields elite (Gnoll base, 30-min rotation)


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out = repo / "data" / "maps-src" / "fields_overflow.tmj"

    ground = [[GRASS] * W for _ in range(H)]

    # roads: west road back through the gate line y14-15; north spur
    rect(ground, 0, 14, 40, 15, PATH)
    rect(ground, 10, 6, 11, 40, PATH)

    # hedge maze NW quadrant
    for x in range(18, 40, 2):
        rect(ground, x, 6, x, 22, WALL)
    for x in range(20, 38, 4):
        ground[14][x] = GRASS  # maze gates

    # standing stone circle E
    lcg = 0xF13D5
    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n
    for i in range(10):
        x = int(48 + 9 * ((i * 7) % 10 / 10.0))
        y = int(24 + 9 * ((i * 3) % 10 / 10.0))
        if ground[y][x] == GRASS:
            ground[y][x] = WALL

    # gnoll camp pits NE, bat ruin W, widow hollow SE
    rect(ground, 48, 6, 60, 12, DARKGRASS)
    rect(ground, 6, 28, 16, 38, DARKGRASS)
    rect(ground, 44, 34, 58, 44, MUD)

    # border walls; gate opening west at (0,14-15)
    for x in range(W):
        ground[0][x] = WALL
        ground[H - 1][x] = WALL
    for y in range(H):
        ground[y][0] = WALL
        ground[y][W - 1] = WALL
    ground[14][0] = PATH
    ground[15][0] = PATH
    ground[20][W - 1] = PATH   # east mouth: Bonehowl Mine steps (S18)
    ground[21][W - 1] = PATH
    ground[10][W - 1] = PATH   # east mouth: Weeping Castle road (T-123)
    ground[11][W - 1] = PATH

    blocked = [[1 if ground[y][x] in (WALL, WATER) else 0 for x in range(W)] for y in range(H)]
    zones = [[ZONE_FIELDS] * W for _ in range(H)]

    def obj(oid, name, otype, tx, ty, tw, th, props):
        return {"id": oid, "name": name, "type": otype,
                "x": tx * TW, "y": ty * TH, "width": tw * TW, "height": th * TH,
                "rotation": 0, "visible": True,
                "properties": [{"name": k, "type": "int" if isinstance(v, int) else "string",
                                "value": v} for k, v in props]}

    spawns, portals = [], []
    oid = 1
    def spawn(name, tx, ty, tw, th, mob, alive, respawn_ticks):
        nonlocal oid
        spawns.append(obj(oid, name, "spawner", tx, ty, tw, th,
                          [("mobId", mob), ("maxAlive", alive), ("respawnTicks", respawn_ticks)]))
        oid += 1
    def portal(name, tx, ty, tw, th, target_map, tx2, ty2):
        nonlocal oid
        portals.append(obj(oid, name, "portal", tx, ty, tw, th,
                           [("targetMapId", target_map), ("targetX", tx2), ("targetY", ty2)]))
        oid += 1

    spawn("bats_ruin", 6, 28, 10, 9, MOB_PLAGUE_BAT, 9, 360)
    spawn("gnoll_camp_north", 49, 7, 9, 5, MOB_BONEPICKER_GNOLL, 7, 650)
    spawn("widows_stone_circle", 46, 22, 12, 6, MOB_CHARNEL_WIDOW, 6, 800)
    spawn("gravecaller_hedge", 22, 30, 8, 5, MOB_GRAVECALLER, 3, 1000)
    # T-101 Old Maw pit: open south-center grass, off the roads and camps.
    # maxAlive 1, 30-min rotation (36000t — GDD 15-60 band, deterministic).
    spawn("old_maw_pit", 28, 38, 6, 4, MOB_OLD_MAW, 1, 36000)
    portal("west_gate_back", 0, 14, 1, 2, 1, 61, 14)
    portal("mine_mouth_in", 63, 20, 1, 2, 4, 4, 31)   # Bonehowl Mine mouth (S18)
    portal("castle_road_in", 63, 10, 1, 2, 6, 2, 40)  # Weeping Castle moor (T-123)

    ground_data = [v + 1 for row in ground for v in row]

    def tilelayer(i, name, data):
        return {"data": data, "height": H, "id": i, "name": name, "opacity": 1,
                "type": "tilelayer", "visible": True, "width": W, "x": 0, "y": 0}
    def objlayer(i, name, objs):
        return {"draworder": "topdown", "id": i, "name": name, "objects": objs,
                "opacity": 1, "type": "objectgroup", "visible": True, "x": 0, "y": 0}

    tmj = {
        "compressionlevel": -1, "height": H, "infinite": False,
        "layers": [tilelayer(1, "ground", ground_data),
                   tilelayer(3, "zones", [v for row in zones for v in row]),
                   objlayer(4, "spawns", spawns),
                   objlayer(5, "portals", portals)],
        "nextlayerid": 6, "nextobjectid": oid, "orientation": "isometric",
        "renderorder": "right-down", "tiledversion": "1.10.2",
        "tileheight": TH, "tilewidth": TW,
        "tilesets": [{"columns": 8, "firstgid": 1, "margin": 0,
                      "name": "placeholder_terrain", "spacing": 0,
                      "tilecount": 8, "tileheight": TH, "tilewidth": TW}],
        "type": "map", "version": "1.10", "width": W,
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(tmj, indent=1) + "\n")
    walk = 100.0 * (1 - sum(sum(r) for r in blocked) / (W * H))
    print(f"make_fields_overflow: wrote {out} ({W}x{H}, walkable {walk:.1f}%, spawners {len(spawns)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
