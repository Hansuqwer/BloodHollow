# Friday-Night Test readiness (MVP §6) — agent-mapped, human-run

The 4-leg acceptance needs real players (director-owned). This page maps
each leg to runnable tooling + entry criteria so the night is plannable.
Status 2026-09-15: legs 1–3 runnable on current stack; leg 4 is a survey.

## Leg 1 — two 5-man parties race to 12 in 3 h (bots backfill to 20)

- Run: live server (`--db` alpha DB), 10 humans + `./build/linux-gcc/tools/bots/bh_bots --count 10 --secs 10800 --profile fighter` backfill.
- Entry: T-109 limiter needs staggered logins (waves ≤ 30 new accounts/60 s
  — brief the humans); accounts pre-created the day before preferred.
- Pass if: the Cultist players are fought over in chat (judged, not metered).
- Risk: 3 h is long for placeholder art (hero boxes) — set expectations:
  systems test, not beauty test.

## Leg 2 — Red Widow 3-way PK story

- Systems live: karma law (T-056), bands/red plates (T-057), guards (T-073),
  curse (T-070), duels (T-056), Widow uniques (T-128), EK fame (T-130).
- Stage: GM picks a named-elite timer overlap at the Widow, invites 3
  parties; pass if someone goes red, loses an item, and it becomes a story.
- Needs: GM runbook §2 (no /ban — brief the host firewall procedure).

## Leg 3 — impromptu siege rehearsal, 20v10 + bots

- Run: scratch DB rehearsal server (`--siege-rehearsal`), 20 humans attack
  + 10 defend + `siege` bots backfill (`tools/t137_m4_leg.sh` pattern;
  M4 proved flips 3/3 + p99 6.6 ms @40 bots).
- Pass if: ownership flips ≥ once + screenshots. Drill forensics
  (devlog 0098) apply: quintile parties, spawn mustering, kiters own the
  quiet window — brief team captains or the night eats itself.
- NEVER on the alpha DB (rehearsal writes holder state).

## Leg 4 — survey ≥ 70% "back Saturday?"

- Director-owned form; quota: every leg-1–3 participant before logout.

## Go / no-go checklist (night-of)

- [ ] Stack merged through T-144 (or rehearsal pins: epoch 26, schema v14)
- [ ] Backup taken (`bh_backup.sh backup`) + drill green this week
- [ ] Unit `bh-server` healthy (`systemctl status`, journal clean)
- [ ] Limiter briefed; accounts pre-created; GM runbook printed
- [ ] Scratch DB + ports reserved for leg 3; screenshot key assigned
