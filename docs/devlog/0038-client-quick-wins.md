# 0038 — Client quick wins: point filter, zoom snap, maps 4/5 (S26)

S26 of the extended queue: three T-ART micro-cards, one devlog. All
render-only: no sim, no wire, no epoch impact (stays **11**).

## T-ART-01 — point filtering in `loadAtlas`

`engine/assets/atlas.cpp` loaded real sheets without a filter while the
procedural placeholder sets `TEXTURE_FILTER_POINT` (`placeholder.cpp:57`).
One line after `LoadTexture`. No headless unit possible (`LoadTexture`
needs a display) and no atlas unit file exists in `tests/` (the backlog's
"existing atlas unit tests" line is stale — flagged, not invented):
acceptance is build-green + the call mirrors the placeholder exactly.

## T-ART-02 — snap wheel zoom to {1, 1.5, 2}

Pure `snapZoom` inline in `engine/render/camera_rig.h` (<1.25→1, <1.75→1.5,
else 2); wheel block calls it, `Z` toggle untouched (already 1↔2). Old
2.5 ceiling now snaps to 2 — intended (backlog: reachable ∈ {1,1.5,2}).
`tests/test_zoom.cpp` pins the boundaries; `bh_tests` links raylib PRIVATE
for the header (test TU never opens a display).

## T-ART-08 — map cases 4/5

Two lines in `Game::mapFileFor` (`client/src/game.cpp:830`):
4 → `bonehowl_mine.bhmap`, 5 → `drowned_crypt.bhmap` (filenames match the
server boot lines). Zone handoff itself is untouched (T-037); no display
exists here, so the 1→4→5→1 walk is recorded as untested-render, pin-only.

## Ledger flips (no code)

- **T-ART-03 → done-superseded by T-071.** The additive mask and warm pools
  shipped in devlog 0034 (`DrawCircleGradient` per lit entity, peak 90 <
  floor 150). The backlog's "engine has none" premise is stale.
- **T-ART-06 → partial.** Kinds 68 (confessor, T-070) + 69 (fence, T-069)
  ship and render through the generic furniture rect + name label. Kinds
  67/70/71/72/73 remain — future card, not this shift.
- **T-ART-11 stays parked** — it needs refine +5, which does not exist until
  S32 (T-079) lands. **T-ART-09 stays parked** — needs the decal layer.

## Files

`engine/assets/atlas.cpp`, `engine/render/camera_rig.h`,
`client/src/game.cpp`, `tests/{test_zoom.cpp,CMakeLists.txt}`. Suite
**135 / 328,102**, ctest 2/2, warning-free.
