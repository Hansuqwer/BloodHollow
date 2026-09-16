# T-162 — Mob roster completion + sheets for 1011–1014 + D9 rename (P1, audit 13/20)

## Context
Audit findings **C17 / E1.2 / B9.4**. `shared/content/mobs.h` has 14 rows
(1001–1014); GDD §11's roster asks for 12 common + 2 elites + 3 named + 1 boss and
names **Pale Cultist (caster), Mine Wretch, Lantern Spider, Mud Golem, Crypt
Revenant, Grave Banshee (elite), Bell Ringer (summoner elite)** — none shipped.
Art covers 1001–1010 only, so the Gate Guard (1011), Old Maw (1012), Red Widow
(1013) and Cantor Vex (1014) — three of them the *named elites that drive the
Friday-Night leg-2 story* — draw as the hero placeholder. The B0 gate's D9 ruling
renames mob 1007 to **Waxen Celebrant** (the player class keeps "Gravecaller");
`mobs.h` still says Gravecaller. GDD §9's night-only mobs (Wraiths, Bloodfiends)
are also absent while the `nightOnly` spawner mechanism ships.

## Scope
- Content lane (art-dependent parts coordinate with the art batches, not blocking):
  1. Apply D9: rename 1007 → Waxen Celebrant in `mobs.h` (id-keyed folders
     unchanged) + REGISTRY + any doc that names it.
  2. Add the missing roster to a level band plan covering L1→25 across zones 2–5
     (mine: Wretch, Lantern Spider, Mud Golem; crypt: Crypt Revenant, Grave
     Banshee, Bell Ringer; town/fields: Pale Cultist caster). Each row: hp/dmg/def/
     dex/xp/aggro/cd/wander/leash/loot/gold consistent with the existing tier law
     (fix the ×8/×20/×100 multiplier drift while here, or record it for T-158).
  3. Night-only variants using the shipped `nightOnly` spawner flag (+50 % XP per
     GDD §9) so night has its own roster.
  4. Spawner placement in the generators (`tools/mapgen/*`) + `validate_links`
     clean + density rule respected; **all six generators must keep reproducing
     their committed `.tmj` byte-identically** (add `--out` to the four that lack
     it — see T-168).
  5. Art: request sheets for 1011–1014 and each new mob through the existing
     aigen pipeline (probe-before-batch rule, ≤10 gens/session); until they ship,
     confirm the fallback is the placeholder and not a crash.
- Epoch: new spawners/entities change world composition → **epoch bump + fresh
  leg** (T-068/T-112 precedent). Coordinate with T-151/T-159/T-161 so the tree
  takes one bump per wave, not four.
- OUT: boss phase redesign, vampire race, mounts.

## Acceptance criteria
1. `kMobs` covers the GDD roster (or the gap is an explicit T-158 decision);
   tier multipliers recomputed and matching the documented law within ±10 %.
2. Every new mob has a unit pin (stat block + drop + XP) and appears in a live leg
   log (kill line per new mobId).
3. `validate_links.py` 0 problems; all 6 mapgens byte-identical after regeneration.
4. Night-only mobs spawn at night and not by day (pin + leg evidence).
5. Replay `mm=0` on a fresh epoch leg; old legs refuse exit 4.
6. Art requests filed with probe results; sheets for 1011–1014 either shipped or
   carded with a quarantine note.

## Tests required
Stat-block pins, spawner/night pins, mapgen determinism, replay leg.

## Evidence owed at merge
Suite count, leg + replay line, roster table, art status, devlog, board row.
