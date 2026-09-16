# T-154 + T-168 — Headless gate + one build-dir law (SHIPPED 2026-09-16)

Combined: the two cards overlap completely (both about building without X11 and unifying `build/` vs `build/linux-gcc`). Single stacked branch.

## T-154 state found

Core complaint already fixed in tree: `tests/CMakeLists.txt` guards `test_zoom/test_animstate/test_clientlaw` on `TARGET raylib` and links raylib conditionally; `BH_BUILD_CLIENT=OFF` (headless preset) drops the raylib FetchContent. Verified: `cmake --preset headless && ctest --preset headless` → 314/314 green with no X11 packages installed beyond the sandbox baseline. What was missing: CI never exercised it (single `build/` configure, X11 apt always installed) and no job replayed a committed leg.

## T-168 state found

Three laws coexisted: `tools/bootstrap.sh` → `build/` (Release, sandbox-restore script), README + presets → `build/linux-gcc`, legs hardcoded `BUILD_DIR=build/linux-gcc`. `bootstrap.sh` is a sandbox-restore script (pip cmake + /tmp/x11prefix), NOT the documented law — left as-is, noted here.

## Scope shipped

- 9 leg scripts: `BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}` (`t126`, `t127`, `t128`, `t135`, `t137_m4`, `t138`, `t139`, `t140`, `t146`). `bash -n` clean ×10 incl. new script. Default behaviour unchanged.
- 4 generators gain `--out` (default = committed path): bonehowl_mine, drowned_crypt, fields_overflow, thornwall_crypt. `--out /tmp` output diffs byte-identical vs committed `.tmj` ×4; `git status data/` clean on default run.
- CI (`.github/workflows/ci.yml`): mapgen determinism 1/6 → 6/6; new `headless` job (preset configure + build + ctest, no X11 apt) + replay of `logs/t159.bwj` (epoch guard — a future bump fails loudly instead of orphaning the leg).
- `tools/clean_clone_check.sh`: clone HEAD → headless build → ctest → boot + 5 wander bots → replay t159 → PASS/FAIL. Ran: **PASS** (replay ticks=641 cmds=417 hashes=6 mm=0).
- Caught en route: unquoted `:` in the new CI step name broke YAML parsing (verified with `yaml.safe_load` before/after).

## Tests / evidence

- `ctest --preset headless` 2/2 green (314/314 doctest) on this branch (no test changes — infra only).
- Generator diffs: BONE-OK DROWN-OK FIELDS-OK CRYPT-OK (+ thornwall/castle already had --out; all 6 in CI).
- `bash tools/clean_clone_check.sh /tmp/bh_cc_test 7859` → PASS (note: clone carries committed HEAD incl. T-159; uncommitted infra files ride the PR, not the clone).
- Override smoke: `BH_BUILD_DIR=build/headless bash -c '...'` → `build/headless`. Full leg re-run under override NOT done (would overwrite committed `logs/*.bwj` — T-157 owns leg refresh).

## Out of scope (kept)

- `bootstrap.sh` repair (PEP 668 pip flag, apt-mirror fallback) — sandbox-owned, separate card if the director wants it.
- `shared/protocol/gen/` stale duplicate — T-165 owns it.
- `linux-gcc` graphical build + macOS arm64 — CI matrix unchanged.

## Follow-ups

- T-157 (evidence refresh) consumes the 6/6 CI gate on its first run.
