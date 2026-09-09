# 0048 — Night base light: director option, priced not built (T-076, S36)

S36 of the extended queue. **No code changed, no tests, no epoch impact.**
GDD §9 says base light radius 6; the tree ships 0 (T-071). Shipped-wins
says keep 0 + fix the GDD line — but that guts the torch (radius 6 =
torch), so this goes to the director with both options priced.

## Option A — GDD-literal base 6 at night

- Change: `Entity.lightRadius` defaults to 6 whenever `isNight()` holds
  (evaluate at use time, not stored — no migration, no save change).
- Consequences: torch becomes duration-only (6000t refresh at the same 8g
  counter — price untouched, value proposition thins); lantern still +2
  and infinite; mask constants unchanged (falloff already handles 6);
  `night_ghouls` aggro unchanged (sight, not light).
- Cost: content sim change → **epoch bump** (12→13) + fresh soak journal +
  replay line. Small diff (<40 lines + tests: base-6 pin, torch-still-sets,
  dawn reverts to 0).
- Pointers: `lightRadius` default (`server/src/world.h`), torch branch in
  `useItem`, `isNight` (`world.cpp:538`), falloff (`lightmask.h`).

## Option B — keep shipped 0, fix the write-up

- Change: one GDD §9 line — "base 6" becomes "base 0; torches 6, lantern 8".
- Consequences: zero. Torch keeps its full value proposition; nothing
  replays, nothing bumps, nothing soaks.
- Cost: one docs edit.

## Recommendation (non-binding)

B, unless night exploration playtests report darkness fatigue — the torch
economy (8g sink, Marta's 6th stock) was balanced around 0, and A spends
an epoch bump to delete a working sink. If A is picked, do it as its own
sprint with the test pins above.
