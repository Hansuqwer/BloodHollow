# T-ART-B5 — procedural placeholder NPC sheets (verified 2026-09-17, code already in tree)

## Context

7 NPC slots (67..73) were spawnable but drawn as brown rect. Procedural sheets were generated 2026-09-12 via `b5_npc_proxy.py` (idle4, 32×48/64×48, ≤32 colours). Card file `T-ART-B5-procedural-npcs.md` shows DONE checklist but remained in `tasks/` (board says still open). This card verifies.

## Verification 2026-09-17 (branch `task/art-probe-and-b5-close`, stack on `task/T-R-LUMA-rimlight:238/27`)

- `assets/aigen/npcs/*/sheet.png` + `sheet.json` + `palette.png` + `portrait.png` (7×4) present, `sheet.json` v1 schema (dirs 8, anchorY 42, idle4), `loadAtlas` replica clean.
- `engine/assets/atlas.h:furnitureSheetPaths` + `client/src/game.cpp:atlasFor` furniture branch present (T-ART-B5), fallback to hero rect works.
- `tests/test_clientlaw.cpp` `T-ART-B5` 7 rows PASS (`furnitureSheetPaths` 67..73).
- `assets/LICENSES.md` procedural row present (MIT, no third-party pixels).
- `cmake --build --preset linux-gcc` + `bh_tests 299/299` + `ctest 2/2` + `validate 0/6` green; no epoch/wire (client-only).

## Close-out

No code change — code/assets already shipped. Card moved `tasks/T-ART-B5-procedural-npcs.md` → `done/T-ART-B5.md` (copy, original kept for history). No epoch/wire.

## Deviations

- None.
