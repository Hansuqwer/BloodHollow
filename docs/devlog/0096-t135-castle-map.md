# Devlog 0096 — T-135 Weeping Castle map live (Phase S, map 6/6)

## What

Adopted `weeping_castle.tmj` + generator from the T-123 lane working tree
(byte-stable under regeneration — verified before adopt), added the
Thornwall return portal (`castle_road` (21,14) → map 6 (20,27); regen
verified: ONLY the portals layer changed), wired map 6 into mapconv +
both boot zone lists, extended the validator to 6 maps. Boot shows zone 6
online; the castle↔town portal pair validates both directions.

## Epoch

24 → 25 (CORRECTION mid-card: first claimed no-bump, but the t128
re-replay failed 12/12 hashes — zone 6's spawners + siege works shift the
entity set, the T-068 barricade precedent exactly). Fresh leg `t135.bwj`;
guard refuses t128 exit 4.

## Evidence

- New TU case: real-bhmap zone-6 load seeds 2 gates + 1 stone.
- validate_links: 0 problems across 6 maps.
- Gate leg (5 fighters, 35 kills, entities=189): replay mm=0.
- Full ctest 2/2 (270 cases). Duel pin unchanged.

## Merge note (human)

PR #23 is SUPERSEDED on `data/maps-src/weeping_castle.tmj`,
`tools/mapgen/make_weeping_castle.py`, and the zone-6 boot wiring by this
card's PR — its remaining content (client deltas, devlogs, T-122/T-123
cards) still wants human reconciliation against #22.

## Next

T-136: `--siege-rehearsal` flag + siege bot profile. T-137: rehearsal
harness + M4 verdict.
