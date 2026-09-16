# T-ART-12 — Textured terrain: plates + prism skins + D12 edges (filed 2026-09-16, T-156)

**Status:** `open` — renderer work, needs a graphical env for evidence (screenshots + fps). Headless implementers: keep math raylib-free where possible (snapZoom/animstate pattern) so `ctest --preset headless` keeps coverage.

## Context

On disk: 709 terrain PNGs + `terrain.json` manifests. On screen: flat `terrainColor()` diamonds + untextured prisms (`client/src/game.cpp:728,749`). Spec: `docs/art/10-terrain.md`; ruled B0 (`docs/art/B0-GATE-DECISION.md` D6).

## Scope

Textured diamond draw with plate cut by world coords, prism skins, D12 adjacency edge lookup from `terrain/<zone>/terrain.json` (organic-over-built; WALL never bleeds). Renderer-only: no sim, no authority, no wire, no epoch. Lookups key on zone/map id + content id, never slug (REGISTRY contract).

## Acceptance

1. Thornwall + Fields render textured ground with correct edge bleeding + skinned prisms; mine + both crypts use their own manifests.
2. Before/after screenshots, Thornwall square @ zoom {1, 1.5, 2}, day + night.
3. 60 fps at 1024×768 with 20 bots on screen (paste `GetFPS()` evidence).
4. `ctest` green both presets; `assets/aigen/REGISTRY.md` + `docs/art/00-VERIFY.md` rows → shipped.

## Out of scope

VFX/icon art generation (art lane); player sheets (T-142 stack).
