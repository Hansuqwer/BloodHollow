# Devlog 0093 — T-132 siege gates (Phase S, 2a/4)

## What

Zone-6 load seeds Outer + Inner gates (kind 75, hp 300, staging positions
off the zone spawn). `/breach` → journaled `kBreach`: registered attacker,
live battle, same-zone ≤2 reach → −10/ram, directed progress line; at 0
the gate despawns with a chatCh-2 splinters broadcast.

Design call (in-card): breach-by-channel, because `setAttack` refuses
furniture and `killMob` would drag XP/loot/whitening along. No combat-path
changes, no RNG, no repair (defenders contest by killing breachers).

## Epoch

NONE (stays 24): `t128.bwj` re-replays mm=0.

## Evidence

- `test_siege_gates.cpp`: 4 cases — spawn shape, gating matrix
  (no-battle/unregistered/far/dead), progress math, 30-ram kill +
  broadcast, journaled path.
- Full ctest 2/2 (257 cases). validate_links 0/5. Duel pin unchanged.

## Next

T-133: Heartstone (kind 76) capture + crown channel → holder set, battle
ends. Then S3 (taxes/vault/buff + holder persist), S4 (bots + M4 gate).
