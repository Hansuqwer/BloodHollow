# 0086 — The reach stops being luck, the kiter was never casting, and the crossing got a route (T-125)

T-118's series closed on an uncomfortable sentence: the font was reached
once, in ten legs, and the leg that reproduced its posture exactly did not
reproduce the reach. So the entry went into the record as luck. The
follow-on had one job — find out whether that was true.

It wasn't. Six legs, all 900 s, all replayed bit-exact:

| leg | change | map-5 entries | reach times |
|---|---|---|---|
| r18 | r9's map-1 posture restored (individual march, cap-cross q≥2) | 0 | — |
| r19 | + distance-aligned marks | **3/5** | **147.1 / 147.8 / 196.4 s** |
| r20 | + the Firebolt kit-gate fix | **4/5** | **268.4 / 268.6 / 269.4 / 272.1 s** |
| **r21** | + a staged map-5 route (four nodes) | **4/5** | **231.7 / 232.8 / 237.5 / 237.9 s** |
| r22 | + the kiter band on map 5 — **reverted** | 0 | — |
| r22b | r22 re-run, same binary | 2/5 | 331.0 / 331.3 s |

Two consecutive reaches where the previous ten legs produced one, and r19 got
there in a quarter of r9's time. r9 wasn't a lucky corridor; the party had
been mis-marking its targets the whole way. Five bots standing within three
tiles of each other were each picking their own "lowest-HP" mob from their
own stale last-seen-hp view — five views, five different rankings, DPS split
five ways. Distance is the one field a tight column actually agrees on, so
nearest-first makes the marks coincide by construction. That was handover
§5's fix #2, sitting untried for a whole series.

r18 is the odd one out, and the honest one: it reached nothing. But it took
all five bots to map-3 **x=38–39** — the barrow corridor — where r17 never
got past x=24. It bought penetration and not the finish, which is exactly the
kind of result that gets overstated if you only read the entry column.

## The thing the legs found

Then the r19 forensics pointed somewhere unexpected. The party was arriving
on the font map healthy — 84–100% hp — and dying in the crossing, and the
Gravecaller, the one kit with a ranged nuke, was walking into melee to do it.

`tools/bots/main.cpp` gated the Firebolt cast and the kiter band on
`kitClass == 3`, with a comment explaining why: *"Firebolt is the Pale
Choir's channel — the Gravecaller kit is Chorus/Haste, so the kiter is
kitClass 3."* The comment had the two kits exactly backwards.
`shared/content/kits.h` gives ch5 Firebolt to the **Gravecaller** at L1 and
leaves the **Cultist's** ch5 at `0` — "kit may not channel it" — and
`World::trySkill` drops the cast at that gate before `tryFirebolt` is ever
reached. Every bolt the "kiter" sent, every 2.5 seconds, for the whole
r8→r19 series, was deleted by the server. The party fought the gauntlet and
the Gravemother melee-only, and one of its five members was playing a role
that did not exist.

A comment is not a spec. The fix moved both gates to `kitClass == 2` — and
the reason it survived this long is that nothing pinned the channel table, so
the suite was green while the harness was wrong. `tests/test_kits.cpp` now
runs the real server path both ways: the Cultist's bolt leaves `mp` unpaid at
30 and the target untouched at 12 hp, the Gravecaller's lands. Six new
assertions; the suite is **209/209 · 329,058**.

With a bolt that fires, the reach went 3/5 → 4/5 and the party killed its
first elites at this gate.

## What is still not known

**The boss verdict is still not established, and it should not be reported as
settled in either direction.** Both reaching legs engaged the Gravemother —
`bossSeen=20`, curse 2 — and scored zero kills. The blocker has moved, and
the map-5 traces say where: the party enters at `(3,30)` healthy and dies
crossing to the font camp at `(22,3)`; the deepest x it reached was **12**.
Map 5's route table has exactly one node, so the crossing is a single
25-tile dive through L18–25 content with a L13 party — map 3 got a four-node
corridor spine and map 5 got nothing. And the kiter band is still switched
off on map 5, so the Gravecaller now bolts, but from inside the boss's range
rather than outside it.

