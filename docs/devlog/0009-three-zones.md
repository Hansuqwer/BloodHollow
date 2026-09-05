# Devlog 0009 — three zones, one death rule, zero validator debt *(2026-09-04)*

Post-S8 gatekeeping turn. What landed:

## Delivered

- **T-038** — `docs/rfcs/0001-anvil-aura-spine.md`: the Soma adoption as a
  concrete, costed plan: five aura tiers (I `Edge Rite` .. V `Crimson Pact`),
  anvil locations, parts+gold tolls, first-attempt guarantee, karma split
  (good halves fail odds; bad gains +15% anvil gold), telemetry hooks. Cards
  S9..S11 spin out of it.
- **T-040** — bot deaths probe now CombatEvent-driven (kind 3, self target).
  Immediately proved worth it: the three-zone gate logged deaths=54+75+7 that
  the old hp-delta probe had silently missed (it read 0 in past runs).
- **Cross-map validator** — `tools/mapgen/validate_links.py`: checks every
  portal (standable rect, known target map, walkable arrival) and spawner
  walkable coverage. **Caught 3 real bugs its first run**: east_gate pointed
  at map 2 which didn't exist, and ghouls_orchard spawned half-submerged in
  the river (7/35 walkable — silently starving that camp since S7).
- **Map 2 born**: `fields_overflow.tmj` (Bleak Fields East, 64x48, 86.7%
  walkable, 4 spawners incl. gnoll camp + gravecaller hedge); the world is now
  thornwall / fields / crypt. Server boots all three; replay mirrors; client
  map table extended.
- **Bindstone rule** (marine park lesson from the gate: scouts died 75 times
  in a death-loop near their spawn zone) — death respawns player at zone 1
  town every time now, transferred cleanly through the zone machinery
  (zoneChanged event + spatial reseat). Locked by test case.

## Three-zone gate of record (320 s)

- 5 grinders + 2 crypt scouts + 2 east scouts, 70 peak entities across 3 zones.
- soak p99 0.17 ms, 0 mid-leg drops; two `[zone]` crossings recorded.
- **Journal replay: 6401 ticks / 2908 commands / 65 hashes — 0 mismatches**
  (`logs/2026-09-04-3zone-journal.bwj`).
- Death churn visible (deaths=136 across fleets; pots 291; levelDrops 2).

## State of the campaign

Tests 44/44 (327,146+ assertions). Phase 3 is now properly open: three live
zones, a validated map graph, a designed crafting spine, and proof the world
deterministically replays. Next: S9 = T-041..T-046 (anvil objects, aura tier
I-II implement, item flags v5, pilgrim bots, karma columns) + M2b campaign leg.
