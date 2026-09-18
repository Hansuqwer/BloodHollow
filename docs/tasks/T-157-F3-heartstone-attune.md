# T-157-F3 — Heartstone attunement denied by 6 holders (filed 2026-09-16)

**Status:** `open` — the remaining M4 gap after F1 (horn) landed.

## Finding (`m4e29b`)

Battle joined (12 bands), BOTH gates breached (~tick 5400), then nothing:
`attuned 0, flips 0` over the remaining ~200 ticks... (10k ticks / 20 Hz ≈
500 s). Attackers cycled the crown loop 659× against 6 defenders holding the
ring; the 60 s uncontested window (`kHeartCaptureTicks=1200`, 3-tile ring)
never opened. Same signature as the audit drill (`attuned=0`). p99 2.8 ms,
replay mm=0 — the sim is healthy; the contest is denied.

## Evidence update (2026-09-16): option (a) exhausted

`m4e30a` (16v6 × 900 s, epoch 30, `logs/t137_m4e30a.bwj`, replay mm=0):
battles 1, gates 2/2, attuned 0, flips 0, p99 8.9 ms PASS, 1119 crown
attempts vs 6 holders. Tripling the attackers does not open the 60 s
window — staff recommendation moves from (a) to **director call: (b) or (c)**.

## Scope (director picks ONE)

- (a) Drill-side: more attackers (e.g. 16v6) and/or longer clock (900 s), kiter
  split already in bots. Cheapest, no sim touch — try first.
- (b) Sim-side: revisit `kHeartCaptureTicks` / ring radius / defender grace
  (re-kneel guard precedent). Balance change → needs director + M4 re-proof.
- (c) Accept: record "M4 flips need ≥16v6 or (b)" as the standing gate shape.

## Acceptance

Whichever lever: a leg with `flips ≥1` + p99 <25 ms + replay mm=0, verdict
appended to `done/T-157.md`. Until then M4 stays FAIL-open with cause.

## Out of scope

Horn/allowlist (F1, landed), bands (F2).
