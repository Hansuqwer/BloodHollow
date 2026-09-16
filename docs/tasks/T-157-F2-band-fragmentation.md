# T-157-F2 — Siege-bot band fragmentation (filed 2026-09-16, quality)

**Status:** `open` — observation from `m4e29`, not a gate blocker.

## Finding

12 attackers enlisted as SEVEN bands (4+1+1+1+1+1+3, server log) instead of ~3
quintiles. Quintile doctrine (devlog 0098) assumes mustering at spawn; invites
refuse past 12 tiles (T-138 law), so scattered arrivals band alone. M4's 8-band
cap was never stressed (7/8); a real 20v10 night could fragment worse and hit
the cap with singletons.

## Scope

Tighten drill muster (wait-for-quintile before `/siege-reg`, or rally-point
hold until T+X), then re-measure band count in the F1 re-run. Bots-only; no sim.

## Acceptance

F1 re-run shows ≤4 attacker bands for 12 bots; doctrine note in the M4 devlog.
