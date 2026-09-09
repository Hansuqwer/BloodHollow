# 0034 — Night light: torch + Blessed Lantern + night-only spawns (T-071)

S22b of the overnight queue (day/night completion, second of two cards).
Greenfield: no `lightRadius`/torch/`nightOnly` existed anywhere.

## Scope as pinned

- **Server:** `Entity.lightRadius` (u8, 0), `lightUntil`, `lanternLit`.
  Torch (3003, 8g, stack 8): `useItem` consumes one → radius 6 for 300 s
  (6000 ticks, exact edge), torch wins over lantern. Blessed Lantern (3004,
  150g, stack 1): never consumed, `/use` toggles radius 8 on/off,
  `lightUntil -1` (never expires while held). Tick-edge expiry for torches.
  Potions/gold numbers untouched.
- **Acquisition (judgment call):** Marta stocks both (torch 8g brief-pinned;
  lantern 150g pinned by the overnight shift — no brief basis, director
  review). No drop-table churn. Panel: Marta rows F1–F7 (size-driven loop,
  capped at F12), Sable's crate shifts F6–F8 → **F8–F10**. The stat-assign
  F5/F6/F7 double-bind when points>0 pre-exists (same class, noted).
- **Client render:** after the night overlay, one `DrawCircleGradient`
  per lit AoI entity (radius = tiles × 48 px, peak alpha 90). Blending
  toward warm can only lighten — the T-062 tint floor (alpha 150) is never
  pushed darker, by construction. Day: mask skipped. No mask wire protocol
  (render-only); source tiles ride `EntitySpawn`/`EntityDelta` +1 trailing
  `u8 light` (per-tick deltas propagate torch expiry in ≤1 tick).
- **Night-only spawns:** `SpawnDef.nightOnly u8` + `.bhmap` v1→**v2** wire
  (tail byte; loader stays strict, maps regenerate at build) + mapconv
  `nightOnly` prop + all five `.tmj` regenerated (only thornwall differs).
  `night_ghouls` (feral ghouls, maxAlive 4, respawn 600) in the quiet middle
  fields (36,26,6,4 — grass gap off the roads and the campaign line).
  Gate: no initial/refill and no acquire outside 21:00–05:00 (tick-derived,
  same schedule as T-061); held targets kept.
- **Build fix bundled:** only thornwall re-synced at build — the v2 change
  left zones 2–5 stale-unloadable (caught by test_s18). `bh_maps` now syncs
  all five maps. `.bhmap` files stay git-ignored artifacts.
- Epoch **9 → 10** + fresh-leg replay.

## Premise corrections (brief vs tree)

1. No new `kUse` command — `kUseItem` is already journaled and the client
   already sends it for any slot-2 click. Torch/lantern branch inside
   `useItem` by item id.
2. vfx kind-14 is HASTE, not a free violet-eye slot (same finding as T-070);
   the mask needs no combat kinds.
3. Wire version stays 237 (field appends; S16 precedent, stated). bhmap has
   its own version law and DID bump (1→2).

## Soak + replay

Fresh 560 s grinder mix (port 7873, fresh DB `/tmp/t071.db`, epoch-10
journal `logs/t071.bwj`), same 14-bot shape:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 45 | **0** | 3 | killerByLvl empty; mend=8 live |
| fighter ×4 | 109 | 52 | 4 | spread, levelDrops 5 |
| pilgrim ×3 | 122 | 48 | 4 | spread, levelDrops 5 |
| wander ×5 | 0 | 24 | 1 | decoys |

Bands: Rat 4.7 / Bat 3.5 / Ghoul 13.2 / Hound 56.7 (n=9) / Gnoll 80.0 (n=6)
/ Gravecaller 125.0 (n=3) — ordering preserved; mid-band wobble is now a
three-leg standing watch item (see 0032/0033), still noise-shaped, still no
lever moved. Entities 310–356 (idle tail refills to 342 — night spawner +
maxAlive top-up with 0 online, healthy), p99 ~4.1–5.9 ms.

`./build/server/bh_server --replay-world logs/t071.bwj` →

`[replay] OK ticks=13201 sessionCmds=5633 hashes=133 mismatches=0 entities=342`

## Files

`server/src/{world.cpp,world.h,main.cpp}`, `shared/{content/items.h,
sim/bhmap.{h,cpp},protocol/messages.md}`, `tools/mapconv/main.cpp`,
`tools/mapgen/make_thornwall.py`, `data/maps-src/thornwall.tmj`,
`engine/render/lightmask.h` (new), `client/src/{game.cpp,net_client.cpp,
net_client.h}`, `tests/{test_light.cpp,test_bhmap.cpp,CMakeLists.txt}`.
Suite **126 / 328,054**, ctest 2/2, warning-free.
