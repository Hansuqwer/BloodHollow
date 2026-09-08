# 0030 — the gate opens: barricade off the bridge, L8 reachable (T-068)

Devlog 0029 ended on a flagged scope boundary: bot tuning was exhausted and
L7→L8 had **no reachable step-up on map 1**. The director picked **Option A —
move the L11 Gravecaller barricade off the bridge approach** (over B "add a
crossing" and C "re-anchor the gnolls"). Card T-068. This devlog records the
placement math, the epoch bump, and the measured probe.

## The placement math (why x=1, y[43,45])

`spawnMob` anchors each mob at a **random tile inside the spawner rect**
(`world.cpp:228`), so placement has to be safe for the *worst* in-rect
anchor. Gravecaller (1007): aggro 7 (+1 at night), wander 5, leash 12 — the
kill zone is effectively **leash-12 Chebyshev around the anchor** (a mob can
be pulled 7 from its current spot, then chase out to 12 from its anchor
before the leash drops it — the v4 retreat deaths at (18,30)/(24,46) sat at
exactly 11 from the old anchor).

Bot route tiles to the L7 content: south road `x[14,15] y[17,33]`, bridge
`x[14,16] y[34,37]`, then the east-bank diagonal to `gnolls_pits`. Any
anchor with `x ≥ 2` south of the river can wander (5) into night-aggro (8)
range of the bridge-corner tiles; the only fully-clean placement west of the
river is the far south-west wall, **rect (1,43,1,3)**: every in-rect anchor
leaves mob reach ≥ 8 from every route tile, day and night. The old rect
(6,18) had its kill zone squarely over the south road — that *was* the wall.

The real bridge-exit gauntlet remains `hounds_marsh` (8 × L5, leash 16,
wander 10) — pre-existing content the bots already trade with.

## The epoch bump (6 → 7)

A moved spawner shifts the sim under old journals (T-030 precedent: "worldHash
now includes zones 4/5 … journal epoch 4→5 is the correct contract response").
`kJournalEpoch` 6 → 7 with the chain comment updated. Verified live: the
replay runner refuses a v6 journal — `[replay] journal epoch 6 vs build epoch
7 — sim semantics changed since; record a fresh gate leg (old leg retained as
history)` — and refuses to lie with it.

## Opening the waypoint (`tools/bots/main.cpp`)

L7+ on map 1 now camps **(53,41)**, the gnolls_pits north edge: gnolls
(wander 8, aggro 7) pull straight onto the camp; `ghouls_south_road` ghouls
(L3) are fodder via the defend branch; retreat-home crosses the river north.
`widow_glade` (L9) shares the bank and *can* pull — the pack cap
(`pack >= 2` skip) and swarm panic-break at 5 are the levers. This waypoint
is the verification vehicle: without it `TARGET L8` can never fire.

## Result — val5 is a diagnostic, val5b is the verdict

**val5 (chain DB resumed as-is) died in a known mode, not the new one.** The
leg-12 chain DB saved both characters parked mid-swarm at the ghoul perch
(bot00's login tile (53,16) is *adjacent* to the `ghouls_east` rect). Both
L6 bots fed the perch treadmill: **156 deaths** (killerByLvl L3:76/L3:80,
`lastDeath=(53,16)` — the retreat-path choke past the rect), levelDrops=11,
maxLevel 4. This is the same death-loop family as devlog 0009's "scouts died
75 times" and chain legs 3/5 (142/134 deaths) — a harness state problem
(resuming inside a live aggro cluster), **not** evidence against the change:
zero L11 kills, and the L7 waypoint (the thing under test) never activated.
Harness lesson recorded: the val pattern must **set** state via sqlite, not
just copy the DB.

**val5b (chain DB, chars set to L7 in town via sqlite, gear/gold kept) —
`TARGET L8 DONE in 444.3s`.**

| | val4 (v5c, resumed at L5) | val5b (T-068, resumed at L7 in town) |
|---|---|---|
| TARGET L8 | never fires (no south route) | **fires, t=444.3 s** |
| peak / end | L7 / L7 | **L8 / L7** (one post-L8 death, levelDrops=1) |
| deaths | 10 | **6** |
| killerByLvl | *(empty)* / L5:10 | L2:2+L5:2 / L2:2 — **zero L11** |
| lastDeath | (25,30) | (32,16) — the road near town |
| kills / pots | 131 / 40 | 147 / 84 |
| replay | OK 0 mm | **OK ticks=15601 hashes=157 mismatches=0 (epoch 7)** |

The route the bots walked is exactly the one the old barricade walled: south
road → bridge → east bank → gnolls_pits north-edge camp `(53,41)`. Deaths on
route were L2 bats (road walk) and the L5 hound bridge-exit gauntlet —
pre-existing content — and the L11 Gravecallers killed **nobody** from their
far-marsh post. The director's `TARGET L8` line is met end-to-end on map 1.

## Scope boundary that remains

L8→L9 (`widow_glade`) has no opened waypoint yet (the L7+ camp stops at
gnolls), and widows share the bank with the gnoll camp — a pulled L9 widow
at the camp is survivable but untuned. Follow-up card if the director wants
the ladder pushed to L9/L10. No mob numbers moved in T-068.

