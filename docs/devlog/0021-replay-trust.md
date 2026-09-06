# 0021 — replay green is currency again (T-049 closed)

## The story

T-049 haunted us since S10: cadence-25 hashes intermittently slipped live
vs replay under burst load. Every sprint since has claimed "replay
bit-exact" while a known superstition hovered. Today the superstition got
a fair trial — and was acquitted entirely *by the acceptance protocol*.

## What the record shows

Three consecutive 6-bot × 300 s soaks at `BH_HASH_CADENCE=25` (222–240
hash checkpoints each, 1918–2338 session commands including burst spam),
each replayed twice:

```
run1  replay OK  ticks=5561  cmd=1918  hashes=222  mismatches=0
run2  replay OK  ticks=6005  cmd=2317  hashes=240  mismatches=0
readers stable: replay1 == replay2 on all three legs
```

## How it died

Not by new code — by audit. The S10-era remediation (applyWorldCommand as
the single mutation source) plus journal stamping conventions (process-tick
for c-lines, next-tick for l/d lines, replay applies ≤t pre-tick in file
order) turn out to be fully consistent. I then hunted every
world-mutating escape hatch: session reads, wall-clock, unordered-map
iteration, pointer ordering, the bless sidecar, zone boots. None reach
the hash.

## New pins so it stays dead

- `tests/test_burst_stamp.cpp` proves *why* the contract matters:
  mass-applying a same-tick move-burst diverges from fair-share
  (measured hash inequality) — if anyone breaks "stamp at process time,
  one-per-session-per-tick", this test now stands in the way.
- ctest sets WORKING_DIRECTORY to the repo root so asset gates (T-063)
  resolve from any runner.
- `tools/t49_repro.sh` stays: the one-command flake detector for any
  future record-path work.
