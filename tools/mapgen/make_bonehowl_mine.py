#!/usr/bin/env python3
"""Bonehowl Mine (mapId 4) — S18 content drop. Cave chart: needful bad light.

56x40 worked-out tunnel complex: an entrance shaft from Bleak Fields east
edge, three galleries off a central drift, bone-pile middens and an old
ore spur. Mid-tier packs (hounds, gnolls, one widow vein). Deterministic
LCG exactly like the other gens. One portal pair (mine_mouth <-> fields).
"""
import json
from pathlib import Path

W, H = 56, 40
TW, TH = 64, 32
GRASS, DIRT, WALL, WATER, WOOD, PATH, MUD, DARKGRASS = 0, 1, 2, 3, 4, 5, 6, 7
ZONE_MINE = 4
MOB_HOLLOW_HOUND = 1003
MOB_PLAGUE_BAT, MOB_BONEPICKER_GNOLL = 1004, 1005
MOB_CHARNEL_WIDOW = 1006
MOB_RED_WIDOW = 1013  # T-102 mine elite (Widow base, 45-min rotation)


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out = repo / "data" / "maps-src" / "bonehowl_mine.tmj"

    # cave pitch-dark canvas: everything starts as rock, we carve rooms
    ground = [[WALL] * W for _ in range(H)]

    # entrance shaft SW at mouth of gallery 1
    rect(ground, 2, 30, 8, 36, DIRT)
    # gallery 1 (west) + drift east
    rect(ground, 2, 24, 6, 36, DIRT)
    rect(ground, 4, 24, 52, 27, DIRT)      # main drift E-W at y24..27
    # gallery 2 (north center) + gallery 3 (north-east elbow)
    rect(ground, 22, 8, 30, 24, DIRT)
    rect(ground, 40, 12, 50, 24, DIRT)
    # ore spur south-east from the drift
    rect(ground, 44, 27, 47, 35, DIRT)
    rect(ground, 40, 33, 47, 35, DIRT)

    # muddied workings (damp-cracked floor) + a widow vein
    rect(ground, 24, 10, 28, 14, MUD)
    rect(ground, 44, 14, 48, 18, DARKGRASS)

    # bone middens: single-tile WOOD "props" (era brown boards) along walls
    lcg = 0xB0E911
    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n
    for _ in range(14):
        x = 6 + int(rnd(40))
        y = 10 + int(rnd(24))
        if ground[y][x] == DIRT and ground[y - 1][x] == WALL:
            ground[y][x] = WOOD

    blocked = [[1 if ground[y][x] in (WALL, WATER) else 0 for x in range(W)] for y in range(H)]
    zones = [[ZONE_MINE] * W for _ in range(H)]

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

    # packs: bats ride the entrance air; hounds guard the drift; gnolls hold
    # gallery 2+3; one widow veins the NE damp
    spawn("bats_mouth", 3, 30, 5, 5, MOB_PLAGUE_BAT, 10, 300)
    spawn("hounds_drift_west", 10, 24, 8, 4, MOB_HOLLOW_HOUND, 8, 420)
    spawn("hounds_drift_east", 36, 24, 8, 4, MOB_HOLLOW_HOUND, 8, 420)
    spawn("gnolls_gallery2", 23, 10, 7, 5, MOB_BONEPICKER_GNOLL, 9, 600)
    spawn("gnolls_gallery3", 42, 14, 7, 5, MOB_BONEPICKER_GNOLL, 9, 600)
    spawn("widow_vein", 44, 14, 5, 4, MOB_CHARNEL_WIDOW, 4, 800)
    # T-102 Red Widow nest: gallery-3 elbow floor below the gnoll rect,
    # against the widow-vein darkgrass. maxAlive 1, 45-min rotation (54000t).
    spawn("red_widow_nest", 44, 19, 4, 3, MOB_RED_WIDOW, 1, 54000)
    spawn("hounds_ore_spur", 41, 33, 6, 3, MOB_HOLLOW_HOUND, 6, 420)

    # mine mouth back to Bleak Fields east edge (pair lives on that map)
    portal("mine_mouth_out", 2, 31, 1, 3, 2, 62, 20)

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
    out.write_text(json.dumps(tmj, indent=1))
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
