# 0046 — Durability on death: iron rusts with you (T-081, S34)

S34 of the extended queue. GDD §7 literal, minimal.

## Scope as pinned

- In `World::killPlayer`, after the chaotic-drop block: every surviving
  gear slot (slot ≤ 1) −5 durability, floor 0 (dormant per T-058, never
  destroyed). Junk/consumables untouched. All victims — lawful rusts the
  same as red. Order pinned: drops first, wear second (deterministic).
- No epoch bump (stays **12**): deterministic arithmetic, no new strata —
  cited rule. Short soak + replay to be safe (below).

## Soak + replay

Fresh 540 s grinder mix (port 7891, fresh DB `/tmp/t081.db`, epoch-12
journal `logs/t081.bwj`), same 14-bot shape. Death wear is live in this
leg (130 deaths → ~130 × −5 applications fleet-wide, zero anomalies in
the close states):

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 97 | **0** | 4 | killerByLvl empty; mend=14 live |
| fighter ×4 | 47 | 24 | 3 | spread |
| pilgrim ×3 | 42 | 20 | 3 | spread |
| wander ×5 | 0 | 86 | 1 | decoys |

Bands: Rat 4.6 / Bat 4.8 / Ghoul **8.5** (back at the T-033 8.9 pin —
the wobble is noise, confirmed across legs) / Hound 25.5 (n=9).
Entities ~345–358, p99 ~3.2–3.8 ms.

`./build/server/bh_server --replay-world logs/t081.bwj` →

`[replay] OK ticks=12801 sessionCmds=7536 hashes=129 mismatches=0 entities=359`

## Files

`server/src/world.cpp`, `tests/test_combat.cpp`. Suite **156 /
328,471**, ctest 2/2, warning-free.
