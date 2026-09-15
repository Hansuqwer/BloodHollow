# Devlog 0094 — T-133 Heartstone + crown (Phase S, 2b/4)

## What

Zone-6 load seeds the Heartstone (kind 76, by the Inner Gate). While the
battle runs, uncontested registered presence within 3 accumulates 1200
ticks (60 s) → attuned (chatCh-2) — any contesting presence freezes it,
no decay. `/crown` → journaled `kCrown`: registered + alive + attuned +
≤2 of the stone → 200-tick kneel; move/hit/death/battle-end/leaving
breaks it (directed line); completion sets `siegeHolder_`, ends the
battle, broadcasts "takes the Weeping Crown!". Swings allowed mid-kneel
(stillness is about feet). Friday-Night leg 3 runs on this card.

## Epoch

NONE (stays 24): zero draws; `t128.bwj` re-replays mm=0.

## Evidence

- `test_siege_crown.cpp`: 6 cases — stone spawn, accumulate/contest/hold,
  attune broadcast, crown gates, full kneel (holder + battle end +
  broadcast), all four breaks + leave, journaled path.
- Full ctest 2/2 (263 cases). validate_links 0/5. Duel pin unchanged.
- Test lesson (pinned): movement validation drops non-adjacent waypoints,
  so the move-break pin must queue a valid adjacent chain — a pushed
  teleport path vanishes in-tick and the kneel (correctly) holds.

## Next

S3: taxes/vault/holder buff + holder persist. S4: siege bots + M4 gate
(40 bots, p99 <25 ms, 3/3 flips).
