# 0052 — Refine glows from +5, in the inventory first (T-ART-11)

T-ART-11, render-only. Unblocked by T-079 (the +5 rows now exist to glow).

## What landed

- `engine/render/refine_glow.h` (new, raylib-free): `refineGlowTier` + `refineGlowAlpha` (70, under the 90 cap). Same pattern as the S28 client laws (`overhead.h`, `lightmask.h`) — pin the math headless, state the draw.
- `client/src/game.cpp` inventory rows: glow wash + warm text + `+N*` (mythic `+N**`). Dormant still wins. One `static_cast<unsigned char>` to satisfy `-Werror=narrowing` on the `Color` aggregate — the only build surprise.
- `tests/test_refine_glow.cpp` (new): 2 cases, boundary pins. Suite 160 → 162.
- Comment fix: `InvSlotWire::refine` 0..3 → 0..7.

## Judgment calls

- Inventory-first, not sprite overlays: the backlog's "~2 overlay frames" need per-entity refine on the wire, which no snapshot carries. Shipping inventory glow now (the auditor's view) and scoping the wire field as a future card respects the non-wire contract instead of sneaking a protocol bump into an art card.
- Mythic branch despite the +7 ceiling: pinned and unreachable on purpose — when a future card raises the ceiling, the silhouette read already exists and the test already guards it.
- No soak journal (render-only): the two replay lines are the proof, plus the honest headless note for screenshots.
