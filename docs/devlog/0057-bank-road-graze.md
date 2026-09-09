# 0057 — The corner guards itself (T-088)

T-088, audit-only. The bank-road graze is the oldest living watch item (born in the T-033 gate leg, carried through three handoffs). Eight legs later the verdict is finally written down: there is nothing to fix.

## What the numbers say

Two identical 240 s legs: zero L11 kills, then ~twenty. Same binaries, same profiles, same map. That single comparison retires the idea that the graze is drifting anywhere — it is roamers meeting a wall mob at night, at whatever rate the RNG deals that leg. The campaign route, the thing the barricade was moved to protect, is 8-for-8 clean.

## Judgment calls

- Declined the prescribed rect tweak even though it was pre-approved in prose: re-ran the Chebyshev and it does not clear the corner at max wander + night (7 ≤ 8). Shipping an epoch bump for a fix that does not fix would be the worst kind of progress — motion with a version number.
- The day the corner DOES show a campaign-route pull (killerByLvl with L11 on a campaigner, lastDeath on a route tile), the response is the rect move as its own sprint with the full content-change liturgy: mapgen truth, regen, epoch bump, fresh 540 s leg. Until then the L11s can keep their corner.
