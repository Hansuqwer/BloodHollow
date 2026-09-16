# 0087 — The corridor gets a joint and the kiter gets a leash, then variance bites (T-126)

T-125 left us at 4/5 reach at 232 seconds, dying at x=15 inside three aggro fields at once — apse elite (16,4), Sexton (21,5), Gravemother (21,2), all radius 8. The handover's first suggestion was an extra node at (19,10), walkable, five tiles from the Sexton, so the apse elite dies before the Sexton wakes. The second was the kiter band on map 5, re-landed behind a reaching leg with a front-runner guard.

Both landed in this card. The first two legs looked like a breakthrough — 5/5 at 150s. The next two legs, same binary, said otherwise.

## What changed

`tools/bots/main.cpp` only, no sim semantics, epoch stays 21:

- **Map-5 route**: was four nodes `6,21 → 13,10 → 22,10 → 22,3`. Now five: `6,21 → 13,10 → 19,10 → 22,10 → 22,3`. (19,10) is gid 2 in `drowned_crypt.tmj` → ground 1 walkable; (8,20) and (14,12) are gid 4 → ground 3 blocked, as T-125 warned. The corridor is (3,30) portal → north up x=6 → east along y=10 → north up x=22 → font. The new joint sits on the y=10 east segment, splitting the apse climb into two staged fights.

- **Kiter band re-land**: was `mapId !=1 && !=3 && !=5` (off on map 5, reverted after r22 never exercised). Now `mapId !=1 && !=3` with a guard: on map 5, if self is the front runner (`frontRunnerOf(b).id == ownId`), skip the band. The runner owns the route machine — banding it is the r8c stall that froze the column at the first respawned ghoul. Non-runners on map 5 now get the far band 7-8 for casters (Gravemother bolt range 6 vs Firebolt 8) and 3-6 for melee, same as map 2.

Diff <60 lines, reviewable.

## The legs — variance is the headline

Four legs, same binary, same staging (L13 / 7-11-6 / Pit Blade + Hide Armor / 16 vials / map 3 (1,22)), all replay bit-exact:

| leg | secs | map5 | reach (s) | elites / trash | deaths | replay | file |
|-----|------|------|-----------|----------------|--------|--------|------|
| r23 smoke #1 | 300 | **5/5** | 147.3 / 149.9 / 151.9 / 149.0 / 152.3 | 3,4,4,4,3 / 8-9 | 6/4/4/2/6 | ticks=6042 cmds=1957 hashes=60 mm=0 | overwritten, log in bash output 09:00Z |
| **r23 gate #1** | 900 | **5/5** | **145.8 / 150.3 / 150.0 / 152.3 / 153.8** | 1,5,5,5,2 / 5-8 | 8/6/10/6/10 | ticks=18042 cmds=4979 hashes=180 mm=0 | overwritten, log in bash output 09:15Z |
| r23 smoke #2 | 300 | 1/5 | 188.1 | 0 / 0-2 | 6/4/6/2/6 | ticks=6043 cmds=1842 hashes=60 mm=0 | `logs/m3_gate_r23.bwj` (36K) |
| r23 gate #2 | 900 | 0/5 | — | 0 / 0 | 8/10/10/8/10 | ticks=18042 cmds=4611 hashes=180 mm=0 | `logs/m3_gate_r23b.bwj` (92K) |

First two legs: 5/5 at ~150s, elites 5 on three bots — ~35% faster than r21's 4/5 at 232s. Second two legs, same binary: 1/5 and 0/5, never leaving map 1. Same binary, same staging, 5/5 then 1/5 then 0/5 — exactly the variance T-125 warned about (r22 0/5 vs r22b 2/5 same binary, r21 4/5 vs r22 0/5).

Why? The traces show 618 of 791 traces on map 1 in the 0/5 leg — deepest map-3 x=36, node (39,12) visited 5 times vs 115 in the 5/5 leg. The change is provably inert on maps 1 and 3 (bandMap false there, route table identical for maps 1/3), so difference is leg variance, not the lever — same verdict as r22.

## Why this still matters

- **Best-case improved**: 5/5 at 150s is the fastest and most complete reach in the series (r9 3/5 at 617s, r21 4/5 at 232s). Node (19,10) splits (13,10)→(22,10) 9-tile dive into 6+3 staged fights, so apse elite (16,4) can die before Sexton (21,5) aggros.
- **Kiter band now exercised**: r22 never reached map5, so was unmeasured. r23 #1 reaches map5 in 150s and band fires — no stall on map1/3 because runner guard keeps march-first intact. But Gravecaller still dies most (10 deaths), so band alone doesn't solve triple-aggro.
- **Variance is the finding**: same binary 5/5 → 1/5 → 0/5. Handover §4: one leg cannot promote a lever, and one cannot retire one either. Need n=2 more at 900s to confirm mode.

Boss verdict still not established — no kills in any leg.

## Housekeeping

Suite **209/209 · 329,058**, duel pin `b273be661b54673a`, old legs `m3_gate.bwj` (r17, 36042 ticks, 5933 cmds, 360 hashes, mm=0) and `t120.bwj` (1500 ticks, mm=0) replay clean. Epoch 21, wire 237, schema v11 — bots-only, no bump.

Journals `m3_gate_r23.bwj` (1/5) and `r23b` (0/5) force-added as variance evidence; the 5/5 journals were overwritten before archiving (logs in bash output). Next session can re-run r23 to confirm mode.

Next: **T-127** — re-run r23 900s to confirm 5/5 is its mode (handover §4), or break last 7 tiles (extra node (22,6) or pre-mark ordering). Boss verdict remains open.

Counters: next card **T-127**, next devlog **0088**, epoch **21**, wire **237**, schema **v11**, suite **209/209**, duel pin `b273be661b54673a`.

