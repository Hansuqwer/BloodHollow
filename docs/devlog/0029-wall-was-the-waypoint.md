# 0029 — the wall was the waypoint, not the mobs (Route v5)

Devlog 0028 closed on a wrong hypothesis: that the new L6 ceiling was the
"over-level gnoll/widow triangle". The death diagnostics added since then
(`killerByLvl`, `lastDeath`) disprove it — the killers were **L3 Feral Ghouls**,
not L7/L9 mobs. The wall was never mob level. It was **pack density at the
campaign waypoint**, plus a retreat that walked into worse content. Fixing
those two things in the bot profile moved the ceiling from L6 to **L7** and cut
deaths by ~85%.

Full analysis issued as `docs/prompts/campaign-pack-wall-analysis.md`.

## The evidence that broke 0028's story

| run | level | deaths | killerByLvl | lastDeath |
|---|---|---|---|---|
| `diag_l6` (fresh climb) | L3 | 20 | **L3:10** L5:2 / L3:4 L5:2 L2:2 | **(55,18), (55,20)** |
| `val2` (resumed, 540 s) | L5 | 66 | L7:2 L5:8 **L3:22** / L7:2 **L3:22** L5:10 | (24,46), (25,30) |

L3 = Feral Ghoul. L7 (gnoll) killed 2 bots **total across both runs** — noise,
not a wall. Three independent findings:

**(a) The waypoint was the pack centre.** Route v4 parked every level ≥ 3 bot
on `(55,19)` — the exact centre of `ghouls_east`
(`make_thornwall.py:156`: rect x[52,58] y[17,22], **maxAlive 8**). `lastDeath`
clustered on `(55,18)`/`(55,20)`: bots died standing on their own waypoint,
inside an 8-mob spawn rect. Ghoul aggro is 6 (7 at night), so all 8 engaged;
8 × 9 dmg on an 18-tick cycle deletes a 94 hp L3 bot in ~1.5 s.

**(b) The "pull singles" filter exempted the swarm.** The old rule only skipped
a target when `pack > 0 && level >= own - 1`, with trash swarms (own-2) treated
as "era fodder" divable freely. At L5 a L3 ghoul fails `3 >= 4`, so an 8-mob
cluster was classified as free XP. The filter was also gated on `d > 2`, so at
point-blank there was no density check at all.

**(c) The retreat walked into the graveyard.** The v4 disengage fled ±10 tiles
along the threat axis. From `(55,19)` that runs a bot west down the south road
`PATH x[14,15] y[17,33]`, which passes 3 tiles from the **L11 Gravecaller**
barricade `(6,18)` (dmg 30, aggro 7) and on into `hounds_marsh (10,40)`
(8 × L5, leash 16). That is exactly the L11:2 and L5 kills at
`lastDeath = (18,30)/(24,46)` — nowhere near camp.

## Why deaths were the binding constraint

XP curve (`shared/sim/combat.h`): L5→6 = 1960, L6→7 = 2750, L7→8 = 3660; a
ghoul pays 90. ~94 kills for L5→L8 — rate was never the problem. Death debt
(`world.cpp:2128`) is `10 + (level-1)*15/24` % of the current bar: **235 xp per
death at L5**, and de-level when xp goes negative. val2's 66 deaths ≈ **15.5k
xp of debt** — more than the entire L5→L8 requirement. Healing is OOC-gated
(`kOocRegenDelay = 400` ticks = 20 s, then +1 hp/2 s), so a bot that never
breaks contact never regens.

## Route v5 — what changed (`tools/bots/main.cpp` only)

1. **Edge-stand the ladder.** L3+ waypoint `(55,19)` → `(55,14)`: the east road
   `PATH x[21,62] y[14,15]`, 3 tiles north of the ghoul rect. Ghoul aggro 6
   means only the north row pulls instead of all 8; the leash (14 from anchor)
   lets a retreat break the fight. L1 `(46,13)` / L2 `(15,6)` unchanged.
2. **Pack cap that ignores level, L3+ only.** Skip any target with 3+
   aggressive mobs clustered (`pack >= 2` counting others within Chebyshev 3),
   regardless of the level difference — the own-2 "era fodder" exemption is
   gone. Passive mobs (`aggroRadius == 0`, i.e. Marsh Rat) are excluded from
   the count via `entAggressive()`, so the L1 rat camp stays diveable, as does
   the L2 bat swarm by design.
3. **Retreat home, not away** on map 1. Path to `homeX/homeY` (~`(32,16)`,
   outside every aggro radius) instead of the threat-axis flee; the flee is
   kept for maps 2/3. Sticky until healed **and** unpursued, so the bot cannot
   pivot mid-flight and re-aggro the same pack.
4. **Defend while disengaging.** The retreat branch `continue`s before the
   attack block, so a fleeing bot never swung — it walked home as a free
   punching bag. It now attacks anything already at point-blank (and the
   target-selection `retreating` skip moved to `d > 1`, "no new pulls", not
   "no fighting").
