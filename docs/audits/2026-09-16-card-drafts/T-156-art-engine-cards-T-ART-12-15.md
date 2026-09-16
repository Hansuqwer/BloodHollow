# T-156 — File and run the four ruled-but-unfiled art engine cards (P0, audit 7/20)

## Context
Audit findings **E1.4 / E1.6 / E1.7 / E1.8 / E5 / P0-6**. The art lane has
produced more than the renderer can show:

| lane | on disk | on screen | gate |
|---|---|---|---|
| terrain plates + D12 edges + prism skins | **709 PNGs** + `terrain.json` manifests | flat `terrainColor()` diamonds + untextured prisms (`client/src/game.cpp:728,749`) | **T-ART-12** |
| callout/name bitmap font (D4, 7×11) | designed; `tools/atlaspack/bhfont.py` | default raylib font (no `LoadFont`/`DrawTextEx` anywhere) | **T-ART-13** |
| VFX strips (~25 spec'd) | **0 PNGs** | callout text only; `loadAtlas` rejects `dirs:1` (`engine/assets/atlas.cpp:29`) | **T-ART-14** |
| item/skill/UI icons (~60 spec'd) | **0 PNGs** | text-only panels | **T-ART-15** |

All four were **ruled** at the B0 gate (`docs/art/B0-GATE-DECISION.md` D6, D8 and
"Part 4 — Engine cards for the sibling to open") and never written to
`docs/tasks/`. `ls docs/tasks/done | grep ART` shows only T-ART-06/09/11. Design
pillar 1 ("era-authentic feel … not retro-styled 2024") is unmet until at least
T-ART-12 lands.

## Scope
- **File the four cards** in `docs/tasks/` from the B0 rulings + `docs/art/`
  specs (10-terrain, 50-vfx, 40-items-icons, 63-ui-chrome-options), each with
  executable acceptance criteria and a screenshot requirement.
- **Implement in this order**: T-ART-12 (biggest visual delta: textured diamond
  draw with plate cut by world coords, prism skins, D12 adjacency edge lookup from
  `terrain/<zone>/terrain.json`) → T-ART-15 (icons/hotbar) → T-ART-13 (bitmap
  font) → T-ART-14 (`dirs:1` strips, unblocking VFX production).
- Renderer-only: no sim, no authority, no wire, no epoch. Art lookups key on
  **zone/map id and content id**, never on slug (REGISTRY contract).
- Ship the R-TEXT-2 rule while in there (crowd-test approved: >3 overlapping
  overhead name tags degrade to the karma-badge glyph).
- OUT: generating the missing VFX/icon art (art lane), player sheets (T-142 stack).

## Acceptance criteria
1. Four cards filed; each implemented card lands with a before/after screenshot of
   Thornwall square at zoom {1, 1.5, 2}, day and night.
2. Thornwall + Fields render textured ground with correct edge bleeding
   (organic-over-built; WALL never bleeds, D12) and skinned prisms; mine + both
   crypts use their own manifests.
3. `ctest` green; renderer-law pins added where the math is raylib-free (follow the
   `snapZoom`/`animstate` pattern so the headless build keeps coverage).
4. No frame-rate regression: 60 fps at 1024×768 with 20 bots on screen (paste the
   `GetFPS()` evidence or a capture).
5. `assets/aigen/REGISTRY.md` + `docs/art/00-VERIFY.md` rows updated to "shipped".

## Tests required
Renderer-law unit pins; screenshot sets; fps check; existing suite green.

## Evidence owed at merge
Screenshots per zone, fps evidence, the four filed cards, devlog, board rows.
