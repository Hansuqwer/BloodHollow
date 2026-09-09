# 0044 — Refine +4 to +7: the Widow's hotter coals (T-079, S32)

S32 of the extended queue. Extends the anvil in shipped spirit.

## Scope as pinned

- Shipped 0–3 law **frozen** (mercy, mercy, 60% + destruction; 1 junk part
  + 50g; full-durability gate; "no hotter coal" voice).
- New rows take GDD §7 rates: 3→4: 65% · 4→5: 50% · 5→6: 35% · 6→7: 25%.
  Failure slips one temper — except the +7 bid, which forgets every temper
  (+0, GDD-literal). Only refine-2 shatters (unchanged). Fiction lines per
  outcome in the same voice.
- The refine value already rides `ItemSlot.refine` — **no wire change**;
  the +5 glow render stays T-ART-11's job (this card unblocks it — the
  S26 park note now lifts).
- New RNG strata (refine rolls) → **epoch 11 → 12** + fresh soak journal.

## Soak + replay

Fresh 540 s grinder mix (port 7887, fresh DB `/tmp/t079.db`, epoch-12
journal `logs/t079.bwj`), same 14-bot shape. Bots barely refine
(anvilTries 0) — the soak proves the ceiling change didn't perturb the
bands; unit strata pin the table:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 30 | **0** | 3 | killerByLvl empty |
| fighter ×4 | 56 | 30 | 3 | spread |
| pilgrim ×3 | 36 | 6 | 3 | quiet leg |
| wander ×5 | 0 | 146 | 1 | worst straw yet (see watch) |

Bands: Rat 6.1 / Bat 5.1 / Ghoul 13.2 / Hound 12.0 (n=7) / ordering
preserved. **Watch (new):** wander deaths per leg this shift —
24, 16, 24, 66, 68, 84, 146 — trends up while the fighting profiles stay
flat. Kills spread L2/L3/L5/L7 (+2 L11 bank-road strays), locations
scattered, no L15 involvement: shape says roam-RNG, but the monotonic
drift is recorded, not dismissed. If it persists two more legs, the prime
suspect is live-entity growth (330 → 378 across the shift) widening aggro
coverage, not any single card. Entities ~350–378 here, p99 ~3.2–4.4 ms.

`./build/server/bh_server --replay-world logs/t079.bwj` →

`[replay] OK ticks=12801 sessionCmds=7984 hashes=129 mismatches=0 entities=367`

## Files

`server/src/{world.cpp,main.cpp}`, `tests/test_gear_churn.cpp`. Suite
**152 / 328,447**, ctest 2/2, warning-free.
