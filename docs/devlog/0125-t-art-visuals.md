# 0125 — Art-engine visuals: T-ART-12/13/14 + T-164 matrix (Xvfb frigor over)

Graphical env was available after all (Xvfb + Mesa llvmpipe @1024x768), so
the four "deferred" cards got executed instead of deferred. All renderer-only:
no sim, no wire, no epoch (suite + replay untouched by construction).

## T-ART-12 textured terrain — DONE

Law `engine/render/terrain_skin.h` (raylib-free: plate-cut coords, D12
face/point pieces, WALL skirt rule, deterministic variant hash) + 6 pins.
Client `client/src/terrain_skin.{h,cpp}`: per-zone bake at first draw
(plates cut by world coords, D12 edge/skirt overlays, one static texture) +
per-frame skinned prisms (mine/crypt v0..v2 anti-strobe sets). The flat pass
stays as underlay/fallback — missing art degrades per tile, never holes
(castle has no terrain.json → flat, stated). Bakes: town 2813/828, fields
2664/990, mine 583/273, thornwall-crypt 745/342, drowned-crypt own manifest.
Shots `docs/research-notes/qa/tart12/` (Thornwall day/night @ {1,1.5,2} +
flat before pair; fields/mine/crypt day). fps 20–58 llvmpipe-bound (bakes
included); hardware 20-bot framing owed as a one-liner — no renderer change
needed for it. Manifests flipped to `engine_validated: true`.
Lessons: (1) this raylib build draws shapes as RL_QUADS — RL_TRIANGLES
immediates streak across the screen (faces go out as textured quads, same
winding as `iso::drawPrism`); (2) `TakeScreenshot` strips directories
(saves basename to CWD); (3) offline `--map` kept mapId 1 — fixed with
`mapIdForFile` (fields/mine first shot with town art, retaken).

## T-ART-13 bitmap font + R-TEXT-2 — DONE

`bhfont.py` exported cap7/cap11 to `assets/aigen/ui/font/` (offline, no key).
`client/src/bitmap_font.{h,cpp}` (white strip + per-draw tint + 1px outline;
missing files fall back to DrawText). Floaters ride cap11, nameplates cap7.
R-TEXT-2 law in `engine/render/overhead.h` (pile limit 3, tag-rect overlap)
+ pins in `test_clientlaw.cpp`; piles of 4+ degrade to the karma badge
(own name exempt; furniture included — the town-square NPC cluster is the
worst piler; first anchor-only attempt missed wide-text overlap, fixed with
tag rects). Shots `tart13_font_pile2/3.png` (online, ~70 ents): bitmap names
+ badges in the pile. HUD/body stays default font (63-chrome rule).

## T-ART-14 dirs:1 strips — LOADER DONE, events → T-ART-14b

Law `engine/assets/atlas_dirs.h` (1|8 accepted, row 0 replicated, bounds
shrink) + pins; `loadAtlas` uses it (8-dir path byte-identical). Proven by a
procedural FIXTURE strip (`assets/aigen/vfx/test/castring/`, not shipped
art) + `--vfx-test` dev visual looping at the hero (`tart14_vfx_ring.png`).
Real ~25 strips + CombatPulse wiring filed as T-ART-14b (needs art key).

## T-ART-15 icons + hotbar — STRUCTURE DONE, art owed (stays open)

Hotbar (bottom-center, online): follows the live input page (1-6 / Shift+1-8
/ Ctrl+1-5), lock state from `kits.h` unlocks at hero kit+level, §11 family
plates, req level when locked (`tart15_hotbar.png`: Cultist L1, Resurrect
dimmed L20). Bag rows: rarity icon plate ahead of text (placeholder rect;
marker + name colour already live via T-159f1.3). Owed: 60-icon art set (no
pipeline key in sandbox), cooldown sweep (no wire), vendor plates, 2x icons.

## T-164 night-light law — DONE

Matrix `docs/research-notes/qa/t164/` (10 shots: 00/02/04h × lamp 0/6/8/10
@ z1 + 00h × 0/6 @ z2). Torch-6 reads as designed; lamp-0 navigable but not
grindable. D6b was already shipped (T-071) and is now ONE shared helper
(`drawLightPool`: online `snap.light` loop + offline `--lamp`, same call).
GDD §9 reconciled (Vigil +2, pools-after-overlay, evidence pointer). No
sim/wire/epoch. Dev captures: `--hour/--zoom/--lamp/--flat-ground/
--flat-font/--create/--vfx-test` (all render-only).

## Suite at close

`ctest` both presets + `t159f1.bwj` replay in the verify pass (see board).
M1/M2 full evidence stands closed per 0124 (this pass added only a bounded
60 s probe — redundant, same verdict).
