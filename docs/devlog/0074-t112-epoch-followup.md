# 0074 — T-111 epoch follow-up: the replay leg that could not run (T-112)

T-111's "no epoch bump" claim rested on a sandbox that could not build. This
session could: cmake 3.28 via pip, sqlite pre-seeded from the vendored
amalgamation, gcc 12.2 headless. The optional checkbox in PR #8's test plan
("replay `logs/t107.bwj` under epoch 18 → mismatches=0") failed 81/81 from
the first checkpoint (tick 25), entities 196 vs 169.

## Isolation

Probe build = T-111 with ONLY the `initialMobSpawns` hunk (F4) reverted:
the same leg replays perfectly clean — `ticks=2042 sessionCmds=1097
hashes=81 mismatches=0 entities=169`. So:

- **F4 is the sole breaker.** The old `--n`-on-blocked-roll wrap aborted
  whole spawners (blocked first roll → wrap → `n > 40` break → zero mobs
  from that spawner). F4 fills them: spawn counts change AND the boot RNG
  stream lengthens/shifts, so every downstream draw diverges. World hash
  diverges from tick 0. This is exactly the "changes sim semantics under
  old journals" class AGENTS.md pins to an epoch bump.
- **F1/F2/F3/F5 are replay-neutral** on this leg (probe kept them all).
  Devlog 0073's per-fix reasoning was right for those four and wrong only
  about F4 ("spatial is law, not cosmetics" holds; "no epoch" does not).

## What landed (T-112)

- `kJournalEpoch` 18 → 19 (history line extended).
- `tests/test_t112_spawn_epoch.cpp`: 3 pins for the new spawn law (F4a
  fully-blocked rect terminates + places nothing; F4b half-blocked fills
  maxAlive on walkable tiles only; F4c single-free-tile rect fills via
  bounded re-roll). F4 had no unit coverage in T-111.
- Fresh gate leg `logs/t112.bwj`: 20 campaign bots × 10 s, soak p99
  1.51 ms, replay `ticks=202 sessionCmds=313 hashes=3 mismatches=0`.

## Lineage note (director-facing)

- `task/T-104-review-fixes` (PR #9) also declares epoch 19 from a separate
  lineage with a partially overlapping fix set (its fix #5 ≈ T-111 F1,
  fix #6 ≈ F3). `git merge-tree` shows PR #8 × PR #9 content-conflicts in
  `server/src/world.cpp` and `tests/CMakeLists.txt`. Whichever merges
  second re-bumps to 20 and regenerates its gate leg.
- Evidence commands (this sandbox): see the T-112 PR description; builds
  use `cmake -S . -B build/server-only -DBH_BUILD_CLIENT=OFF` with the
  sqlite amalgamation pre-seeded into `build/server-only/_deps/`.
