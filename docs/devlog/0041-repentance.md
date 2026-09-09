# 0041 — Karma repentance: the chapel's second lane (T-075, S29)

S29 of the extended queue. Fills the T-070 deferral with derived pins.

## Scope as pinned

- `/repent` chat verb → journaled `kRepent` (appended after `kConfess` in
  `server/src/command.h` — old journal kind-ints untouched) →
  `World::repent` via `applyWorldCommand` (live and replay share it).
- Gates: within 3 of the confessor (`nearConfessor` reuse) + **72000-tick
  (1 logged hour) cooldown** (`repentUntil` stamp) + wanted refused with
  the gallows-bound line (gate law and chapel grace stay separate lanes).
- Effect: **+20 karma** — exactly one whitening-hour at the pinned rate —
  through `bumpKarma` (±1000 clamp, band-crossing events fire). Curse
  untouched (grace, not cure).
- Amount and cooldown DERIVE from shipped pins (whitening block), flagged
  for review — no invented numbers.

## Epoch

No bump (stays **11**): journaled command, no tick/entity change, karma
unhashed. Short soak + replay run to be safe (below).

## Soak + replay

Fresh 540 s grinder mix (port 7881, fresh DB `/tmp/t075.db`, epoch-11
journal `logs/t075.bwj`), same 14-bot shape:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 50 | **0** | 4 | killerByLvl empty; mend=16 live |
| fighter ×4 | 81 | 52 | 3 | spread |
| pilgrim ×3 | 73 | 30 | 4 | spread |
| wander ×5 | 0 | 68 | 1 | decoys drew the short straw (cf. T-073's 66) |

Totals 150 vs 116–126 priors — wander variance, same as S24's finding
(death totals flat once redistributed; no guard/L15 involvement). Bands:
Rat 11.7 (n=25) / Bat 3.4 / Ghoul 22.8 / Hound 74.8 (n=10) / Gnoll 26.6
(n=2) / Gravecaller 33.0 (n=1) — ordering preserved; mid-band wobble stays
a standing watch item. Entities ~345–356, p99 ~4.5–5.5 ms.

`./build/server/bh_server --replay-world logs/t075.bwj` →

`[replay] OK ticks=12801 sessionCmds=7483 hashes=129 mismatches=0 entities=356`

## Files

`server/src/{world.cpp,world.h,command.h,main.cpp}`,
`tests/{test_repent.cpp,CMakeLists.txt}`. Suite **147 / 328,167**,
ctest 2/2, warning-free.
