# Devlog 0010 — The Widow Anvil: auras, pilgrims, and karma (Sprint 9)

2026-09-05 · Sprint 9 · cards T-041 – T-046 · RFC 0001 implemented in full

Soma's signature mechanic — the weapon aura — is now **live, persisted,
journaled, replay-verified, and gate-proven**. A pilgrim bot walked to the
plaza anvil, paid the toll in rat pelts and coin, and the server logged:

```
[bless] bot_00 <- 2001:1,4001:35,skill:25,gold:400 (skill now 25, gold 400)
[aura] bot_00 tier=1 res=ok mercy
```

## What shipped

**T-041 Anvil objects.** "Widow Anvil" furniture entities (wireKind 65;
63+ is now the reserved furniture band) spawn in Thornwall's plaza and in the
crypt. Server gate: Chebyshev ≤ 2, same zone. Client draws them teal and the
**F key fires the next-tier petition** when you stand close.

**T-042 Aura tiers I–V table** (`shared/content/auras.h`). Skill gates
20/50/80/120/150, parts+gold tolls (30 rat pelts & 120g for tier I), failure
laws soft/discard/**destroy** from tier III, and the mercy rule: first try at
each tier is guaranteed (persisted in `anvil_mercy`, schema v5→v6). Live
effects today: tier I flat +3 attack in `equippedWeaponDmg`, tier II adds
%hpMax to the out-of-combat regen beat; proc hooks for III–V are tables +
vfx events awaiting the S10 proc sim.

**T-043 Protocol + persistence.** `AnvilOp` (id 19), protocol bumped to
v131 (wire floor 100+count), `ItemSlot` gains `aura` u8, inv blob extends to
4 fields `id:qty:eq:aura` with backward-compat parse in both live login and
replay, characters gain `anvil_mercy` + `karma` (schema **v6**).

**T-044 karma columns + moral split** (T-046 card). Karma drifts **+2 per
successful tithe, −6 per drowned blade** (the destroy law stains).
The two-sided Soma incentive: karma > 0 ⇒ **+15% XP**; karma < 0 ⇒ **+15%
gold loot drops**. Both halves tested at the world level.

**T-045 Pilgrim bots.** New bot profile: fight like a fighter, but when the
blade is armed and the purse holds the toll, walk home to the Widow's bench
and petition. Fleet summary now reports `anvilTries` + gate counters
(`b/g/p/a/r`) so future balancing runs tell you *why* nobody's enchanting.

**Debug lane:** `--bless name=id:qty,...,skill:N,gold:G` grants at login and
is journaled (`b` lines) so replays reproduce blessed sessions bit-exact.
That was the last determinism hole the aura work opened: replay of the first
anvil gate run diverged at tick 200 because grants existed only live-side.

## Evidence

- Tests: **48/48** (anvil gates+tolls+mercy, tier I effect + tier II regen,
  karma split ×2 incl. destroy-law stain, bindstone respawn from T-040 fix).
- Live gate: `logs/2026-09-05-s9-anvil-gate-srv.log` — 6 pilgrims, 190s:
  51 bot tries, **4 tier-I rites succeeded** (`res=ok mercy`), 53 polite
  "already blessed" refusals, one `nearAnvil` whiff; soak p99 ≤ 922µs.
- Journal: `logs/2026-09-05-s9-journal.bwj` → replay **OK, 45 hashes, 0
  mismatches** over 4401 ticks with aura RNG and bless grants identical.

## Learnings & scars

1. **Buffered stdout lies.** `pkill`-dead servers and `>`-truncated log fds
   made earlier smoke runs look like silent no-ops. The refusal-trace table
   (`[anvil-refuse] … why=…`) earned its keep instantly; keep it.
2. **Skill is derived, not stored.** Blessing `swordSkill` direct does
   nothing — combat recomputes it from `swingLands/20`. Bless now seeds the
   counter. A general rule going forward: any debug path must write to the
   *source of truth* columns, never to derived ones.
3. **Furniture/mob wire-kind boundary**: vendor=64, anvil=65. Filters: mobs
   `< 64`, furniture `>= 64`. Waiting on a central assert (T-047 candidate).
4. The bots' aura refresh lags server truth (they retried 53 times after
   success). Harmless, but a latency-tolerant attempt queue in bots v3 would
   tidy the gate logs.

## Next (Phase 3 continues)

- T-047 wireKind floor convention assertion + proc sim for aura tiers III–V
  (`procId` hooks are in the table already).
- T-034 balancer v2 (gear-aware duels) feeding the first era curve.
- M2b: L1→8 campaign run across both zones stays open as the pacing gate.
- Client anvil panel (f-text banner exists; a real panel + karma readout on
  the OwnStats line is the follow-up).
