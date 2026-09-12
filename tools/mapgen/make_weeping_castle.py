#!/usr/bin/env python3
"""Weeping Castle (mapId 6) — T-123 B3: the siege stage.

56x44 fortress on a bleak moor, south of Bleak Fields East. A moat band with
two causeways fronts the south wall; TWO 1-tile gate gaps (west 20,34 /
east 36,34 — the destructible gates stand on them, code-spawned) open into
the courtyard. The Heartstone rests at the crossroads (28,20); the keep nook
north shelters the throne (28,11). Four Oathbroken Sentinel spawners (the
dead garrison) hold gates, heart and keep steps; a Hollow Hound pack hunts
the approach moor. One portal pair (moor road <-> fields east edge).

B4 (siege law) owns gate HP damage rules, the crown channel and ownership;
B3 ships geometry + furniture + defenders only. Deterministic LCG like the
other gens.
"""
import json
from pathlib import Path

W, H = 56, 44
TW, TH = 64, 32
GRASS, DIRT, WALL, WATER, WOOD, PATH, MUD, DARKGRASS = 0, 1, 2, 3, 4, 5, 6, 7
ZONE_CASTLE = 6
MOB_HOLLOW_HOUND = 1003
MOB_OATHBROKEN_SENTINEL = 1015  # T-123: the dead garrison


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out = repo / "data" / "maps-src" / "weeping_castle.tmj"

    # the moor: bleak grass to every edge
    ground = [[DARKGRASS] * W for _ in range(H)]

    # courtyard floor (drawn first; walls and keep go over it)
    rect(ground, 14, 9, 42, 33, DIRT)
    # processional roads: north-south from the gates to the keep, east-west
    # across the heart
    rect(ground, 27, 14, 29, 33, PATH)
    rect(ground, 15, 20, 41, 20, PATH)
    rect(ground, 20, 33, 21, 33, PATH)   # west gate landing
    rect(ground, 36, 33, 37, 33, PATH)   # east gate landing

    # curtain wall: rect (13,8)-(43,34), then the two gate gaps cut through
    rect(ground, 13, 8, 43, 8, WALL)     # north face
    rect(ground, 13, 34, 43, 34, WALL)   # south face
    rect(ground, 13, 8, 13, 34, WALL)    # west face
    rect(ground, 43, 8, 43, 34, WALL)    # east face
    ground[34][20] = PATH                # WEST GATE gap (gate entity at 20,34)
    ground[34][36] = PATH                # EAST GATE gap (gate entity at 36,34)

    # moat band with the two causeways (gates' shadows)
    rect(ground, 13, 35, 43, 35, WATER)
    ground[35][20] = PATH
    ground[35][36] = PATH

    # the keep: walled nook north-center, throne room within, south opening
    rect(ground, 24, 9, 32, 13, WALL)
    rect(ground, 25, 10, 31, 12, DIRT)
    ground[13][27] = PATH                # keep door (2-wide with 28)
    ground[13][28] = PATH

    # approach road from the moor portal (SW) to the west causeway
    rect(ground, 2, 36, 2, 40, PATH)
    rect(ground, 3, 38, 19, 38, PATH)
    rect(ground, 3, 39, 19, 39, MUD)     # trampled verge beside the road

    # deterministic siege debris: rotting boards in the courtyard
    lcg = 0x0BEA75CA5
    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n
    for _ in range(14):
        x = 15 + int(rnd(27))
        y = 15 + int(rnd(18))
        if ground[y][x] == DIRT:
            ground[y][x] = WOOD

    blocked = [[1 if ground[y][x] in (WALL, WATER) else 0 for x in range(W)] for y in range(H)]
    zones = [[ZONE_CASTLE] * W for _ in range(H)]

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

    # the dead garrison: Oathbroken Sentinels at gates, heart and keep steps
    spawn("gate_west_watch", 16, 30, 4, 3, MOB_OATHBROKEN_SENTINEL, 1, 1200)
    spawn("gate_east_watch", 36, 30, 4, 3, MOB_OATHBROKEN_SENTINEL, 1, 1200)
    spawn("heart_ring", 24, 16, 4, 4, MOB_OATHBROKEN_SENTINEL, 1, 1200)
    spawn("keep_steps", 25, 13, 7, 2, MOB_OATHBROKEN_SENTINEL, 1, 1400)
    # hounds hunt the approach moor
    spawn("moor_pack", 36, 39, 6, 4, MOB_HOLLOW_HOUND, 2, 900)

    # the moor road back to Bleak Fields East (zone 2, east edge)
    portal("castle_road_out", 2, 41, 1, 2, 2, 61, 10)

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
