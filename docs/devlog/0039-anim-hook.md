# 0039 — Client anim-state hook: combat frames play (T-ART-04, S27)

S27 of the extended queue. Render-only: no sim, no wire, no epoch impact
(stays **11**). No journal touched — replay parity holds by construction
(mask/decal class: the server never sees this code).

## Design (backlog timing truth, honored)

The client learns about swings only when the `CombatEvent` pulse arrives,
i.e. post-resolution — so the attack/cast anim STARTS on the contact
frame. No wind-up prediction (no intent messages exist), no authority.

- `engine/render/animstate.h` (new, raylib-free): `EntAnimState`
  (none/attack/cast/hurt/die), durations (attack 8t = half the 16t player
  swing cadence so a swing always finishes; cast 8t; hurt 4t flinch; die
  held, not timed), `animNameFor` selection table.
- `RenderEnt` carries state + start/expiry stamps (`game.h`). The
  `combatIn` loop in `applyNetState` sets: swing kinds (1,2) → attack on
  the attacker; cast kinds (5,9,10,13,14) → cast; hurt kinds (1,2,5,9) →
  hurt on the target; kind 3 → die. Victim-side wins ties; furniture never
  fights.
- `drawRemoteEnt` resolves the anim on a per-state clock (frame 0 at the
  pulse tick) and falls back to walk/idle when the atlas predates combat
  frames (`animFrame` returns empty when absent — every current sheet
  takes this path until B3+ lands, which is correct, not a bug).
- Die clears when the same id breathes again (respawn); despawn removal is
  unchanged (vestige layer owns the corpse beat).

## Tests

`tests/test_animstate.cpp`: selection table + duration pins (incl. the
die-hold −1). Atlas-missing fallback and the pulse→state wiring are
headless-untestable (need textures / a live server) — stated, not faked.

## Files

`engine/render/animstate.h` (new), `client/src/{game.h,game.cpp}`,
`tests/{test_animstate.cpp,CMakeLists.txt}`. Suite **137 / 328,115**,
ctest 2/2, warning-free.
