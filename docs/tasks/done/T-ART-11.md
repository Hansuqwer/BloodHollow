# T-ART-11 — Refine +5 glow composite (render-only, non-wire)

## Context
GDD §7 "item glows from +5"; T-060/T-079 ship refine to +7 with no glow render (art-backlog T-ART-11, parked until S32 — now unblocked). Backlog pins: additive overlay alpha ≤ 90; +10 reads as silhouette change, not brighter glow.

## Scope
- Headless glow law `engine/render/refine_glow.h`: tiers (None <5, Glow 5–9, Mythic 10+), overlay alpha 0/70/70 (cap 90 holds). Mythic pinned but unreachable at the +7 ceiling — future-proofing for a later raise.
- Inventory rows (incl. equipped = in-hand): glow wash (alpha 70) + warm row color + `+N*` marker; mythic `+N**` + paler row. Dormant (durability 0) wins over glow (a worn-out +5 does not glow). Aura/equipped order otherwise unchanged.
- Stale `InvSlotWire::refine` comment 0..3 → 0..7 (T-079).
- Deliberately OUT: in-world sprite overlays — no entity snapshot carries refine, and adding one is a wire change (protocol bump). Non-wire card by backlog contract; a future card can carry gear on the wire.

## Acceptance criteria
- [x] +5 blade glows in inventory + equipped rows day/night at 1×/2× (draw-path, headless-stated).
- [x] +10 branch pinned as silhouette (same alpha, new marker) though unreachable.
- [x] Replay bit-exact (render-only); suite green.

## Tests required
- `tests/test_refine_glow.cpp`: tier boundaries (4/5/7/9/10/12), alpha pins + cap check. Suite 160→**162**, 328,502 assertions.

## Out of scope
- Wire/epoch/sim changes (none); in-world overlay sprites (~2 frames per backlog — needs the wire field first).

## Evidence
- Suite: **162/162 (328,502 assertions)**, ctest 2/2, warning-free (`-Werror`, narrowing fix with explicit `static_cast<unsigned char>`).
- Replay: `logs/t082.bwj` OK 12801/7391/129 mm=0; `logs/t084.bwj` OK 6001/718/241 mm=0 (render-only proof).
- Headless screenshot block (no display) per brief §verification-5: draw wiring stated, not faked.
- Devlog: `docs/devlog/0052-refine-glow.md`.
