# T-154 — Headless preset builds and runs the test suite (P1, audit 5/20)

## Context
Audit finding **A1 / A4 / P1-1**. `cmake --preset headless` (`BH_BUILD_CLIENT=OFF`)
builds the server, bots, mapconv and duel harnesses, but **`bh_tests` fails**:
`tests/test_zoom.cpp` includes `engine/render/camera_rig.h`, whose line 3 is
`#include <raylib.h>`. `tests/CMakeLists.txt` has no `BH_BUILD_CLIENT` guard and
links `raylib` unconditionally. Consequences: any agent session without X11/GL
dev headers (the common sandbox case — this audit had no raylib at all) has **zero
unit coverage**, the `headless` testPreset in `CMakePresets.json` cannot pass, and
AGENTS.md's DoD ("all tests pass") is unverifiable there. T-108/T-110 claim this
gate exists.

## Scope
- Guard the raylib-dependent test TUs on `BH_BUILD_CLIENT`: `test_zoom.cpp`
  (camera rig), and any other TU that pulls raylib headers (`test_animstate`,
  `test_clientlaw`, `test_refine_glow`, `test_decals` — check each by compiling).
- Drop the unconditional `target_link_libraries(bh_tests PRIVATE raylib)`; link it
  only when the client is enabled.
- Where a pin is pure math behind a raylib header (e.g. `snapZoom`), prefer
  **extracting the math into a raylib-free header** over skipping the test, so
  headless still covers it. Do not weaken a pin: if a test must be skipped, print
  a doctest skip message naming the reason.
- CI: add a `headless` configure+build+ctest job (no X11 packages) so the gate is
  actually gated.
- OUT: any sim/wire/schema change; any test deletion.

## Acceptance criteria
1. `cmake --preset headless && cmake --build --preset headless && ctest --preset
   headless` → **all green in a container with no X11/GL dev packages** (paste the
   log; this audit's sandbox is a valid reproduction environment).
2. `cmake --preset linux-gcc && ctest --preset linux-gcc` → still green, with the
   client-only pins present (suite count must not drop on the graphical build).
3. CI shows two jobs: existing graphical matrix + new headless leg.
4. No pin was deleted: `git diff tests/` shows guards/extractions only.

## Tests required
The suite itself, in both configurations.

## Evidence owed at merge
Both ctest outputs, CI run URL, devlog, board row, and an AGENTS.md/DoD note if
the wording needs a truth-up.
