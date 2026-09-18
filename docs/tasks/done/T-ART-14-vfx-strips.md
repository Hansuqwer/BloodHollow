# T-ART-14 — dirs:1 VFX strips (LOADER SHIPPED 2026-09-18; events → T-ART-14b)

**Status:** `done` — loader accepts dirs:1 and plays (fixture-proven).

## Context

~25 VFX strips spec'd (`docs/art/50-vfx.md`); 0 PNGs on disk; in-client VFX is callout text only because `loadAtlas` rejects `dirs:1` (`engine/assets/atlas.cpp:29`). Ruled B0.

## Scope

Accept `dirs:1` strips in `loadAtlas` (single-direction anims), wire strip playback to the shipped combat/skill events (rendering-only, server stays authoritative). Unblocks VFX art production.

## Acceptance

1. A `dirs:1` strip loads + plays against its event (screenshot series).
2. `ctest` green both presets; 00-VERIFY row → shipped.

## Out of scope

Generating the missing VFX art (art lane); any sim/wire change.

## Evidence (2026-09-18)

- Law: `engine/assets/atlas_dirs.h` (raylib-free: dirs 1|8 accepted, row 0
  replicated to all 8 facings, bounds shrink to 1 row) + pins in
  `tests/test_terrain_skin.cpp`. `loadAtlas` uses it (8-dir sheets byte-
  identical path).
- Playback path proven: procedural FIXTURE strip only
  (`assets/aigen/vfx/test/castring/`, 3-frame ring, clearly not shipped
  art) + `--vfx-test` dev visual (loops at hero) — shot
  `tart14_vfx_ring.png` shows load+play end to end.
- **Owed → T-ART-14b:** the ~25 real strips (art lane, needs pipeline key)
  + wiring playback to combat/skill events (render-only hook point:
  `drawVfxTest` shows where event pulses will feed a strip player).
