# Devlog 0011 — Proc riddles and the replay flake (Sprint 10)

2026-09-05 · Sprint 10 · cards T-047, T-048 done · T-049 opened

Full aura ladder is now real: tiers I–V all do something on the wire, and the
last furniture-band magic numbers are gone. In exchange the day coughed up a
replay flake that does not belong to the aura code at all — documented and
carded, not hidden.

## Shipped

**T-047a — furniture floor constant.** `shared/content/wirekind.h`:
`kWireKindFurnitureFloor = 64`, `kWireKindVendor`, `kWireKindAnvil`,
`wireIsFurniture/wireIsMob`; all site filters (server, bots, client, tests)
use the vocabulary now. `kMobs` table carries a
`static_assert(kMobKindCount < floor)` — mob wire kind is the 1-based table
index, so the assert is the load-bearing rail for "add the 63rd mob" day.

**T-047b — aura procs III–V in the combat resolver:**
- tier III **Widow's Edge**: cleave — the rolled damage also lands on up to 2
  extra mobs within 2 tiles of the victim (era rule: 100% hit on the spill),
  kills deferred to end of swing (deque-erase safety, learned once already).
- tier IV **Sunder**: 15% per landed swing for +35 flat damage.
- tier V **Graft**: 25% chance to steal 20% of dealt damage as health.

All proc rolls draw from `World::rng_` in-engine; all procs emit
`CombatEvent kind=7` with flavor-coded amounts (1xxx/2xxx/3xxx) so the client
can call the shot with red-caps text. Tests: cleave bleeds packmates, sunder
observed over the maximum table roll (Sexton target dummy), graft heals a
1-hp litigant back up. **51/51 green.**

**T-048 — anvil panel + moral readout.** Auto-open panel by the Widow bench:
armed blade, current tier, next-tier toll (skill/parts/gold), failure law in
plain words ("the first rite of each tier is guaranteed"), F petitions.
HUD gains a karma line (clean/stained/gray with tint), inventory rows get a
`+I..+V` teal aura mark, furniture entities render as stalls/anvils rather
than hero sprites, and OwnStats carries karma (protocol **v231**).

## The replay flake (T-049, open, not aura's fault)

Cadence-25 hashing (`BH_HASH_CADENCE`) exposed it: 6-bot soaks intermittently
diverge — first as a single walker-tile slip (tick ~125: one path settles one
tile differently), sometimes later as a permanent split (one mob alive at
hp=4 vs dead). Evidence set: `logs/2026-09-05-s10-gate-srv.log` +
`…-s10-journal-cad25.bwj` (200/209 mismatch) vs control fighter-only run
`…-s10-ctrl*` (30/30 OK), plus the canonical S9 journal still **45/45 OK** on
the current binary. Two control facts pin the blame outside T-047: a control
with anvil disabled failed with the same tick-125 slip, and replaying one
journal twice gives identical results — so the **record** path is missing a
lane under load, not the replay reader.

Prime suspects for T-049: journal ticks captured at process-time vs
queue-time under cmdq bursts, fair-share (1 cmd/session/tick) vs replay
mass-apply, and walker sub-step cancellation across path repaths. Fix
direction: journal commands at **receive** time with an explicit drain marker
per tick, and replay honoring fair-share exactly.

## Debug surface that stays

`BH_DUMP_ENTS=1` live entity dumps each hash-line tick, first-mismatch full
entity table dump in replay, `[anvil-refuse] … why=…` traces. These caught
two bugs today before lunch.

## Gates of record

- tests: 51/51 (sum: anvil, karma, procs, bindstone, zones, trade…)
- S9 journal replay: OK 45/45 on final binary (regression floor)
- 2-bot pilgrim gates (tier I + II live rites): replay OK 45/45 hashes,
  4401 ticks, tier II regen live
- 6-bot tier-I–II gate: live log shows 5 rites (`res=ok mercy`), replay
  flake as above (T-049)

## Next

- **T-049** record-path determinism (receive-time journal + drain markers).
- T-034 balancer v2 (gear-aware duel harness).
- M2b campaign run L1→8 across both zones (needs T-049 for its wipe replay).
- Proc balance: 15%/+35 and 25%/20% are first-guess numbers, balancer-driven.
