# 0043 — Guard-murder consequences: the post is town law (T-078, S31)

S31 of the extended queue. Fills the M3 exit with derived pins.

## Scope as pinned

- `World::killMob`: a **player** who drops mob 1011 stains like an unlawful
  PK against level 15 — `−(300 + 20·max(0, 15 − killer.level))` (GDD §5
  shape, victim level pinned, no new numbers) — and takes the wanted mark
  through the **shared** `markWanted` path (refactored out of the T-073 PK
  block; the victim's own anchor is within 0 by construction). Fiction line
  mirrors the PK stain voice.
- Mob-on-guard violence is not a crime (players-only gate). No faction
  flags, no permanent marks, no vendor changes beyond the existing wanted
  refusal, refill stays 1200.
- Interplay found in testing: the at-level whitening tick (+1) fires
  blindly on guard kills (L15 paragon nets −299, pinned in the test). The
  whitening law does not exempt guards — stated, not special-cased.

## Epoch

No bump (stays **11**): existing fields, deterministic, shared path. Short
soak + replay to be safe (below).

## Soak + replay

Fresh 540 s grinder mix (port 7885, fresh DB `/tmp/t078.db`, epoch-11
journal `logs/t078.bwj`), same 14-bot shape. Bots never kill guards
(expected) — the refactor is proven by bands, not by wanted sightings:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 65 | 6 | 4 | L3 ghouls at (32,16), spread thin |
| fighter ×4 | 88 | 52 | 4 | spread L3/L5/L7 + 2 L11 |
| pilgrim ×3 | 20 | 12 | 2 | quiet leg |
| wander ×5 | 0 | 84 | 1 | decoys, worst straw yet |

Bands: Rat 6.3 / Bat 13.9 / Ghoul 24.7 / Hound 51.9 (n=7) / Gnoll 117.5
(n=2) — ordering preserved; mid-band wobble stays a standing watch item.
No L15 killer anywhere (guards uninvolved). Entities ~348–362, p99
~3.2–4.6 ms.

`./build/server/bh_server --replay-world logs/t078.bwj` →

`[replay] OK ticks=12801 sessionCmds=6929 hashes=129 mismatches=0 entities=362`

## Files

`server/src/{world.cpp,world.h}`, `tests/test_guards.cpp`. Suite **150 /
328,177**, ctest 2/2, warning-free.
