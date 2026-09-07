# 0028 — the plateau moves, the wall relocates (T-034d chain rerun)

T-034d gave the campaign bot a re-gear trip; the full 12-leg M2b chain rerun
(`tools/m2b_rerun_chain.sh`, fresh DB, 2 campaign bots, ~10.8k ticks/leg) is the
verdict. **The plateau moved past L5** — from the L4-L5 treadmill to a **L6
ceiling** — but L6 does not hold: it has its own wall.

## The table

| leg | peak | end | deaths | kills | regear | replay |
|---|---|---|---|---|---|---|
| 1 | L3 | L3 | 58 | 81 | 2 | OK |
| 2 | L5 | L5 | 14 | 126 | 2 | OK |
| 3 | L6 | L6 | 40 | 139 | 2 | OK |
| 4 | L6 | L5 | 76 | 113 | 2 | OK |
| 5 | L6 | L3 | 128 | 88 | 2 | OK |
| 6 | L3 | L1 | 182 | 45 | 2 | OK |
| 7 | L4 | L4 | 56 | 125 | 2 | OK |
| 8 | L4 | L4 | 48 | 126 | 2 | OK |
| 9 | L4 | L3 | 68 | 97 | 2 | OK |
| 10 | L5 | L4 | 22 | 108 | 0 | OK |
| 11 | L5 | L5 | 16 | 112 | 0 | OK |
| 12 | L6 | L6 | 32 | 143 | 0 | OK |

`peak` = highest level a bot climbed to that leg (last "reached L" line); `end`
= level at leg close (SUMMARY `maxLevel`). Every journal replays **0
mismatches**; `TARGET L8 DONE` never fires. The trip itself (`regear`) fires in
legs 1-9 and is correctly silent once gear persists across the death spiral
(legs 10-12).

## What moved

Baseline (T-034c, devlog 0025) sat at L4-L5 for 12 legs and **never touched
L6**. This run reaches L6 four times and *ends* two legs at L6 (legs 3 and 12)
with deaths in the teens. The re-gear trip did exactly what T-034c predicted:
decoupling shopping from death-respawn clears the hound band — deaths drop
58→14 across legs 1→2, the single biggest one-leg improvement in the ledger.

## What didn't

L6 is a new wall, and it is **not the armor band**. It is the over-level
triangle T-034c flagged as secondary (§4 of the card): the moment a bot hits
L6 its campaign waypoint steps into gnoll (L7) / widow (L9) camps, and the
death tax spirals it back. Legs 5-6 log **128 and 182 deaths** and 19
level-drops, collapsing two L6 bots to L3 and then L1 before legs 7-12 slowly
climb back. The treadmill has moved up one band, not vanished.

## Next lever

T-034d's card predicted "if it parks at the armor band, Hide Armor 120→80".
It does not park there — it parks at L6. The honest next lever is the campaign
profile's **L6+ target selection** (don't walk a L6 bot into the L9 widow
glade; the "no walls" filter only skips `level > own+2` mobs when `d > 2`, so
an adjacent gnoll/widow is still pulled). That is a bot-profile change, not a
content/price nudge. Flagged for the director — no mob numbers move without a
card.

## Evidence

- `logs/m2b_rerun_gate.log` + `logs/m2b_rerun_leg1..12.bwj` — 12/12 replay
  0 mismatches.
- Per-leg SUMMARY in `logs/m2b_rerun_leg1..12_bots.log` (kills/deaths/regear/
  maxLevel per leg).
- Suite 105/105 (327,925 assertions), ctest 2/2. Journal epoch stays 6 — the
  change is client-side bot behaviour only.
