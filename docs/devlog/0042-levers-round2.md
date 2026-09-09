# 0042 — Bot levers round 2: retreat 5→4 (T-077, S30)

S30 of the extended queue. Executes pit item (1) from devlog 0037, same
A/B recipe (L2-parked pair at (55,14)/(54,13), 540 s, 2-bots-alone).
Bot-only: no epoch bump (stays **11**), no sim change.

## The one lever

`swarmOnUs >= 5` → `>= 4` (`tools/bots/main.cpp:789`, level gate kept —
one lever). The v5 comment warns 3 disengages too much; 4 is the measured
middle between v5's 5 and the over-eager 3.

## Before (carried from T-074, same regime, v5c binary)

L2-park BEFORE: 32 deaths (L3 all), lastDeath (55,14)/(54,13), maxLevel 5,
levelDrops 2, mend 52.

## After (harvested)

`logs/t077_after.*`: **92 deaths** (L3:44+48, all ghouls),
lastDeath (54,14)/(54,13), maxLevel **3** (vs 5), levelDrops 8, mend 98,
kills 120. Same ground, same killers, same start state.

## Verdict: lever REJECTED, reverted

Retreat-4 nearly tripled deaths (32 → 92) and capped the climb at L3:
earlier breaks mean the pair never banks XP and never out-levels the
pack — the v5 comment's warning about threshold 3 extends to 4. The tree
keeps a 3-line comment on the restored `>= 5` recording the rejection
(judgment call, flagged — prevents re-deriving, zero behavior change).

Round-2 pit remainder stays parked with numbers: **repeats** (round 1–2
were n=1 pair each — chaos dominates single runs) and **L1-naked**
(isolates the pack gate with the climb confound removed). No blind tweaks;
no third lever stacked.
