# 0035 — Aura tiers III–V audit: already shipped, gates hold (S23)

S23 of the overnight queue. The brief's premise ("T-042 shipped only I–II")
was stale: **T-047 landed the top tiers** (`tests/test_combat.cpp:638/677/712`).
Per the corrected brief this is audit-only: no code change, no epoch bump
unless gate drift is found. None was.

## Gate check (pinned 20/50/80/120/150 weapon-skill law, RFC 0001)

`shared/content/auras.h` `kAuraTiers`:

| tier | reqSkill | parts | gold | effect encoding |
|---|---|---|---|---|
| I | 20 | 4001×30 | 120 | +3 atk flat |
| II | 50 | 4002×15 | 400 | regen 2 (%hpMax/60t) |
| III | 80 | 4004×10 | 1200 | proc 1 (cleave) |
| IV | 120 | 4005×5 | 4000 | proc 2 (sunder) |
| V | 150 | 4005×3 | 8000 | proc 3 (graft) |

Gates match the law exactly. Effects match GDD §3 (III = 3-target
multiattack cleave; IV = sunder; V = graft/lifedrain) and the T-047 pins:

- III (`test_combat.cpp:638`): aura-3 blade bleeds adjacent packmates with
  the same rolled damage (up to 2 in reach).
- IV (`:677`): sunder procs add +35 flat — max observed tick-loss clears 35
  against a table that otherwise caps well under it.
- V (`:712`): graft procs heal the attacker (thread-hanging survivor test).

## Replay parity spot-check

T-047 procs are combat-sim: tonight's epoch-10 grinder journal
(`logs/t071.bwj`, 109+122 fighter/pilgrim kills incl. aura-eligible melee)
replays `[replay] OK … mismatches=0` — parity holds across all three tiers
in live traffic, not just the pins.

## Verdict

No drift, no fix card, no bump. Epoch stays **10**. S23 closes as
audit-only; the queue's implementation budget stays on S24.
