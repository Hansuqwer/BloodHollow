# 0045 — Trade transaction log: the audit trail (T-080, S33)

S33 of the extended queue. GDD §7 dupe-audit enabling, minimal.

## Scope as pinned

- On the executed swap in `World::tradeCommit` (post-validation,
  post-deduce, offers still intact — the cleanup loop below clears them):
  one append-only line. Pinned shape:
  `tick=<t> a=<id:name> b=<id:name> a_gives=(<iid:qty,...>)+<gold>g
  b_gives=(...)+<gold>g`, canonical by lower id (identical no matter who
  commits second — replay- and order-stable).
- Opened per trade (`fopen "a"` + close; trades are rare, no hot-loop fd).
  Default path `logs/trades.log`; `World::setTradeLogPath` test seam keeps
  the suite off the real file.
- Cancelled/unsatisfiable commits append NOTHING (proved in tests).
- Replay re-executes identically (same tick/ids/offers) — ops dedups
  trivially. Stated, not solved.
- Deviation from the brief's filename (`logs/trades-<date>.log`): fixed
  path, no wall-clock anywhere near the sim (determinism hygiene);
  rotation is ops'. Flagged.

## Epoch

No bump (stays **12**): outside the journal, no sim effect. Short soak +
replay to prove the latter (below).

## Soak + replay

Fresh 540 s grinder mix (port 7889, fresh DB `/tmp/t080.db`, epoch-12
journal `logs/t080.bwj`), same 14-bot shape. Bots don't trade — the soak
proves no perturbation; unit tests pin the log:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 49 | **0** | 3 | killerByLvl empty |
| fighter ×4 | 86 | 52 | 4 | spread |
| pilgrim ×3 | 75 | 38 | 4 | spread |
| wander ×5 | 0 | 70 | 1 | decoys |

Bands: Rat 11.9 (n=20) / Bat 12.7 / Ghoul 15.0 / Hound 71.7 (n=9) /
Gnoll 111.7 (n=2) / Gravecaller 66.7 (n=2) — ordering preserved;
mid-band wobble stays a standing watch item. Entities ~344–357, p99
~3.2–4.8 ms.

`./build/server/bh_server --replay-world logs/t080.bwj` →

`[replay] OK ticks=12801 sessionCmds=7389 hashes=129 mismatches=0 entities=354`

## Suite hygiene found live

The pre-existing trade swap test executed real swaps into the real
`logs/trades.log` (three identical lines found mid-shift). Fixed: the
test pins the tmp path (same seam) and restores the default; plus
`logs/trades.log` is now git-ignored (runtime exhaust, not evidence).
Also added mid-implementation: canonical by-lower-id ordering (the first
cut ordered by committer — caught by the pinned-line test).

## Files

`server/src/{world.cpp,world.h}`, `tests/{test_tradelog.cpp,test_combat.cpp,
CMakeLists.txt}`, `.gitignore`. Suite **154 / 328,457**, ctest 2/2,
warning-free.
