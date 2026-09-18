# 0119 — T-161b.4 Corpse Explosion (Gravecaller ch14)

## What
Spend the freshest mob corpse within 4 tiles for fixed 20+3L AoE (radius 2,
slam-mirror: no roll, no crit). Mob corpses only (players sacrosanct,
thralls crumble); kin + own thrall immune; player kills run the full PK law
(a red-choice AoE). Death tape: cap-32 ring, 300t meat window, unhashed.
Kind-5 hit lane — zero client change. Bots offer the shape every 20 s
(meatless casts spend nothing). Channel table 14→15 (Grave ch14=14).

## Why this shape
- Fixed damage keeps it RNG-free; the tape derives from journaled kills, so
  live and replay read the same meat. `wave2/sanct/raise.bwj` re-replay mm=0
  — epoch stays 30.
- T-106 care: the victim loop re-finds the caster per kill (killPlayer can
  crumble a pet and shift the deque); ids-first collection (cleave pattern).

## Evidence
- 4 pins in `tests/test_corpse.cpp` (meat + fixed AoE + caster XP, all gates
  incl. no-meat-no-spend, stale meat, replay-path dispatch). Debugging note:
  ghoul-brawl corpse laying works, but Entity& refs dangle across death —
  capture ids first (T-106 applies to tests too). Suite 354/354.
- Fresh leg `tools/t161b_blast_leg.sh` (L14 crypt party, 300 s, disclosed
  staging): `logs/t161b_blast.bwj` c14=3, replay mm=0 (60 hashes).

## Debt / next
- T-161b items 5–6 (control set, Ravager set); M3 re-measure at card close.
