# 0122 — T-157-F2 CLOSE (server-side band merge, 12v6 measures 4 bands)

## Fix (T-139 extended, sim-side)
`World::siegeRegister`: when any LIVING member of the caller's party already
rides the war camp — the caller (re-reg mustering late mates) or an enlisted
mate (a straggler joining the party's band) — the missing living members
muster into the EXISTING band: no new slot, cap not consulted. Pure dups
stay quiet-false (T-131/T-139 pins preserved). No bots change (bots reg once
each; the server now merges). Hash-neutral by construction (siege rosters
ride outside `worldHash`, like pledge state): no epoch bump (stays 30 for
this card), no journal/wire/schema change. 3 new pins in
`test_siege_sched.cpp` (straggler merge, re-reg muster + full-camp bypass,
never-muster-the-dead).

## m4f2m verdict (card-literal 12v6 × 900 s @ epoch 30, `logs/t137_m4f2m.bwj`)
| Check | Result |
|---|---|
| regs → bands | 12 regs → **4 bands (1+4+5+1, 11 members)** + 1 party-muster → **F2 bar ≤4: PASS** (was 7 in m4e30f) |
| battle / gates | joined (horn OK), Outer + Inner breached |
| attuned / flips | 0 / 0 → M4 still FAIL (attune gap = F3 director call, unchanged) |
| p99 | 1.39 ms → PASS |
| determinism | unit pins + `m4e30f.bwj` re-replays mm=0 under the merge rule (hash-neutral proof). m4f2m journal is epoch-30; post-R2 binary (31) refuses it by contract |

Bots-only levers stay retired (7,7,6,7,7 across five legs); the merge is the
structural fix. NOTE: `siegeStart`'s "N bands" printf prints
`siegeAttackers_.size()` (members), not `siegeBandsUsed_` — pre-existing
mislabel, left alone (log text, not a pin).
