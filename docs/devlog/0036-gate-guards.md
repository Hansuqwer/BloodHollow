# 0036 — Gate guards + spawn-camp protection: the gate law has teeth (T-073)

S24 of the overnight queue. M3 exit (guard-murder) + M4 anti-grief list +
Alpha hardening. Straightforward, high value.

## Scope as pinned

- **Guard mob 1011** (`Gate Guard`, L15, hp 400, dmg 30, def 18, xp 0,
  aggro 0, wander 0, leash 12) + `guard` flag on `MobDef` (defaulted 0 —
  all ten existing rows untouched). Two spawners (`make_thornwall.py`):
  `guards_east_gate` (60,13,2,3 — road tiles) and `guards_bridge`
  (13,31,3,2 — bridge approach), maxAlive 2, refill 1200 (both pinned
  overnight — flagged).
- **Wanted:** unlawful PK (the T-056 penalty path) within 8 tiles of a guard
  anchor sets `wantedUntil = tick + 4800` (240 s) + fiction line. While
  wanted: all guard mobs acquire inside leash reach (deterministic spatial
  order); town vendors refuse (Marta buy/pawn rage lines extended,
  repair quiet-fail); death binds at the gallows even with clean karma
  (wanted persists through death until expiry).
- **Spawn protection (M4):** `spawnProtectUntil = tick + 100` on fresh
  spawns AND respawns. Mob acquire lookup skips protected targets
  (normal and guard lanes). **Retaliation still fires** if the protected
  player strikes first (no free hits — flagged). Broke one existing test
  (T-071 night-aggro: fresh spawn observed under protection); test fixed
  to clear protection first, day-half strengthened to pin dormancy alone.
- **Fence stays open** to the wanted (Sable asks nothing — flagged).
- Guards held on an expired wanted stand down (no grudges past expiry);
  normal mobs keep held targets.
- Epoch **10 → 11** (brief's 11→12 assumed an S23 bump; the audit didn't
  bump, so the chain shifts one — stated) + fresh-leg replay.

## Judgment calls (director review)

maxAlive 2 / refill 1200 per post; retaliation-despite-protection; fence
open to wanted; vendor "gallows-bound coin" lines; expiry drops held guard
chases.

## Soak + replay

Fresh 540 s grinder mix (port 7875, fresh DB `/tmp/t073.db`, epoch-11
journal `logs/t073.bwj`), same 14-bot shape:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 59 | **0** | 4 | killerByLvl empty; mend=15 live |
| fighter ×4 | 52 | 32 | 3 | down from 52–60 (noise, see below) |
| pilgrim ×3 | 66 | 21 | 4 | down from 42–48 |
| wander ×5 | 0 | 66 | 1 | up from 16–24 (noise, see below) |

**Totals are flat** (119 vs 116–126 the three previous legs): the wander
spike is redistribution, not a systemic move — wander lastDeaths scatter
((14,36), (33,29), (27,28), (11,44), (59,26)) across L2/L3/L5/L7 with 2
L11 strays, no guard (L15) involvement anywhere, no single-kind clump.
Bands: Rat 6.2 / Bat 11.0 / Ghoul 59.3 (n=30) / Hound 40.4 (n=5) / Gnoll
320.6 (n=1) — ordering preserved; the mid-band wobble is now a four-leg
standing watch item owned by T-074's measure-first discipline. Bots never
PK, so no live wanted occurred (expected) — the law itself is pinned by
the 8 unit cases. Entities ~350–363, p99 ~3.6–6.7 ms.

`./build/server/bh_server --replay-world logs/t073.bwj` →

`[replay] OK ticks=12801 sessionCmds=7321 hashes=129 mismatches=0 entities=361`

## Files

`server/src/{world.cpp,world.h,main.cpp}`, `shared/content/mobs.h`,
`tools/mapgen/make_thornwall.py`, `data/maps-src/thornwall.tmj` (13
spawners), `tests/{test_guards.cpp,CMakeLists.txt}` (+ `test_light.cpp`
hardening for spawn protection). Suite **134 / 328,092**, ctest 2/2,
warning-free.
