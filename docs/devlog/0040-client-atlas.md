# 0040 — Client atlas batch: sheets resolve, party reads green, feet land (S28)

S28 of the extended queue: T-ART-05 + T-ART-07 + T-ART-10. All render-only:
no sim, no wire, no epoch impact (stays **11**). No journal touched.

## T-ART-05 — per-wireKind atlas table

`Game::atlasFor` (`client/src/game.cpp`): players (0), furniture (64+),
and out-of-range kinds → hero fallback; mob kinds index `kMobs` and load
`assets/aigen/mobs/<id>_<slug>/sheet.{png,json}` lazily with a cache. The
dir law (`mobSheetPaths`, inline in `engine/assets/atlas.h`) matches every
shipped B3/B4 row; **1011 Guard has no sheet and falls back** (stated, not
a bug). Furniture keeps the placeholder rect (NPC dirs hold drafts only —
sheets unshipped). The red-circle QA marker path is unchanged for frames
missing everywhere.

## T-ART-07 — party overhead tint

Priority: chaotic red > party green > lawful blue > neutral
(`resolveNameTint`, `engine/render/overhead.h`; own-cream short-circuits
first). Membership reads the roster the client already holds
(`net_->party` entityIds). **Enemy-town rank is a documented no-op**: no
war state exists and no wire carries it — there is deliberately no
enumerator for it.

## T-ART-10 — anchorY

`Anim.anchorY` (42.0 default) + `loadAtlas` reads the JSON key the
atlaspack sheets already emit + inline `animAnchorY` + both draw sites
(online entities per used anim, offline hero). Sepulcher Elite / Gravemother
cells now sit feet-on-diamond per their shipped 42s; other values flow when
art ships them.

## Tests

`tests/test_clientlaw.cpp`: sheet dirs for 1001/1003/1009/1010/1011 +
never-load pins (0/64/69/255), tint priority table (incl. red-wins-ties and
party-beats-lawful), anchor default/override pins. Loader/cache/draw wiring
needs textures + a display — stated headless-untestable, not faked.

## Files

`engine/{assets/atlas.h,assets/atlas.cpp,render/overhead.h}` (overhead new),
`client/src/{game.h,game.cpp}`, `tests/{test_clientlaw.cpp,CMakeLists.txt}`.
Suite **140 / 328,140**, ctest 2/2, warning-free.
