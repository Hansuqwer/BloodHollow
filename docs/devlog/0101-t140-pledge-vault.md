# 0101 — T-140 pledge vault (schema v14, no epoch bump, stays 26)

Phase P closed: deposit-only pledge vault (sworn-holder drip routing +
`/pledge tithe N` kind 41 + `/pledge vault` readout), schema v14,
disband burns the pool (MVP: no payout economy).

Evidence: 291/291 (5 new) · `logs/t140.bwj` vault=500 persisted through
relog, kind-41 journaled, replay mm=0 · `t138`/`t139` re-replay mm=0
(neutrality) · validate 0/6 · duel selftest green. Version-pin cascade
13→14 in T-130/T-122 tests (T-121 precedent).

Next: Phase A art QA-pack (40 landed plates) — needs the Pollinations key
check + plate inventory first.
