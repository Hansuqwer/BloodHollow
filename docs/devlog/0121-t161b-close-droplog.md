# 0121 — T-161b CLOSE (M3c: first boss kills) + T-159f1.4/.5

## M3c verdict (full kit, mirror fixed)
`logs/t161b_m3_m3e30c.bwj` (L13→16 raider party, 900 s): **bossKills=3**,
bossSeen 20, deaths 12 (2–4/bot, down from 84–88), party=5 held, replay
mm=0 (180 hashes). Every new rite fired live (c11=6 through c23=2).
First boss kills in the entire gate series (r8–r22, M3a, M3b: all 0).
**M3 re-measured with the fuller kit: PASS.** T-161b CLOSED — epoch 30
throughout, suite 370/370, seven clean replays, no epoch bump for six
kit items + hotkeys. Bots lesson: profile-gated support shapes need a
parity mirror per path (crypt vs campaign/raider); shared next*At/counters
keep the mirrors mutually exclusive.

## T-159f1.4 Boneyard pin
Twin-world damage-uplift formulation (4000 swings each; hero crits report
kind 5, so uplift — not counts — is the observable). +250 floor sits
several σ under the ≈+1200 mean: seed-proof, still fails loudly on a dead
path. Appended to `test_affix_v2.cpp`.

## T-159f1.5 drop-log instrumentation
`World::dropLog` + `setDropLogPath` (T-080 posture: default
`logs/drops.log`, git-ignored, replay-duplicate harmless). One pinned line
per gear/unique/junk/gold award. Pin: `tests/test_droplog.cpp` (gold line
unconditional on a ghoul kill).

## T-159f1 remainder (deferred, priced)
- .1 multi-affix: needs ADR + schema + blob grammar — own wave.
- .2 per-band tables: sim change = epoch — batch with the next epoch wave.

## Still open after this pass
T-157-F2 (fix-landed, needs siege-leg re-measure) · T-157-F3 (director:
sim-side (b) vs accept (c)) · T-164 (live matrix + mask) · T-156/T-ART
(renderer) · T-165 (director merges) · T-146..149 (human) · M1/M2 refresh.
