# T-ART-09 — Ground decal layer (render-only, non-wire)

## Context
Blood decals (10 min), boss telegraphs (Gravemother 1009), Sanctuary/Wither circles all need the `gore` row from `docs/03-architecture.md`: a render-to-texture decal surface per map chunk. Art-backlog T-ART-09, parked until the layer existed — this card builds it client-side.

## Scope
- Headless law `engine/render/decals.h`: kinds (blood / telegraph 1-2-3 / circle), TTLs (blood 12000t = 10 min; telegraph 20/20/40; circle persistent), linear-fade alphas, FIFO cap 512.
- Client surface (`client/src/game.{h,cpp}`): `decals_` deque, `noteDecals()` harvests kill pulses (kind 3, victim pos from `rents_`, furniture skipped, already-despawned skipped-stated), `drawDecals()` y-sorted under entities (called inside `drawGround()` before `drawEntitiesOnline()`), `addTelegraph(x,y,stage)` + `addCircle(x,y)` boss APIs for future Gravemother/Sanctuary code, zone-reload clears (per-zone surface, same as `rents_`/`floaters_`).
- No sim/wire/epoch change (render-only by backlog contract).

## Acceptance criteria
- [x] Kill leaves a countable blood decal that persists and fades; telegraphs stage 1-2-3; circles hold until zone change.
- [x] Decals sort under entities; replay bit-exact.
- [x] Suite green.

## Tests required
- `tests/test_decals.cpp`: blood TTL boundaries + fade shape, telegraph TTLs + circle persistence, kill-20 count + FIFO cap eviction. Suite 162→**165**, 328,518 assertions.

## Out of scope
- Server-driven telegraph casts (Gravemother has no telegraph attack yet — the API awaits it); render-to-texture chunking (single surface, cap-bounded — chunking if profiling demands); anything wire/sim.

## Evidence
- Suite: **165/165 (328,518 assertions)**, ctest 2/2, warning-free.
- Replay: `logs/t084.bwj` OK 6001/718/241 mm=0; `logs/t082.bwj` OK 12801/7391/129 mm=0 (render-only proof).
- Headless screenshot block (no display) per brief §verification-5: draw wiring stated, not faked.
- Devlog: `docs/devlog/0053-decal-layer.md`.