5. **Swarm panic-break at 5, not 3.** `swarmOnUs` (aggressive mobs within 2
   tiles) forces a retreat at 5+ regardless of hp. The first cut used 3, which
   disengaged from every *normal* ghoul trade at the north edge and zeroed XP
   gain — caught because a 540 s run produced no L6 at all (`val3`, killed).
   5 is a genuine dive-death threshold; the hp<50% and hp<35% bands still
   handle ordinary trades.

## Result (val4: resumed DB at L5, 540 s, same harness as val2)

| | val2 (v4 + armor fix) | val4 (Route v5c) |
|---|---|---|
| maxLevel | 5 | **7** |
| deaths (total) | 66 | **10** |
| bot 00 deaths | 32 | **0** |
| bot 00 killerByLvl | L7:2 L5:8 L3:22 | *(empty)* |
| replay | OK 0 mismatches | OK 0 mismatches |

Bot 00 climbed L5→L6 at t=47 s and **L7 at t=531 s without dying once**;
bot 01 took 10 deaths (all L5 hounds, `lastDeath=(25,30)`) to reach L6. Suite
105/105 (327,925 assertions), ctest 2/2, journal epoch unchanged at 6.

## The 12-leg chain (fresh DB) — Route v5c

In flight at issue time (started 18:41 UTC); the leg table below is filled from
`logs/m2b_rerun_gate.log` once it lands. val4 above is the primary validation:
a resumed-DB leg at the exact level band where v4 stalled (L5), same harness,
same seed — and it cleared L7 with zero deaths on bot 00.

Full 12-leg run (`tools/m2b_rerun_chain.sh`, fresh DB, 2 campaign bots,
~10.8k ticks/leg, characters persist across legs): **every journal replays 0
mismatches**; `TARGET L8` never fires. Methodology identical to devlog 0028 —
`peak` = last "reached L" line, `end` = SUMMARY `maxLevel`.

| leg | peak | end | deaths | kills | replay | baseline (0028) peak/end/deaths |
|---|---|---|---|---|---|---|
| 1 | L5 | L5 | **8** | 128 | OK 0 mm | L3 / L3 / 58 |
| 2 | L5 | L5 | 60 | 127 | OK 0 mm | L5 / L5 / 14 |
| 3 | L5 | L3 | 142 | 99 | OK 0 mm | L6 / L6 / 40 |
| 4 | L5 | L5 | 26 | 113 | OK 0 mm | L6 / L5 / 76 |
| 5 | L4 | L2 | 134 | 72 | OK 0 mm | L6 / L3 / 128 |
| 6 | L5 | L5 | **4** | 136 | OK 0 mm | L3 / L1 / 182 |
| 7 | L5 | L4 | 116 | 122 | OK 0 mm | L4 / L4 / 56 |
| 8 | L5 | L5 | **10** | 118 | OK 0 mm | L4 / L4 / 48 |
| 9 | L5 | L5 | 30 | 129 | OK 0 mm | L4 / L3 / 68 |
| 10 | L5 | L4 | 90 | 94 | OK 0 mm | L5 / L4 / 22 |
| 11 | **L6** | **L6** | 20 | 116 | OK 0 mm | L5 / L5 / 16 |
| 12 | **L6** | **L6** | 22 | 123 | OK 0 mm | L6 / L6 / 32 |
| **Σ** | — | — | **662** | 1477 | 12/12 | — / — / 740 |

**Verdict: the floor came up a full level.** Mean end level 4.08 → **4.58**;
the death spiral no longer collapses bots into the starter bands — worst close
is L2 (leg 5) against the baseline's **L1** (leg 6, 182 deaths), and legs
ending at L5+ go 5/12 → **8/12**. Peaks never drop below L4 (baseline spent
legs 1, 6, 7, 8, 9 at L3-L4). Total deaths fall only ~10% (740 → 662) — v5c
still takes heavy legs (142/134/116) — but the debt now costs a level or two
instead of the whole ladder, which is exactly what "retreat home and regen"
buys: the bot walks the loss off and comes back.

The ceiling is unchanged at **L6** (legs 11-12 reach and hold it, same as the
baseline's legs 3/12). val4's L7 came from a resumed DB with gear already
banked; a fresh-DB chain still ends its legs at L5-L6. L7→L8 stays blocked by
the content gate below.

## Scope boundary that remains

L7→L8 still has no reachable step-up on map 1: `gnolls_pits (50,42)` L7 and
`widow_glade (58,42)` L9 sit south of the river (`WATER y[34,37]`), and the
only crossing — bridge `PATH x[14,16] y[34,37]` — runs 3 tiles past the L11
gravecaller barricade. A solo/pair campaigner cannot open that without a
**content** change (move the barricade, add a crossing, or re-anchor the
gnolls). That is a card for the director, not a bot-profile fix; flagged, no
mob numbers moved.