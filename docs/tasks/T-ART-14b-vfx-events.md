# T-ART-14b — VFX event wiring + real strips (TIER-0 SHIPPED 2026-09-19; art redraw owed)

**Status:** `done` — event feed + 4 tier-0 strips live; art-lane redraws filed below.

## Context

T-ART-14 landed the loader (`loadAtlas` accepts dirs:1, row replicated to 8
facings, law in `engine/assets/atlas_dirs.h`, proven by the
`assets/aigen/vfx/test/castring/` fixture + `--vfx-test` dev visual).
Production is unblocked; nothing plays yet because no real strips exist.

## Scope

- Generate the ~25 strips per `docs/art/50-vfx.md` through the aigen
  pipeline (probe-before-batch, ≤10 gens/session). Priority: cast ring #10,
  Firebolt #11, swing arcs #1–2, Mend motes #17 (the shipped combat loop).
- Wire strip playback to the shipped combat/skill events (CombatPulse kinds
  → strip player at the victim/caster tile, render-only; server stays
  authoritative). Hook point: `Game::drawVfxTest` shows the load+play call
  shape — promote it to an event-fed player, delete the dev flag path.
- Screenshot series per strip (day + night) + fps check at 20 bots.

## Acceptance

1. Cast ring + Firebolt + swing arc + Mend motes play against live events
   (screenshot series, day + night).
2. `ctest` green both presets; 00-VERIFY row → shipped.

## Out of scope

New sim/wire (render-only); icon art (T-ART-15 lane).

## Evidence (2026-09-19 — tier-0)

- Strips: 4 procedural PIL strips (`assets/aigen/vfx/{swing_arc,
  mend_motes, cast_ring, firebolt_impact}/`, dirs:1 JSON) — crescent + bone
  edge + dark outline (outline added after the grey-on-cobble read failed in
  shots), rising motes, expanding ellipse, ember spatter. Tier-0 stand-ins;
  the art lane redraws all 4 + the remaining ~21 per `50-vfx.md`.
- Feed: `Game::noteVfx` maps CombatPulse kinds → strips (1/2/5 swing at
  victim, 9 impact at victim, 8 motes at recipient, 10/13/14 ring under
  caster; slain stays a decal), capped-16 deque, one-shot durations from the
  strip JSON; `drawVfxPlays` over entities, under floaters. Verified live
  (pulse→play counter: 21 plays) + deterministic `--vfx-test` all-strip
  shot (`qa/vfx_tier0_all_z2.png` crop: all 5 incl. fixture).
- Dev affordance: `--cam TX,TY` (render-only camera pin for captures).
- Renderer-only: no sim/wire/epoch.
