# 0053 — Blood stays on the ground now (T-ART-09)

T-ART-09, render-only. The last parked art-enablement card from S26 (devlog 0038).

## What landed

- `engine/render/decals.h` (new, raylib-free): the whole decal law in 70 lines. Blood 12000t with a 110→40 fade (visible the full 10 min, never glaring); telegraphs 20/20/40/glide-yellow/orange/red (the Gravemother language when she gets a telegraphed attack); circles at alpha 110 until the zone drops them.
- Client wiring: kills stain (`noteDecals` off the same pulse loop as the anim hook — kind 3, victim pos, furniture skipped), `drawDecals` y-sorts under entities, zone reload wipes the surface. Boss APIs (`addTelegraph`, `addCircle`) exist with no callers yet — stated, not dead code: they are the contract the next boss card programs against.
- One test red → green during the sprint: the FIFO expectation forgot the kill-20 phase's 20 resident decals (front bornTick 5, not 20). Fixed by clearing between phases — the law, not the code, was wrong.

## Judgment calls

- Single surface, not per-chunk render targets: the architecture doc says "per map chunk", but a 512-cap FIFO deque y-sorted each frame is the same visual result at this entity count with zero texture bookkeeping. Chunking becomes a profiling card if decal counts ever press the cap in live play.
- Wall-clock vs ticks: expiry runs on the client tick counter (same 20 Hz basis, render-read only). Vestiges/floaters use `GetTime()`; decals need minute-scale TTLs where tick math stays exact across frame hitches. Neither touches the sim.
- No soak journal (render-only): replays of t082/t084 are the proof.
