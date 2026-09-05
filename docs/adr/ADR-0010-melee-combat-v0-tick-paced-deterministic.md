# ADR-0010 — Melee combat v0: tick-paced, deterministic, integers-only

**Status:** accepted (Sprint 5, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

Combat is resolved **on the server tick (20 Hz), never on client time**.
Swings are deliberate: players 16 ticks (800 ms), mobs per-def. All combat math
is integer-only and flows through `sim::Rng` (see ADR-005): the formulas from
GDD §4 are mechanically transcribed into `shared/sim/combat.h`:

- hit: `clamp(55 + (ACC − EVD), 5, 98) %`, ACC=2·DEX, EVD=DEX (gear terms land
  with inventory, T-021)
- damage: `dmg = base·(100+2·STR)/100 · 100/(100+DEF)`, crit ×170%
- PvP scalar 0.65 applied multiplicatively when both parties are players

XP: `xpNext(L) = round10(100·λ^1.85)` is a **precomputed literal table**
(`kXpNext[26]`) — no `powf` anywhere near the sim; the curve is frozen in
source and identical on every platform. +3 stat points per level, never
auto-assigned (GDD). Mobs are content rows (`shared/content/mobs.h`) — adding a
mob is a table edit, never a systems change.

Intent model: `AttackRequest(targetId)` sets a sticky chase+swing lock,
cancelled by movement or target loss (designed for Lineage/Helbreath click-to-
fight pace). Mobs: proximity aggro (or retaliation for passives), leash radius
returns them home; mob deaths schedule their spawner's refill.

## Consequences

- Combat is unit-testable headless (`tests/test_combat.cpp` runs full
  World-vs-rat/hound scenarios without a socket).
- TTK math is tunable by data alone; S7's balancer bot (bots v2) reads these
  same tables.
- Death v0 = 3 s respawn at town, full heal; **XP debt + corpse land in S6
  (T-026)** and build on the same kill event.

## References

`shared/sim/combat.h`, `shared/content/mobs.h`, `server/src/world.cpp`
(`trySwing`/`mobThink`), `tools/bots` fighter profile, `docs/02-gdd.md` §4.
