# Devlog 0092 — T-131 siege scheduler + registration (Phase S, 1/4)

## What

Zone-agnostic battle skeleton: weekly Saturday 20:00–21:30 game-time
window (pure tick math from the 08:00 anchor: start 1872000, len 108000,
week 2016000), war-band registration as captain entity ids (cap 8, dedup
quiet, dead/unknown/mobs refused; journaled `kSiegeReg` via `/siege-reg`
self-register), holder slot (0 = unclaimed), `siegeStart` (journaled
`kSiegeStart`, `gm siege-start`; in-window + non-empty registry or quiet;
battle runs to window end).

Divisions (all documented in-card): castle map = PR #23, rehearsal stub =
PR #29 (`gm siege-now` owns out-of-window rehearsals), captain ids upgrade
to pledge ids in Phase P, gates/Heartstone/crown = S2, taxes/persist = S3,
bots + M4 = S4.

## Epoch

NONE (stays 24): zero draws; `t128.bwj` re-replays mm=0 on the new binary.

## Evidence

- `test_siege_sched.cpp`: 6 cases — edges, weekly period, registration
  matrix, holder default + gating, end-at-close, journaled paths.
- Full ctest 2/2 (253 cases). validate_links 0/5. Duel pin unchanged.

## Next

S2: gates → Heartstone → crown channel (still zone-agnostic; map-6
placement constants resolve against whatever zone 6 loads).
