# Campaign pack wall — analysis & execution prompt (issued 2026-09-07)

Analyse and close the **L6 over-level wall** card opened by the T-034d chain
rerun (`docs/devlog/0028-plateau-moves-wall-relocates.md`). Governing docs:
`docs/02-gdd.md` §4-5, `docs/tasks/done/T-034c.md`, `T-034d.md`. Standing
constraints unchanged from `phase3-remainder.md`: deterministic sim only,
replay must stay bit-exact after every change, never regress the solo
baseline, and **no content/price/mob number moves without a card** — the
lever here is bot-profile behaviour only.

## 1. What was worked on (state at issue time)

Devlog 0028 concluded the plateau moved L4-L5 → **L6** and hypothesised the
new wall was "the over-level gnoll (L7) / widow (L9) triangle". Two fixes were
then made on top and are **uncommitted** in `tools/bots/main.cpp`:

1. **Armor-equip de-gated from `nearTown`.** The T-034d re-gear trip buys Hide
   Armor (2101) in town, but the equip branch was `nearTown`-gated while the
   blade branch was not — so the bot walked back to camp *before* the equip
   fired and fought **blade-only** forever. DB proof: `2101:1:0` (owned,
   unequipped) on both characters. After the fix: `2101:1:1`.
2. **No-walls filter tightened** `d > 2` → `d > 1` (strike back at
   point-blank only).

Validated by `logs/val1_*` (300 s) and `logs/val2_*` (540 s, replay 0
mismatches), resuming the chain DB at L5. Armor equips, L11 gravecaller deaths
drop out of the histogram, deaths 32 → 24. **But `maxLevel` stays 5.**

## 2. The hypothesis in 0028 is wrong — evidence

The death diagnostics added in `a262909` (`killerByLvl`, `lastDeath`) disprove
it. Histograms are **L3-dominated**, not L7/L9:

| run | level | deaths | killerByLvl | lastDeath |
|---|---|---|---|---|
| `diag_l6` (fresh climb) | L3 | 20 | **L3:10** L5:2 / L3:4 L5:2 L2:2 | **(55,18), (55,20)** |
| `diag_l6b` (resumed) | L5 | 32 | L5:6 **L3:8** L11:2 / **L3:6** L5:8 L11:2 | (24,46), (18,30) |
| `val2` (resumed, 540 s) | L5 | 66 | L7:2 L5:8 **L3:22** / L7:2 **L3:22** L5:10 | (24,46), (25,30) |

L3 = **Feral Ghoul**; L7 = 2 deaths total, i.e. noise. Two independent
findings:

**(a) The waypoint is the pack centre.** Route v4 parks every level ≥3 at
`(55,19)`. `tools/mapgen/make_thornwall.py:156` is
`spawn("ghouls_east", 52, 17, 6, 5, MOB_FERAL_GHOUL, 8, 500)` → rect
x[52,58] y[17,22] with **maxAlive 8**. `(55,19)` is dead centre (and is the
DARKGRASS lair patch `rect(ground, 55, 19, 60, 23, ...)`). `lastDeath` lands
on `(55,18)`/`(55,20)` — **bots die standing on their own waypoint.** Ghoul
aggro 6 (7 at night), so all 8 engage; 8 × 9 dmg on an 18-tick (0.9 s) cycle
deletes a 94 hp L3 bot in ~1.5 s.

The "pull singles" filter waves this through by design — `if (pack > 0 &&
kv.second.level >= b.level - 1) continue;` with the comment "trash swarms
(own-2) are era fodder and may be dived freely". At L5 a L3 ghoul fails
`3 >= 4`, so an **8-mob cluster is treated as free fodder**. The filter is also
gated on `d > 1`, so at point-blank there is no density check at all.

