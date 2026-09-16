# T-157 — Evidence refresh at epoch 29 (OPEN, in flight 2026-09-16)

Source draft: `a1bd867:docs/audits/2026-09-16-card-drafts/T-157-epoch28-evidence-refresh.md` (written for epoch 28; executing at 29 after T-159).

## Landed this wave (docs, in `task/T-15x-audit-remainder`)

- `docs/ops/friday-night-readiness.md`: pins → epoch 29 / schema v14 / wire 242; `gm siege-now` (removed) → `siege-start`; `/ban` line updated (T-152 shipped); M4 epoch-25 citation flagged pending re-proof.
- `README.md` gate-leg refs refreshed (T-158).
- CI replays `logs/t159.bwj` every run (T-154) — future bumps fail loudly.

## Running now

- `m4e29`: `tools/t137_m4_leg.sh 7849 12 6 750 m4e29` with `BH_BUILD_DIR=build/headless` at epoch 29. Verdict (flips ≥1 + p99 <25 ms) lands in `done/T-157.md`; on flips≥1 the leg file is force-added as epoch-29 M4 evidence.

## Still owed (scheduled, not this wave)

- 20 bots × 1800 s fighter soak (M1 shape, p99 budget 10 ms, RSS start/end).
- M2 (`bh_duel` era table) + M3 (party-vs-solo XP/hr) numbers vs MVP §4.
- Anything the runs expose → fix-cards, not inline fixes (T-157 law).

## Evidence owed at close

M4 run lines, soak report, M2/M3 numbers with PASS/FAIL/STALE each, replay line, devlog, board row.
