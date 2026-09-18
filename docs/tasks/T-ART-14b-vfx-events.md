# T-ART-14b — VFX event wiring + real strips (follow-up to T-ART-14 loader)

**Status:** `open` — needs the art pipeline (no key in sandbox) + event hookup.

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
