# Devlog 0006 — Sprint 7: trade, density, and a world that replays *(2026-09-04)*

Seventh sprint closes Phase 2 (Bleak Fields → M2). Four cards: trade, density,
balancer, journal replay.

## Delivered

- **Trade window (T-029, ADR-0011)** — T opens with the nearest player; click
  items in the bag to offer them, **H** raises gold offer, **P** commits, **X**
  cancels. Server-side it's *commit-time validation*: offers are intents;
  nothing moves until BOTH sides commit, then the swap is validated (ownership,
  quantities, gold, distance) and executed in one sim tick. Disconnect, death,
  or walking off auto-cancels. Impossible to dupe or to strand escrow — there
  is no escrow. Protocol now v4 (30 messages).
- **Fields depth to L11 (T-030)** — Plague Bat swarms (L2, fast, annoying by
  design: the "die red-faced" exam answer), Bonepicker Gnolls (L7), Charnel
  Widows (L9), a Gravecaller behind the west barricade (L11). Ten spawners with
  an S5-derived density rule (throughput ≥ hunters). L12+ is crypt territory —
  Phase 3's door.
- **Bots v2 + balancer feed (T-031)** — grinders now pawn junk bags, buy Pit
  Blades, and equip them; SUMMARY adds deaths/shops. Server kills now carry a
  TTK probe (`firstHurtTick`), EMA'd per mob kind, printed at soak FINISH.
  First numbers: Marsh Rat 4.7 s (target ~6 – slightly fast, keep an eye),
  Feral Ghoul 7.3 s (band ✓), Hollow Hound 20.2 s (+3 levels: punitive ✓).
- **Deterministic world replay (T-032, M2 gate infra)** — `--record-world`
  journals logins, world-mutating commands, disconnects, and a worldHash every
  100 ticks; `--replay-world` rebuilds the universe offline and verifies every
  marker. Verification artifact: 801 ticks, 308 queued session commands, 9/9
  hash checks, **0 mismatches** (`logs/2026-09-04-journal-verify.bwj` +
  `-journal-replay.txt`). The M2 "a wipe replays bit-exactly" criterion now has
  its tool.

## Incidents (autopsy)

- I `pkill`-ed by a loose pattern while a gate was live and killed its bots.
  Gate invalidated, rerun with `stdbuf -oL` so evidence can't silently sit in a
  buffer again. Rule added to my checklist: never pkill near a live gate;
  always match full command lines with anchors.

## Balance pack v2 (post-fix rerun, archived `logs/2026-09-04-balance-v2-srv.log`)

Two instrumentation fixes verified by rerun: chaser-tag reset at the den, and
bot swing cadence 1.5s→0.9s (the server CD is 16t=800ms; the old bot lag was
inflating every TTK ~1.7x). Fresh table (240s, 8 fighters, fresh DB):

| mob | L | kills | TTK | reading |
|---|---|---|---|---|
| Marsh Rat | 1 | 17 | 5.0s | starter, band floor ✓ |
| Plague Bat | 2 | 7 | 4.9s | swarm ATK fodder ✓ |
| Feral Ghoul | 3 | 21 | 18.0s | fist-wall → ~6-8s with Pit Blade (gear gate ✓) |
| Hollow Hound | 5 | 9 | 22.9s | +2 levels, punitive ✓ |
| Bonepicker Gnoll | 7 | 1 | 29.8s | +4, wall by design |
| Gravecaller | 11 | 2 | 329s | poacher wall; do not solo |

Soak: 4801 ticks, p99 0.20 ms, p50 13us. Conclusion: equal-level band is met
once the gear curve engages (fists→blade ≈ 3x dps); no content numbers changed.
Standing S9 card T-034: gear-aware bot duel harness so these reads come with a
weapon column.

## Status

- Tests: **41 cases / 327,129 assertions green**.
- Protocol: v4 (30 msgs). Map: 10 spawners / 7 mob kinds L1-L11.
- M2 gate rerun: in flight at this writing; verdict lands in devlog 0007.

## Next: the M2 gate pass itself (T-033), then Phase 3 — Thornwall Crypt
(zones) and the first aura quest set (Soma adoption: Anvil crafting spine).

---

## Appendix — S7 gate result (rerun after the pkill incident)

*620 s soak, 12 grinder-fighters + 4 wanderers, fresh DB, new 10-spawner map.*

```
[soak] FINISH ticks=12401 online=0 entities=31 tickUs p50=23 p99=237
[soak] OK  p99=0.24ms  (budget 10.00ms)
[bots] SUMMARY welcomed=12/12 moved=12/12 minDeltas=12000 kills=116 pots=316
       swings=842 deaths=0 shops=13 maxLevel=4
[bots] SUMMARY[wander] welcomed=4/4 moved=4/4 kills=0 maxLevel=1
```

Balancer table (EMA TTK vs attacker mix; first *honest* look):

| mob | L | kills | TTK | verdict |
|---|---|---|---|---|
| Marsh Rat | 1 | 36 | 4.8 s | fast side of band; acceptable starter |
| Plague Bat | 2 | 21 | 4.7 s | swarmy-but-quick; as designed |
| Feral Ghoul | 3 | 29 | 33.6 s | **off-band** — chase/leash overlap inflates |
| Hollow Hound | 5 | 16 | 34.6 s | punitive vs L2-4 by design |
| Bonepicker Gnoll | 7 | 9 | 285 s | wall to 1-man ravagers at L4 — intended gate |
| Charnel Widow | 9 | 1 | 233.6 s | n=1, sniff only |
| Gravecaller | 11 | 4 | 131.7 s | soft-capped grinder target; okay for now |

Follow-ups committed: metric hygiene (firstHurtTick resets when the mob idles
at its anchor), and a tuning card for S8 (ghoul chase-time vs combat-time
ratio; gnoll gold/XP payment vs group effort). Shops=13, pots=316: the grinder
economy v2 loop works end to end. deaths=0 across 600 s bots holds: rats are
rats; M2 human pass will go pick fights like a person would.
