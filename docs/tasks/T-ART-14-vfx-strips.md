# T-ART-14 — dirs:1 VFX strips (filed 2026-09-16, T-156)

**Status:** `open` — renderer work, needs a graphical env for evidence. See T-ART-12 header note.

## Context

~25 VFX strips spec'd (`docs/art/50-vfx.md`); 0 PNGs on disk; in-client VFX is callout text only because `loadAtlas` rejects `dirs:1` (`engine/assets/atlas.cpp:29`). Ruled B0.

## Scope

Accept `dirs:1` strips in `loadAtlas` (single-direction anims), wire strip playback to the shipped combat/skill events (rendering-only, server stays authoritative). Unblocks VFX art production.

## Acceptance

1. A `dirs:1` strip loads + plays against its event (screenshot series).
2. `ctest` green both presets; 00-VERIFY row → shipped.

## Out of scope

Generating the missing VFX art (art lane); any sim/wire change.
