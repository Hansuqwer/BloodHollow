# T-ART-13 — Callout bitmap font (filed 2026-09-16, T-156)

**Status:** `open` — renderer work, needs a graphical env for evidence. See T-ART-12 header note.

## Context

D4 7×11 bitmap font designed; generator `tools/atlaspack/bhfont.py` exists. Client uses the default raylib font (no `LoadFont`/`DrawTextEx` anywhere). Spec: `docs/art/63-ui-chrome-options.md`; ruled B0 D8.

## Scope

Load the bitmap font, route callouts/nameplates/combat text through `DrawTextEx`. Include the R-TEXT-2 rule while here (crowd-test approved: >3 overlapping overhead name tags degrade to the karma-badge glyph). Renderer-only.

## Acceptance

1. Before/after screenshots (Thornwall square day/night, mixed pile showing R-TEXT-2 degrade).
2. `ctest` green both presets; 00-VERIFY row → shipped.

## Out of scope

Font redesign (art lane owns glyphs).
