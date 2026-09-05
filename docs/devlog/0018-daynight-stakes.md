# 0018 — the dark pays now (Sprint 17): nightcreep + night economy

## What shipped

**T-061.** After 21:00 the world hurts more. Mob swings land ×1.15 (floored,
single band) and aggro eyes see one tile further — read-side, so dawn
doesn't魔术 "de-aggro" a pack mid-bite. Day is indistinguishable from
yesterday's build down to the last roll: the hooks are pure multipliers
off `sim::hourAt(tick_)`, consume zero randomness, and stay strata-clean.

**T-062.** The dark pays. Kills at night earn +10% XP and +25% relative
loot odds (junk *and* the S16 gear-drop table). The "permanent day mod"
lesson from Soma's private servers is honored in the only two ways that
respect it: (a) night is now the richest hunting hour — not a handicap to
be modded away; (b) the overlay physically cannot drown readability —
alpha caps at 150 of 255 at the deepest key (already true since T-008;
now a *documented contractual floor* in the T-062 card).

"Blood Bolt +25%" from the remainder prompt is deferred into S18 by name:
it belongs to the Gravemother's kit; there is no ranged mob-ability slot
yet to hang a 25% bonus on.

## Gates

- Suite **87/87** (327,479 assertions) incl. 4 night cases: band edges
  (20:59/21:00/03:30/05:00), paired-world damage ratio in (1.10, 1.20),
  paired-world XP exactly floor(×1.10), 400-kill-per-clock empirical loot
  witness (night > day, ≤2×).
- Smoke: S17 leg replay **bit-exact** — `ticks=1292 sessionCmds=209
  hashes=12 mismatches=0`. Journal epoch intentionally **unchanged (4)**:
  no new rng draws were introduced, old S16 contract stands.

## Numbers worth remembering

- One Soma game day = 4 real hours; 12000 ticks/hour; night is 33% of the cycle.
- Paired-worlds testing (fresh seed, one dial moved) — now the standard
  pattern for any branch that would otherwise tangle strata.

## Next (Sprint 18, content drop)

T-063 Bonehowl Mine + Drowned Crypt maps; T-064 Gravemother L14 ×20 +
elites ×8 (Blood Bolt lands here); T-065 session-scoped bounty board.
M2b-final L1→8 rerun fires after S18.
