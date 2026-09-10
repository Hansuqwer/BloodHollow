# 0071 — The nest in the elbow (T-102)

T-102, content (epoch 15→16). Second elite, deliberately boring in all the ways T-101 was interesting: same pattern, same announce, same honesty note. The only new judgment is real estate — the gallery-3 elbow, below the gnoll rect, against the vein darkgrass.

## What landed

- Row 1013, nest spawner, three tests, GDD shipped-mark. The shared `namedEliteIdx` needed no changes (slots reserved in T-101 — the one piece of foresight this run can claim without blushing).
- One collateral: the S18 asset gate pinned mine spawners at 7. Mapgen truth moved to 8, so the gate moved with it. Old gates that pin counts must move when counts move — noted here so the next elite card (crypt count 11→12) does the same without surprise.

## Judgment calls

- Nest shares the elbow room with gnolls rather than getting a private chamber: the mine's carved space is tight, and an elite that pulls its neighbors is content, not a bug. If playtests report the elbow as a death funnel, the lever is a side pocket off the drift (one mapgen rect), not stat surgery.
- 45 minutes (not 30): the three timers stagger 30/45/60 so no two elites rotate together. Ritterotational spacing is the whole "random" illusion, priced per T-097.

## Soak (epoch-16 validation leg)

`logs/t102.bwj`: 14-bot grinder mix, 540 s → `[replay] OK ticks=12801 sessionCmds=7242 hashes=513 mismatches=0 entities=180`. Bots: campaign 0/L3, fighter 76, pilgrim 20, wander 64. The fighter 76 reads high until the histogram: spread across L2/L3/L5/L7/L11 with scattered grounds and maxLevel 5 — roam band (T-083), not treadmill (T-074 legs were all-L3 at one waypoint). The Widow's nest kept undisturbed (no mine traffic, as stated).
