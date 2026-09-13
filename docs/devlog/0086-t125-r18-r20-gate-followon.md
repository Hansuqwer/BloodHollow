# 0086 — The reach stops being luck, and the kiter was never casting (T-125)

T-118's series closed on an uncomfortable sentence: the font was reached
once, in ten legs, and the leg that reproduced its posture exactly did not
reproduce the reach. So the entry went into the record as luck. The
follow-on had one job — find out whether that was true.

It wasn't. Three legs, all 900 s, all replayed bit-exact:

| leg | change | map-5 entries | reach times |
|---|---|---|---|
| r18 | r9's map-1 posture restored (individual march, cap-cross q≥2) | 0 | — |
| r19 | + distance-aligned marks | **3/5** | **147.1 / 147.8 / 196.4 s** |
| r20 | + the Firebolt kit-gate fix | **4/5** | **268.4 / 268.6 / 269.4 / 272.1 s** |

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

r21 gives map 5 a route; r22 puts the kiter on the 7–8 band at the font. If
a four-deep party with working ranged damage and a staged crossing still
cannot kill her, "unkillable at L13" becomes a real measured finding. It has
not been earned yet — until r20 the party never had ranged damage at all.

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

Legs of record: `logs/m3_gate_r18.bwj`, `logs/m3_gate_r19.bwj`,
`logs/m3_gate_r20.bwj`, each re-verified `mismatches=0`. The r17 journal
`logs/m3_gate.bwj` was preserved byte-identical through all three
(md5 `b4c0cf3f3255e38071c172cc3a3336bd`) — the harness overwrites it, so it
was copied aside and restored each time. Epoch stays 21, schema v11, duel pin
untouched; no sim code moved.
