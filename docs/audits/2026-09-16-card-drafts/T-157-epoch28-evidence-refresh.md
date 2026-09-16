# T-157 — Evidence refresh: M4 flip, M1 30-min, M2/M3 pacing, substantive leg of record (P0, audit 8/20)

## Context
Audit findings **A13 / A18 / G / P0-gate**. At epoch 27 the **only** replayable
leg of record is `logs/t146.bwj`: `ticks=242 sessionCmds=16 hashes=2
mismatches=0` — five wanderers for ten seconds. Every other committed leg refuses
with exit 4 (correctly). Meanwhile the gate claims that justify "playable" predate
the merge repair: M4's 3/3 flips are devlog 0098 at epoch 25, M2/M3 pacing is
T-113/T-114-era, and `docs/ops/friday-night-readiness.md` still pins "epoch 26"
and instructs the GM to use **`gm siege-now`, which no longer exists**.

This audit's own drill shows why the refresh matters: 8 attackers + 4 defenders
for 210 s breached **both gates** (`reg=8 breach=240 crown=8`, p99 733 µs, replay
`mm=0`) but produced **`attuned=0 crowned=0`** — the heartstone needs 60 s
uncontested within 3 tiles (`kHeartCaptureTicks=1200`) and the defenders held the
ring. Nobody currently knows whether the flip still happens on the merged tree.

## Scope
- Re-run at the current epoch (28 if T-151 landed, else 27), on scratch DBs:
  1. `tools/t137_m4_leg.sh 7836 12 6 750` — M4: flips ≥1 (target 3/3), p99 <25 ms.
  2. 20 bots × 1800 s fighter soak, `--p99-budget-ms 10` — M1 shape.
  3. One M2 leg (`bh_duel` era table + balancer TTK feed) and one M3 party-vs-solo
     XP/hr leg (`tools/m3_*`), recording the numbers against MVP §4 targets.
  4. A **substantive leg of record**: one recording covering siege + pledge +
     refine + trade + mine + PK + cross-zone (multi-wave, sqlite-seeded — **not**
     `--bless`, see T-155), replaying `mm=0`, committed with `git add -f`
     (`.gitignore` lists `logs/*.bwj`).
- Fix `tools/*_leg.sh` build-dir hard-coding (`build/linux-gcc`) to honour an env
  override, and correct the stale "recording epoch 25/26" comments.
- Update `docs/ops/friday-night-readiness.md` (epoch/schema line, `gm siege-start`
  not `gm siege-now`) and README §Status's gate-leg reference.
- Add a CI leg that replays the committed leg of record (cheap: no bots, one
  `--replay-world` invocation) so a future epoch bump cannot silently orphan it.
- OUT: fixing anything the runs expose (file cards instead); M5 (200×12 h + MBA
  fps) stays director-scheduled.

## Acceptance criteria
1. Four run reports pasted (command, date, HEAD, scratch DB path, decisive lines).
2. M4: ≥1 flip with the `crowned:` line, or a filed P1 card explaining why not.
3. M1: `soak OK p99=… budget=10.00ms` over 1800 s; RSS start/end recorded.
4. M2/M3: numbers vs MVP §4 targets with a PASS/FAIL/STALE verdict each.
5. New leg of record replays `mismatches=0` on a clean checkout; CI replays it too.
6. Ops/readme doc lines corrected (diff shown).

## Evidence owed at merge
Run logs (scratch, quoted in the card), the committed leg, CI run URL, devlog,
board row.
