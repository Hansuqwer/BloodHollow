#!/usr/bin/env python3
"""Thornwall Crypt (interior zone, mapId 3) - Phase 3 zone map.

Deterministic like make_thornwall.py (ADR-007 spawn + regen diff check).
48x36 stone interior: entry hall -> ossuary -> candle crypt -> bone barrow.
Exit portal on the entry-hall west edge returns to thornwall (map 1).
"""
import json
from pathlib import Path

W, H = 48, 36
TW, TH = 64, 32

STONE, FLOOR, WALL, SLAB, BONEPIT, CANDLE = 0, 1, 2, 3, 4, 5
ZONE_CRYPT_ENTRY, ZONE_CRYPT_DEEP, ZONE_CRYPT_SANCTUM = 10, 11, 12

MOB_FERAL_GHOUL = 1002
MOB_CHARNEL_WIDOW = 1006
MOB_GRAVECALLER = 1007
MOB_REVENANT_SEXTON = 1008


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if 0 <= x < W and 0 <= y < H:
                g[y][x] = v


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out = repo / "data" / "maps-src" / "thornwall_crypt.tmj"

    ground = [[STONE] * W for _ in range(H)]

    # rooms (floor) + 1-wide wall gaskets between them
    rect(ground, 0, 12, 12, 24, FLOOR)        # entry hall (west)
    rect(ground, 16, 14, 46, 22, FLOOR)       # ossuary (long central gallery)
    rect(ground, 16, 24, 30, 34, FLOOR)       # candle crypt (south)
    rect(ground, 34, 2, 46, 10, FLOOR)        # bone barrow (north-east)

    # corridors
    rect(ground, 12, 17, 16, 18, FLOOR)       # hall -> ossuary
    rect(ground, 22, 22, 23, 24, FLOOR)       # ossuary -> candle crypt
    rect(ground, 38, 10, 39, 14, FLOOR)       # ossuary -> bone barrow

    # doors as narrow slabs (walkable rubble, cosmetic)
    ground[17][14] = SLAB
    ground[23][24] = SLAB
    ground[12][39] = SLAB

    # dressing: candle rows in the crypt, bonepit pools in the barrow
    for cx in range(18, 30, 2):
        if ground[26][cx] == FLOOR:
            ground[26][cx] = CANDLE
    lcg = 0xC12C7

    def rnd(n):
        nonlocal lcg
        lcg = (lcg * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        return (lcg >> 33) % n

    for _ in range(10):
        x = 35 + rnd(10)
        y = 3 + rnd(6)
        if ground[y][x] == FLOOR:
            ground[y][x] = BONEPIT

    # solid rock = WALL id on the ground layer (bhmap derives blockers from terrain)
    for y in range(H):
        for x in range(W):
            if ground[y][x] == STONE:
                ground[y][x] = WALL
    blocked = [[1 if ground[y][x] == WALL else 0 for x in range(W)] for y in range(H)]

    zones = [[0] * W for _ in range(H)]
    rect(zones, 0, 12, 12, 24, ZONE_CRYPT_ENTRY)
    rect(zones, 16, 14, 46, 22, ZONE_CRYPT_DEEP)
    rect(zones, 16, 24, 30, 34, ZONE_CRYPT_DEEP)
    rect(zones, 34, 2, 46, 10, ZONE_CRYPT_SANCTUM)

    # match thornwall.tmj conventions exactly: "spawns" objectgroup (type
    # "spawner", respawnTicks) + "portals" objectgroup (type "portal").
    def obj(oid, name, otype, tx, ty, tw, th, props):
        return {
            "id": oid, "name": name, "type": otype,
            "x": tx * TW, "y": ty * TH, "width": tw * TW, "height": th * TH,
            "rotation": 0, "visible": True,
            "properties": [{"name": k, "type": "int" if isinstance(v, int) else "string",
                            "value": v} for k, v in props],
        }

    spawns = []
    portals = []
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

    spawn("ghoul_racks", 18, 15, 8, 6, MOB_FERAL_GHOUL, 7, 600)
    spawn("widow_cocoon", 24, 26, 6, 6, MOB_CHARNEL_WIDOW, 6, 850)
    spawn("gravecaller_pulpit", 42, 3, 4, 4, MOB_GRAVECALLER, 3, 1100)
    spawn("sexton_seat", 44, 5, 2, 3, MOB_REVENANT_SEXTON, 1, 6000)
    portal("stairs_up", 0, 17, 1, 2, 1, 10, 11)  # back to Thornwall chapel hatch (tiles)

    ground_data = [v + 1 for row in ground for v in row]

    def tilelayer(i, name, data):
        return {"data": data, "height": H, "id": i, "name": name, "opacity": 1,
                "type": "tilelayer", "visible": True, "width": W, "x": 0, "y": 0}

    def objlayer(i, name, objs):
        return {"draworder": "topdown", "id": i, "name": name, "objects": objs,
                "opacity": 1, "type": "objectgroup", "visible": True, "x": 0, "y": 0}

    tmj = {
        "compressionlevel": -1, "height": H,
        "infinite": False, "layers": [
            tilelayer(1, "ground", ground_data),
            tilelayer(3, "zones", [v for row in zones for v in row]),
            objlayer(4, "spawns", spawns),
            objlayer(5, "portals", portals),
        ],
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
    spawners = len(spawns)
    print(f"make_thornwall_crypt: wrote {out} ({W}x{H}, walkable {walk:.1f}%, spawners {spawners})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