**(b) The retreat walks into worse content.** The v4 disengage flees ±10 tiles
*along the threat axis* (`fx = tileX ± 10`). From `(55,19)` fleeing west runs
the bot down the south road `PATH x[14,15] y[17,33]`, which passes **3 tiles**
from `gravecaller_barricade (6,18,5,4)` — **L11 Gravecaller, dmg 30, aggro 7,
leash 12**. Fleeing south-west reaches `hounds_marsh (10,40,10,6)` — 8 × L5
Hollow Hound, dmg 13, wander 10, leash 16. That is exactly the `L11:2` and
`L5` kills at `lastDeath=(18,30)/(24,46)`, nowhere near camp.

## 3. Why deaths, not XP, are the binding constraint

`shared/sim/combat.h`: L5→6 = 1960 xp, L6→7 = 2750, L7→8 = 3660. Ghoul = 90 xp
→ ~94 kills for L5→L8. Death debt (`world.cpp:2128`) is
`pct = 10 + (level-1)*15/24` of the current bar: **235 xp per death at L5**
(≈2.6 ghoul kills), 357 at L6, and de-level when xp goes negative. val2's 66
deaths ≈ **15.5k xp of debt** — more than the whole L5→L8 requirement. Legs
with low deaths do climb (leg 2: 14 deaths → L5; leg 3: 40 → L6).

Healing is OOC-gated (`kOocRegenDelay = 400` ticks = 20 s out of combat, then
+1 hp / 2 s), so a bot that never actually breaks contact never regens — the
v4 flee keeps it in combat.

## 4. Scope boundary found: there is no reachable L6+ step-up

The only L7 content on map 1 is `gnolls_pits (50,42,8,4)`, **south of the
river** (`WATER y[34,37]` full width). The single crossing is the bridge
`PATH x[14,16] y[34,37]`, i.e. the same road that runs past the L11
gravecaller barricade; a gravecaller anchored at `(11,22)` with wander 5 sits
within aggro 7 of the bridge approach. `widow_glade (58,42)` L9 is behind the
same crossing. So a solo/pair campaigner **cannot** reach the L6→L8 step-up
without a content change (move the barricade, add a crossing, or re-anchor the
gnolls) — that is a card for the director, not something to fix here.

## 5. Execute (bot-profile only, Route v5)

1. **Edge-stand the ladder.** L3+ moves `(55,19)` → `(55,14)`: the east road
   `PATH x[21,62] y[14,15]`, guaranteed walkable (scattered standing stones
   only replace GRASS), 3 tiles north of the ghoul rect so only the north row
   pulls. Leave L1 `(46,13)` and L2 `(15,6)` alone — they work (bots reach L3
   by ~100 s).
2. **Pack-density cap that ignores level.** Skip any target with `pack >= 2`
   (3+ clustered) regardless of level — delete the "own-2 fodder may be dived"
   exemption. Add a `swarmOnUs` count (mobs within 2 tiles of *us*) and force
   `retreating` at 3+.
3. **Retreat home, not away.** When `mapId == 1`, path to `homeX/homeY`
   (≈`(32,16)`, outside every aggro radius; ghoul leash 14-from-anchor drops
   the chase en route, and mob leash-off full-heals them). Keep the old
   threat-axis flee for maps 2/3. Make `retreating` sticky until healed **and**
   unpursued so the bot cannot turn around mid-flight and re-aggro.

## 6. Definition of done

- Build warning-free; `ctest` 2/2 and the doctest suite 105/105 unchanged
  (journal epoch stays 6 — client-side only).
- Fresh-climb single leg beats the `diag_l6` baseline (20 deaths / maxLevel 3)
  and resumed leg beats `val2` (66 deaths / maxLevel 5).
- `killerByLvl` no longer L3-dominated at the waypoint; `lastDeath` no longer
  clusters on `(55,18-20)`.
- Full `tools/m2b_rerun_chain.sh` 12/12 replay 0 mismatches, compared
  leg-by-leg against the devlog 0028 table.
- Devlog + board update + commit under the bloodhollow-dev identity. The
  river-crossing/L11-barricade finding is written up as a content card, not
  silently worked around.
