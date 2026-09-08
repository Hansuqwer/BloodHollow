# 0032 — Smugglers' fence: Sable opens the no-questions lane (T-069)

S21 of the overnight queue. Inherits a mid-flight Cline session: server
fence logic + `test_fence.cpp` + epoch-8 bump were on disk, uncommitted,
with one suite failure. Audited, repaired, finished (client crate panel),
soaked, replayed.

## Audit of the inherited work

Cline's tree (8 modified + `test_fence.cpp`) was reviewed hunk by hunk:

- **Fence core — sound.** `nearFence`/`fenceBuy`/`fenceSellJunk`, item-disjoint
  lane routing in `applyWorldCommand`, 60%/25%-markup/karma-gate pins, Marta
  gates untouched. Kept as-is.
- **One-Marta fix — real bug, kept.** The vendor spiral had lost its break
  and seeded ~425 duplicate Martas (entity bloat + no free town tile for new
  furniture). Live proof: T-033's soak held 544–584 entities; tonight's
  identical 14-bot mix holds **~330–360**. One Marta, as T-027 documented.
- **Combat hunks — scope creep, kept + flagged.** Retaliation now fires on
  the strike itself (miss no longer skips it) with stale-path drop, and
  `mobThink` clears the wander path on acquire. Plausible (a mob notices
  being swung at; a chase should start fresh), covered by the same epoch
  bump, reverting risked the T-047 pins. No separate card; flagged here.
- **Fence seed nesting — cleaned.** Cline nested the fence spiral *inside*
  the bounty-board search loop (shadowed `r/dy/dx`, and a board-miss would
  skip the fence). Hoisted to `World::spawnFence`, called unconditionally.
- **Suite failure — test bug, not sim bug.** `zones: stepping onto a portal`
  asserted per-tick hash churn; with the bloat gone the idle world is
  legitimately static (furniture never thinks, passive rats wander on-slice
  only). The old CHECK was coupled to the 425-vendor ID layout shifting rat
  think slices. Replaced with a bounded eventual-churn loop (200 ticks).
- **Judgment calls (director review):** (1) buy-lane routes *by item*, not
  proximity — Marta at the plaza and Sable at the gallows are never both in
  range, and disjoint stocks make the branch unambiguous and replay-exact.
  (2) Client goes one step past the brief's render-only scope: the vendor
  panel opens at the fence with Sable's crate appended (F6–F8, markup
  prices) over the existing `BuyRequest` path — otherwise `fenceBuy` is
  unreachable for humans. No wire change, no new art (generic furniture
  rect + name label already covers kind 69).

## The soak (epoch-8 journal)

`bh_server` (fresh DB `/tmp/t069.db`, `--soak-secs 700`, journal
`logs/t069.bwj`, epoch 8) + the T-033 grinder mix on port 7869 (600 s):

| group | profile | count |
|---|---|---|
| t69w | wander | 5 |
| t69f | fighter | 4 |
| t69p | pilgrim | 3 |
| t69c | campaign (target L8) | 2 |

## Results (group SUMMARY lines)

| group | kills | deaths | maxLevel | shops | notes |
|---|---|---|---|---|---|
| campaign ×2 | 71 | **0** | **4** | 5 | L2 t≈10 s, L3 t≈91 s, L4 t≈412 s; killerByLvl **empty** |
| fighter ×4 | 115 | 60 | 4 | 3 | killerByLvl spread L3/L5/L7/L11 (L11 leads, see watch) |
| pilgrim ×3 | 100 | 42 | 4 | 4 | same spread; anvilTries 0 (parts/gold gates — standing watch) |
| wander ×5 | 0 | 24 | 1 | 0 | pacifist decoys |

- **Campaign route health:** 2 campaigners L1→L4 with zero deaths and an
  empty killer histogram while 12 bots churn — route holds in a crowd.
- **Death geography (standing watch from 0031):** fighter/pilgrim
  lastDeaths again at (12–13,38–40), the bank-road corner night-graze of
  the relocated gravecallers. Spread across bands, not clumped on one kind.
- **Perf dividend of the one-Marta fix:** entities 321–343 (T-033: 544–584);
  p50 ~2.3–3.1 ms / **p99 ~3.8–5.6 ms at 14 bots** (T-033: 12.9–16.4 ms).
  Idle-tail p99 0.74 ms. Under every budget on the board.

## Balance bands (server-computed, live kills)

| mob | L | kills | TTK | T-033 | band |
|---|---|---|---|---|---|
| Marsh Rat | 1 | 44 | 2.6 s | 2.9 s | ✓ starter |
| Plague Bat | 2 | 17 | 4.4 s | 3.1 s | ✓ swarm-fodder |
| Feral Ghoul | 3 | 30 | 13.9 s | 8.9 s | ✓ (watch: +56%) |
| Hollow Hound | 5 | 12 | 55.9 s | 18.2 s | ✓ punitive (watch: small sample) |
| Bonepicker Gnoll | 7 | 3 | 65.6 s | 21.0 s | ✓ (watch: n=3) |
| Gravecaller | 11 | 3 | 451.8 s | 169.6 s | wall by design (n=3) |

Ordering preserved (starter < fodder < mid < punitive <<< wall); no
off-band mob. The mid-band slowdown vs T-033 is recorded as a **watch
item, not a fix**: samples are small (600 s vs 720 s leg), deaths are
*down* (60 vs 76 fighter), and no blind lever is moved overnight — T-074
owns bot-death tuning with its measure-first discipline. Possible second
order from retaliation-on-miss (mobs answer the first swing now); if T-074
confirms it, the lever is documented here first.

## Wipe replay (T-032 contract, epoch 8)

`./build/server/bh_server --replay-world logs/t069.bwj` →

`[replay] OK ticks=14001 sessionCmds=5984 hashes=141 mismatches=0 entities=343`

Fresh epoch-8 journal replays bit-exact. Old (epoch-7) journals refuse
cleanly by contract (exit 4).

## Files
`server/src/{world.cpp,world.h,command.h,main.cpp}`,
`shared/content/{items.h,wirekind.h}`, `client/src/{game.cpp,game.h}`,
`tests/{test_fence.cpp,test_combat.cpp,CMakeLists.txt}`. Suite **112 /
327,969**, ctest 2/2, warning-free.
