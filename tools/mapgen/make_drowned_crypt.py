#!/usr/bin/env python3
"""Drowned Crypt Depths (mapId 5) — S18 content drop. The Gravemother's hall.

48x36 drowned-burial maze below thornwall_crypt (zone 3, bone vault): a
processional causeway over black water between chapel halls. Eight
Sepulcher Elite camps ring the boss apse; the Gravemother presides last,
alone at the font. One portal pair (depths stairs <-> crypt bone vault).
Deterministic LCG like the other gens.
"""
import json
from pathlib import Path

W, H = 48, 36
TW, TH = 64, 32
GRASS, DIRT, WALL, WATER, WOOD, PATH, MUD, DARKGRASS = 0, 1, 2, 3, 4, 5, 6, 7
ZONE_CRYPT2 = 5
MOB_GRAVECALLER = 1007
MOB_REVENANT_SEXTON = 1008
MOB_GRAVEMOTHER = 1009
MOB_SEPULCHER_ELITE = 1010


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out = repo / "data" / "maps-src" / "drowned_crypt.tmj"

    # flooded bones: whole dungeon drowned except carved stone
    ground = [[WATER] * W for _ in range(H)]

    # entry chapel (SW) + processional causeway to the boss apse (N center)
    rect(ground, 2, 28, 9, 34, DIRT)
    rect(ground, 5, 8, 6, 28, DIRT)        # causeway north
    rect(ground, 5, 8, 42, 10, DIRT)       # east-west transept
    # west chapel (ghoulish side-altar) and east rows of tombs
    rect(ground, 12, 16, 20, 24, DIRT)
    rect(ground, 28, 16, 38, 24, DIRT)
    rect(ground, 12, 15, 20, 15, DIRT)
    rect(ground, 28, 15, 38, 15, DIRT)
    rect(ground, 40, 10, 42, 20, DIRT)
    # boss apse north of the transept
    rect(ground, 16, 2, 30, 8, DIRT)
    rect(ground, 4, 10, 8, 12, DIRT)

    # sludge strips hugging the waterline (damp-cracked edges)
    rect(ground, 2, 27, 9, 27, MUD)
    rect(ground, 30, 16, 38, 17, MUD)
    rect(ground, 18, 3, 26, 4, DARKGRASS)

    # drowned pews: WOOD boards as era props in the tomb rows
    lcg = 0xDEADCA55
    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n
    for _ in range(12):
        x = 13 + int(rnd(24))
        y = 16 + int(rnd(8))
        if ground[y][x] == DIRT:
            ground[y][x] = WOOD

    # rim walls except carved tiles; water hurts nobody but walks no one
    blocked = [[1 if ground[y][x] in (WALL, WATER) else 0 for x in range(W)] for y in range(H)]
    zones = [[ZONE_CRYPT2] * W for _ in range(H)]

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

    # eight elite camps ring the approach (T-064: elites x8)
    spawn("elite_entry_chapel", 3, 29, 4, 4, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_transept_west", 8, 8, 4, 2, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_transept_east", 38, 8, 4, 2, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_west_chapel", 13, 18, 4, 4, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_east_row_south", 33, 20, 4, 3, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_east_row_north", 33, 16, 4, 3, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_apse_l", 16, 4, 4, 3, MOB_SEPULCHER_ELITE, 1, 1200)
    spawn("elite_apse_r", 25, 4, 4, 3, MOB_SEPULCHER_ELITE, 1, 1200)
    # two gravecallers hold the causeway bend; a sexton tolls the apse
    spawn("causeway_bend", 8, 8, 4, 3, MOB_GRAVECALLER, 2, 1000)
    spawn("apse_sexton", 21, 5, 3, 3, MOB_REVENANT_SEXTON, 1, 1400)
    # the Gravemother presides: single spawn, long breath (T-064 boss x20)
    spawn("gravemother_font", 21, 2, 4, 3, MOB_GRAVEMOTHER, 1, 4000)

    # stairs back up to thornwall_crypt (zone 3) bone vault
    portal("depths_stairs_up", 2, 30, 1, 2, 3, 44, 6)

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
