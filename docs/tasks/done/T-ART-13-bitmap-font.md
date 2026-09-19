# T-ART-13 — Callout bitmap font (SHIPPED 2026-09-18)

**Status:** `done` — bitmap path live for floaters + nameplates, R-TEXT-2 live.

## Context

D4 7×11 bitmap font designed; generator `tools/atlaspack/bhfont.py` exists. Client uses the default raylib font (no `LoadFont`/`DrawTextEx` anywhere). Spec: `docs/art/63-ui-chrome-options.md`; ruled B0 D8.

## Scope

Load the bitmap font, route callouts/nameplates/combat text through `DrawTextEx`. Include the R-TEXT-2 rule while here (crowd-test approved: >3 overlapping overhead name tags degrade to the karma-badge glyph). Renderer-only.

## Acceptance

1. Before/after screenshots (Thornwall square day/night, mixed pile showing R-TEXT-2 degrade).
2. `ctest` green both presets; 00-VERIFY row → shipped.

## Out of scope

Font redesign (art lane owns glyphs).

## Evidence (2026-09-18)

- Art (offline, no key needed): `bhfont.py` exported to
  `assets/aigen/ui/font/` (cap7 + cap11 strips + JSON, D4's 7x11 for
  callouts, 5x7 small caps for names).
- Client: `client/src/bitmap_font.{h,cpp}` (white-glyph strip + per-draw
  tint + 1px outline; missing files fall back to DrawText, never holes).
  Floaters ride cap11, nameplates cap7 (uppercased red-caps read).
- R-TEXT-2: law in `engine/render/overhead.h` (`kNamePileLimit=3`, tag-rect
  overlap) + pins in `tests/test_clientlaw.cpp`; client degrades piles of
  4+ to the karma-badge diamond (own name exempt; furniture included —
  the town-square NPC cluster is the worst piler).
- Shots: `tart15_vendor_icons.png` + `vfx_arc2.png` (online, 60+ ents) —
  bitmap names + karma-badge diamonds in the town-square pile (the earlier
  `tart13_font_pile2/3.png` were superseded and removed 2026-09-19).
  HUD/body text stays default font (63-chrome rule;
  re-skinning all panels is a follow-up, not this card).
- Renderer-only: no sim/wire/epoch. Dev affordance: `--create 1M`
  (unattended T-167 answer for captures), `--flat-font` (before shots).
