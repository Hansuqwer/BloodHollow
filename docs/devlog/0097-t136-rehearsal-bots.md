# Devlog 0097 — T-136 rehearsal mode + siege bots (Phase S, 4a/4)

## What

`--siege-rehearsal` opens the window unconditionally (loud, never
default); the journal `v` line carries a `rehearsal` marker with
exit-4 cross-mode refusal both directions. Default path byte-identical
(t135 replays mm=0) — no epoch bump. Bot profile `siege` (attackers):
march blind → portal → `/siege-reg` → bot0 `gm siege-start` → 15-quota
breaches per gate → stone hold → `/crown` loop; death re-marches;
fighter targeting everywhere except the march; 30 s drill traces;
SIEGE verb telemetry.

## Drill verdict (900 s, 6 bots, L15+plate top-up, disclosed)

- 6 bands, battle joined, BOTH gates breached, ATTUNED (tick ~2987),
  369 crown verbs, replay mm=0 (18042 ticks / 4789 cmds / 180 hashes),
  cross-mode refusal exit 4. t135 replays mm=0 (default-path proof).
- NOT crowned live: kennel hounds (wander 10, leash 16, 3.5 s refill)
  + ghouls roam the stone ring and kill/maul still crowners (14 deaths
  in the blind run; chase drag in the armed runs). Four drill iterations
  mapped it: march-blindness fixed wildlife drag (reg 2→6); quota
  40→15 fixed stage convergence; printf markers (not broadcasts) are
  what server-log assertions must grep.
- Consequence, stated: the live crown flip (Friday leg-3 shape) moves to
  T-137 with defender bots + a point-blank-hold mode — T-133 already
  proves channel/holder/battle-end at unit level. No silent scope cut:
  this card's acceptance is register→start→breach→attune LIVE.

## Evidence

- `test_siege_sched.cpp`: rehearsal window pins. Full ctest 2/2 (271).
  validate 0/6. Duel pin unchanged. Leg: `tools/t136_siege_drill.sh`.

## Next

T-137: defender profile + hold mode + 40-bot rehearsal + M4 verdict
(p99 <25 ms, 3/3 flips). Then Phase P (pledge-lite), Phase A (art QA),
Phase O (ops + Friday/M5).
