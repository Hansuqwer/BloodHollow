# T-168 — One build-dir law, working bootstrap, clean-clone script (P1, audit 19/20)

## Context
Audit findings **A2 / A6 / A16 / A17 / A18 / H9**, and director card **T-149**
(clean-machine boot, already filed). Three build-dir laws coexist:
`tools/bootstrap.sh` configures **`build/`** (Release), `README.md:95-105`
documents **`build/linux-gcc`** presets, and every `tools/*_leg.sh` hard-codes
`BUILD_DIR=build/linux-gcc`. `bootstrap.sh` also fails as written on a current
Debian-bookworm sandbox: `apt-get download libxrandr-dev … libwayland-dev
wayland-protocols pkg-config` → *Unable to locate package*, and `pip install cmake`
needs `--break-system-packages` (PEP 668). `assets/maps/*.bhmap` only exist after a
build runs the `bh_maps` target. Separately, four of the six map generators
(`make_fields_overflow`, `make_thornwall_crypt`, `make_bonehowl_mine`,
`make_drowned_crypt`) **ignore `--out` and write straight into `data/maps-src/`**,
so CI's determinism diff pattern only extends to 2 of 6 maps and running a
generator mutates the working tree.

## Scope
- Pick **one** law (recommendation: presets — `build/linux-gcc` for local runs,
  `build/headless` for agent/CI-headless) and make README, `bootstrap.sh` and all
  `tools/*_leg.sh` agree; leg scripts take `BH_BUILD_DIR` with a default.
- Repair `bootstrap.sh`: detect a usable package path (apt *or* the pip/prefix
  fallback), pass `--break-system-packages` when PEP 668 applies, verify
  `cmake`/`ninja` afterwards, and print the resulting build dir.
- Add `--out` to the four generators (default unchanged) and extend the CI
  mapgen-determinism step to **all six** maps + `bh_mapconv --validate` for all six.
- Add `tools/clean_clone_check.sh`: clone HEAD to a temp dir, build headless,
  build maps, boot the server, run 5 bots, replay the committed leg, print a
  PASS/FAIL line — the script T-149's human run can lean on.
- OUT: CI platform matrix changes beyond the above, LFS/asset-split decisions
  (H6), the launcher (T-146).

## Acceptance criteria
1. From a **fresh clone** in a clean container: `bash tools/bootstrap.sh` →
   working build dir; `bash tools/clean_clone_check.sh` → PASS (paste both logs).
2. `grep -rn "build/linux-gcc\|build/" README.md tools/*.sh tools/bootstrap.sh`
   shows one law + one env override.
3. CI: mapgen diff + mapconv validate for **6/6** maps, green.
4. Regenerating all six maps leaves `git status` clean (generators no longer write
   in place when `--out` is given).
5. `tools/t137_m4_leg.sh` and one other leg run unchanged in behaviour with
   `BH_BUILD_DIR` set to the headless dir.

## Tests required
CI extension is the test; plus the clean-clone script output.

## Evidence owed at merge
Bootstrap + clean-clone logs, CI URL, doc/script diffs, devlog, board row.
