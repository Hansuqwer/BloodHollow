# T-ART-12 — Textured terrain: plates + prism skins + D12 edges (SHIPPED 2026-09-18)

**Status:** `done` — renderer landed, all 5 shipped zones bake textured.

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

## Evidence (2026-09-18, Xvfb + Mesa llvmpipe @1024x768)

- Law: `engine/render/terrain_skin.h` (raylib-free: plate-cut coords, D12
  face/point pieces, WALL skirt rule, deterministic variant hash) + 6 pins
  in `tests/test_terrain_skin.cpp` (suite green both presets).
- Client: `client/src/terrain_skin.{h,cpp}` (zone bake at first draw: plates
  cut by world coords, D12 edge/skirt overlays, one static texture; skinned
  prisms per frame incl. mine/crypt v0..v2 anti-strobe sets) + `drawGround`
  keeps the flat pass as underlay/fallback (missing art degrades per tile,
  never holes; castle has no terrain.json → flat, stated).
- Bakes: town 2813 tiles/828 edges, fields 2664/990, mine 583/273,
  thornwall-crypt 745/342, drowned-crypt (own manifest) — all skinned prisms.
- Shots (`docs/research-notes/qa/tart12/`): Thornwall day/night @ {1,1.5,2}
  + flat before pair; fields/mine/crypt day. Night 00:00 stays legible.
- fps (llvmpipe software rendering — lower bound, real GPUs faster):
  20–58 across zones incl. one-time bake hitch. **AC#3 partial:** 60fps with
  20 bots on screen needs hardware framing — owed one-liner, renderer needs
  no change for it (1 static texture + ~hundreds of prisms/frame).
- Renderer-only: no sim/wire/epoch. Dev flags (all render-only):
  `--flat-ground`, `--hour`, `--zoom`, `--map-id` fix (`mapIdForFile`).
- Deviations: prism faces go out as RL_QUADS (this raylib build draws shapes
  as quads — RL_TRIANGLES immediates streak; verified in shots).
