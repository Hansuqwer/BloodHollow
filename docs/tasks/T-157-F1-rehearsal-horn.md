# T-157-F1 — Rehearsal horn vs GM allowlist (M4 blocker, filed 2026-09-16)

**Status:** `open` — the single blocker behind the epoch-29 M4 FAIL (`done/T-157.md`).

## Finding

`logs/t137_m4e29_server.log: [gm-denied] tick=1494 name=t137a__00 verb=gm siege-start`.
T-152 gated ALL `gm` verbs on the operator allowlist (empty by default; `BH_GM_NAMES`
unset in the drill). Bot0 sounds the horn exactly once (bots `siegeStartSent`, t0+20s);
the denial is quiet; `breach()` refuses while `!siegeBattleActive()` — so 12 attackers
rammed closed gates 360× and crowned nothing 659× for 750 s. Pre-T-152 drills (devlog
0098, audit drill) passed because the verb was ungated. No sim bug: determinism mm=0,
p99 3.0 ms.

## Scope (pick after director nod; staff recommends a+b)

- (a) Drill-only, 1 line: `t137_m4_leg.sh` exports `BH_GM_NAMES=t137a__00` (matches the
  disclosed server-down top-up precedent in the same script). Unblocks evidence today.
- (b) Kill the single-shot race class: bot0 retries `gm siege-start` until battle-active
  is visible (T-151 siege panel phase ≥2) or N tries; script asserts "battle joined"
  ≤60 s after regs or aborts fast (fail in 2 min, not 15).
- (c, optional) Server: rehearsal mode bypasses the allowlist for `siege-start` only.
  Widens the T-152 audit surface — needs explicit director approval; NOT recommended
  while (a) exists.

## Acceptance

`tools/t137_m4_leg.sh 7849 12 6 750 m4e29b` → `battles ≥1`, then M4 re-verdict
(flips ≥1 + p99 <25 ms) recorded in `done/T-157.md`. Replay mm=0 on the new leg.

## Out of scope

Band fragmentation (T-157-F2); sim/balance changes.
