# 0066 — Three names looking for bodies (T-097)

T-097, design paper. The GDD has promised Old Maw, the Red Widow, and Cantor Vex since before Phase 3, with nothing but a timer range and a kill announcement to their names.

## What the paper does

Derives every stat from the one shipped elite (1010) instead of inventing a parallel system: same level as the zone's top base, hp a hair under, +2/+2, ~4× xp. The only invention with a pulse is Cantor-B — a half-rate bolt-caster, because shipping the crypt's signature elite as a Sexton palette-swap would waste the best name of the three.

## Judgment calls

- Fixed timers, not random: the GDD word "random" cannot survive contact with the determinism law (no wall-clock, no `random_device` in sim). Staggered fixed periods read as random to players and replay bit-exact. The GDD line needs one word changed when the cards land ("rotating"), flagged in the paper.
- One card per elite, not one elite card: rows + spawners + announce + tests + soak each fit the <400-line discipline separately; together they would be a review-proof blob. The paper is written so each card starts from its row of the table.
- First-kill flag session-scoped (vanishes at reboot, bounty-board pattern): persistent first-kill ledgers are EK-ledger-adjacent, and that ledger is blocked on Marrowgate. No new persistence for elites.
