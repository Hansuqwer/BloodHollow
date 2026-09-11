# AGENTS.md — Rules for AI coding agents in this repository

You are an engineering agent on **BLOODHOLLOW**, a 2D MMORPG (C++20 client on raylib,
headless C++20 server, ENet transport). The human is the director; you are staff.
These rules are standing orders. Task-specific orders live in `docs/tasks/T-*.md`.

## Ground truth, in priority order

1. The task card you were given (`docs/tasks/T-*.md`).
2. Design intent: `docs/02-gdd.md`.
3. Architecture decisions: `docs/03-architecture.md` and ADRs in `docs/adr/`.
4. This file.
5. Existing code.

If the task conflicts with the GDD or an ADR, **stop and flag it** — do not silently
redesign.

## Hard rules

- **C++20, no exceptions across module boundaries, no RTTI.** Warnings-as-errors:
  `-Wall -Wextra -Werror`. Formatting: `.clang-format` exists at the root, but the
  practiced convention (per session handoffs) is **hand-format the lines you
  touch** — do NOT run whole-file `clang-format` on files you did not rewrite
  (it buries reviewable diffs in churn).
- **No new third-party dependency without an ADR** approved by the human. Current
  approved deps: raylib, ENet, sqlite3, nlohmann/json, doctest, miniaudio (via
  raylib), stb_image/stb_truetype.
- **The server never trusts the client.** All combat, movement speed, range, cooldown,
  inventory and economy logic is validated server-side. Client code may predict but
  never decide.
- **Determinism:** all gameplay RNG flows through `sim/rng.h` (xoshiro256**, seeded).
  Server logs its world seed per session; the replay harness
  (`bh_server --record-world J` / `bh_server --replay-world J` — there is no
  separate `tools/replay` binary) must be able to re-execute a session
  deterministically. Journals are epoch-stamped (`kJournalEpoch` in
  `server/src/main.cpp`): any change to sim semantics under an old journal bumps
  the epoch and ships a fresh gate leg. Do not call `rand()`,
  `std::random_device`, or time-based seeds in gameplay code.
- **Fixed timestep sim at 20 Hz** (`SIM_TICK_HZ=20`). Never use frame delta-time in
  `shared/sim` or `server/` logic; use integer ticks.
- **Units:** world/logic integers in *ticks* and *tiles*; floats allowed in
  client-only render code.
- **Networking:** all protocol messages defined in `shared/protocol/` with an explicit
  version byte; bump `PROTOCOL_VERSION` on any wire-format change.

## Workflow

1. Read the task card + relevant docs. Restate the acceptance criteria in your plan.
2. Branch `task/T-xxx-short-slug` from `master` (the default branch is `master`,
   not `main`). Historical practice through T-103 committed straight to `master`
   under the director's review — 0 branches/PRs/merges existed; the branch+PR
   flow below is the standard for contributed cards from T-104 onward.
3. Write/extend tests **before or with** implementation:
   - unit tests (doctest) in `tests/` for sim, items, combat math, protocol codecs;
   - a soak-bot scenario via `tools/bots` + the `tools/*_leg.sh` harnesses when
     touching netcode or the sim loop.
   - *Aspiration, not practice (T-110 truth-up):* an earlier revision of this
     file mandated "property-style fuzz for combat/enhancement math (1M
     iterations, fixed seeds)" — no such fuzz exists in the tree (largest loop:
     100k RNG draws). It remains a recommended card; do not cite it as coverage
     that exists.
4. Keep diffs reviewable: < ~400 lines changed per PR unless the task card says
   otherwise. Split large tasks; open a follow-up card.
5. Update docs with the code: task card status, GDD numbers you changed, ADR if you
   made a judgment call beyond the spec.
6. PR description: what/why, test evidence (paste test+soak output), screenshots/GIF
   if visual, and a list of deviations from the task card.

## Definition of done (any task)

- Builds warning-free on Linux (gcc/clang) **and** macOS (clang, arm64).
- All tests pass; new behavior is covered.
- No *new* allocations in the hot loop beyond pre-warmed pools. (Known debt,
  T-110 truth-up: the shipped `tickServer`/`distributeEvents`/`queryAoi` paths
  do allocate per tick, and entities live in an AoS `std::deque` — the
  architecture doc's zero-alloc SoA `EntityStore` is a deferred card, not
  current law. Don't make the churn worse.)
- Journal round-trip: `bh_server --record-world` a bot session, then
  `--replay-world` it → `mismatches=0`; when sim semantics changed under old
  journals, bump `kJournalEpoch` and commit a fresh gate leg (see `logs/`).
- Docs updated; task card moved to `docs/tasks/done/`.

## What you must NOT do

- Add Windows-specific code (Linux/macOS only).
- Introduce an ECS/game-engine framework (the engine is intentionally hand-rolled).
- Touch `assets/final/` (human-art directory) — placeholder/AI-gen paths only.
- Commit secrets, server hostnames with credentials, or generated binaries.
- Merge your own PRs. The human reviews everything. (Truth-up: through T-103
  all work landed as direct commits to `master` under the director's review —
  the rule binds PR-based work, which is the standard from T-104 onward.)
