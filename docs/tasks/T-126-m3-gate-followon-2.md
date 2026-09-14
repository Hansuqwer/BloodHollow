# T-126 — M3 gate follow-on #2: confirm the r21 mode, break the x=15 wall, attempt the boss verdict

## Context

T-125 (devlog 0086, handover `T-125-r18-r22.md`) made the font reach
repeatable — r19 3/5, r20 4/5, **r21 4/5 at 231.7–237.9 s** with 3–5 elite
and 12–14 trash kills per bot — and moved the wall from map 3 to **map 5
x=15**: the party clears (6,21), holds (13,10), and dies mid-march between
(13,10) and (22,10) inside the overlapping aggro fields of the apse elites
(1010, (16,4,4,3)/(25,4,4,3), r8), the Sexton (1008, (21,5,3,3), r8) and —
at the x=21 boundary — the Gravemother (1009, (21,2,4,3), r8, hp 700 dmg
40). No leg in the series has ever killed the Gravemother; the **boss
verdict is still not established**. Handover §5 orders the next moves:
(1) re-run r21 (its 4/5 is n=1), (2) break the x=15 triple-aggro — option
(a) an extra node at (19,10) — (3) then the boss verdict with the
assembled configuration.

## Scope

Bot/tooling + logs + docs only. No sim/server/protocol/schema/epoch change.

- **r23**: re-run the r21 configuration unchanged (same binary, 900 s,
  same staged DB) to confirm 4/5 is its mode and not its lucky leg.
- **r24**: r21 + a fifth map-5 node at **(19,10)** (verified walkable in
  the shipped `assets/maps/drowned_crypt.bhmap`: blocked=0, ground=1, on
  the y=10 corridor, 5 Chebyshev from the Sexton rect; rest 0 — the
  existing map-5 waypoint quorum hold already applies). Intent: the apse
  elite (16,4) is killed as a stacked fight at the node instead of while
  the column is strung out mid-march at x=15.
- Fresh journals `logs/m3_gate_r23.bwj` (+ `r23b` if the re-run rule
  fires) and `logs/m3_gate_r24.bwj`, force-added; the r17 journal of
  record `logs/m3_gate.bwj` restored byte-identical at close-out
  (md5 `b4c0cf3f3255e38071c172cc3a3336bd`).
- Devlog 0087, board row, handover `T-126-*.md` if the verdict is still
  open.

## Acceptance criteria

- [ ] Headless build warning-free; suite 209/209 · 329,058 assertions,
      0 failures (unchanged — no test-visible behaviour moved).
- [ ] `bh_duel --selftest` unchanged: win=1 ticks=41 hpEnd=82
      hash=b273be661b54673a DETERMINISTIC.
- [ ] `logs/t120.bwj` replay mm=0; epoch 21 untouched.
- [ ] r23: 900 s leg on the r21 binary; `[raid]` evidence recorded
      (deaths, map-5 entries, reach times, bossSeen, p99); journal
      `m3_gate_r23.bwj` replays mm=0.
- [ ] r24: 900 s leg with the (19,10) node; same evidence recorded;
      journal `m3_gate_r24.bwj` replays mm=0.
- [ ] Boss verdict recorded from the legs in one of three shapes:
      KILLED (kill + TTK + party state), STILL ALIVE (per-bot deaths at
      the font, where the party fails next), or NOT REACHED (deepest x,
      killer composition). No other shape is written.
- [ ] Epoch 21 / wire 237 / schema v11 / duel pin all unchanged and
      re-verified.

## Tests required

- Existing doctest suite unchanged (no new sim behaviour to pin — this is
  a bot staging change; the r22 lesson on unexercised branches applies:
  the node is a route-table row, and the leg IS the test).
- Gate legs + replay evidence pasted in the PR description.

## Out of scope

- The r22 kiter band on map 5 (reverted in T-125; re-land only behind a
  reaching leg with the front-runner guard intact — see handover §3).
- Content-side changes to the depths (spawn rects, mob stats, aggro) —
  that is a content card with an epoch discussion, not this bot card.
- Raising the party level above L13 (director call, handover §5.4).
- F1–F5 / trade / economy / persistence — untouched.

## Deviations

- (filled at close-out)