## The crossing got a route, and it worked

r21 gave map 5 the spine map 3 already had. The geometry had to be verified
rather than read from the handover, and that mattered: the handover's own
suggestion put nodes at `(8,20)` and `(14,12)`, and both are **blocked**. The
walkable corridor runs up x=6, east along y=10, then north up x=22 to the
font — 46 steps from the portal. Four nodes on it, **(6,21) → (13,10) →
(22,10) → (22,3)**, turned the dive into four fights, and the numbers moved
everywhere at once:

- map-5 time in the traces: **24 → 122** entries
- elites killed per bot: **1 → 3, 5, 5, 5**
- trash: **2–4 → 12–14**
- reach: **4/5 at 231.7 / 232.8 / 237.5 / 237.9 s**, `bossSeen=26`

Deepest x went 12 → **15**. The party clears the entry chapel, holds the
causeway shoulder at (13,10), and dies where three aggro fields overlap:
apse elite (16,4) r8, Sexton (21,5) r8, Gravemother (21,2) r8. The last
seven tiles of the corridor are the whole fight, and a four-deep L13 party
walking into three 8-tile fields at once is not going to survive it by
DPS alone.

## And a lever that never got a fair test

r22 switched the kiter band on at the font — the Gravemother's bolt range is
6 and our Firebolt's is 8, so the Gravecaller can trade from outside her
reach if it stops walking into a 40-damage slam. Two legs ran that binary.
**Neither one reached the code.**

r22 never entered map 5 at all: 618 of its 791 traces were on map 1, its
deepest map-3 x was 36, and it visited node (39,12) five times where r21
visited it 115. r22b, the same binary re-run, put two bots on map 5 for
**1.3 seconds and 2.3 seconds**, stuck at node (6,21), sixteen traces total.
The band never fired at the font in either leg.

The diff is provably inert on maps 1 and 3 — `bandMap` is false there and
the compiled path is byte-identical — and that is where both legs were
actually lost. So the honest verdict is **inconclusive**, not "the kiter
band is bad". Same binary, same staging, 0/5 then 2/5, against r21's 4/5:
one 900 s leg is a sample of one, and this series has always said so.

It was reverted anyway, and the reason is not caution for its own sake: the
band ends in `continue`, so leaving it in means leaving an unexercised
branch on map 5 inside the best-known configuration, where the failure mode
is exactly the r8c stall the series already paid for once. Re-land it behind
a leg that reaches the font, with the front-runner guard intact — the runner
owns the route machine the whole column follows, and banding it is how a
stall becomes a wipe.

## Housekeeping worth recording

The series' DB died with the last sandbox, so before any leg could mean
anything the staging had to be rebuilt — and that exposed a second
documentation drift: `T-118-r8g.md` §3 says the top-up re-seeds gear and
stats "via the python seed inside `m3_topup.sh`". It never did; that gear was
earned by the grind legs into a DB nobody has any more. Force-setting level
13 on a character still holding creation fists would have produced a leg that
looked like evidence and meant nothing, so the seed went into the script
(Pit Blade, Hide Armor, 7/11/6), idempotent and disclosed. Two docs claimed
behaviour the code did not have. Only one of them was in a comment.

Legs of record: `logs/m3_gate_r18.bwj` through `logs/m3_gate_r22b.bwj` (six
legs), each re-verified `mismatches=0`. The r17 journal
`logs/m3_gate.bwj` was preserved byte-identical through all six
(md5 `b4c0cf3f3255e38071c172cc3a3336bd`) — the harness overwrites it, so it
was copied aside and restored each time. Epoch stays 21, schema v11, duel pin
untouched; no sim code moved.
