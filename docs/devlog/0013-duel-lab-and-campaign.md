# 0013 — Duel lab, portal livelock, and the zigzag campaign

Sprint 12, 2026-09-05. Cards: T-034 (balancer v2) + M2b (campaign gate).

## T-034: bh_duel — soak-free balance numbers

The T-031 in-situ EMA feed could never separate *loadout* from context: its
"Feral Ghoul 18 s (fist-wall)" conflated walk-in, contention, and swing-lag.
`tools/duel/bh_duel` builds an arena World in-process (no net), equips a
player through the **real** progression/inventory path (awardXp → assignStat
build → debugGive → toggleEquip → InvSlot.aura), seeds the world rng per rep,
and answers in 0.23 s what a 126-condition soak evening used to cost.

Headline (`logs/duel-table-s12.csv`, 9 reps each, mob level vs player level
{L-1, L, L+2} × 6 gear steps):

- Gear gradient on-level (Ghoul L3): fists 8.1 s → blade 4.0 s → blade+weave+
  vials 1.6 s. The 2× gear step is intact; sustain is the fight-flipper.
- Walls hold: any bare-kit +0..+2 parse against +1..+2 content is 0–2/9.
  Gravecaller L11 bare blade on-level = 2/9; veteran kit = 9/9.
- Boss by design: Sexton L12 at-level caps 8/9 only under aura IV + plate +
  weave (10.1 s med), 3/9 with weave+vials — exactly the party wall we want.
- Design flag: aura IV blade on an L1 char 1-shots rats/bats (0.1 s). Nothing
  stops a veteran trading a maxed blade to a fresh char; if the low curve
  matters, tier-gate aura transfer, not the aura numbers.

`--selftest` (identical seed ⇒ identical win/ticks/hp/worldHash) is in CTest.

## M2b: the campaign that fought back

New `campaign` bot profile: level-routed waypoints (rats→bats→ghoul spine),
player-paced rests (60–90 s on / 6–11 s off), singles-pull discipline,
retreat-and-sip at ≤50 % hp, flask belt of 4 + Hide Armor maintenance, and
zone awareness from `Welcome.mapId`. Hunt was analyst work, not tuning:

1. **Spawn-camp blindness**: the first route waded between gnoll packs at
   home because campaigns walked *to* camp centers, not camp *edges* — the
   deaths number ([bots] SUMMARY) isn't in the zone log at all; same-zone
   respawns log nothing. Instrument before optimizing. 68→93 deaths/leg.
2. **Surround-reflex overcorrection**: any 3 ghouls within 2 tiles caused
   perpetual retreat chains (leash 14) — freeze without death. Reverted to
   hp-only retreat.
3. **Portal livelock**: hatch portal needs the walker *settled on the rect*.
   Bots "arrived" 2 tiles away and milled; self-defense against bats then
   path-cleared every approach. Portal legs are now pacifist + exact-tile.
4. **Widow cocoon on the crypt landing** (24,26–30): the hatch drops you into
   the leash of 6×L9. Twelve-second round trips. The crypt is party-band
   content — campaign walks past, which is the era answer.
5. **Traveling-shopkeeper**: `nearTown` meant "near first login" — in map 2
   sat by the bats ruin spamming refused sell/buy calls (shops=366, kills=11).

## The replay bug the gate exists for

Leg 3 (first cross-zone resume, character persisted in map 2) failed replay
**103/103 hashes** — not the T-049 flake but a systematic spawn mismatch:
journal login lines (`l`) never recorded `zoneId`, so replay rebuilt
m2b__00 at (18,46) *in zone 1*. Journals are now v2 (zone column after x,y);
replay still accepts v1/13-field legacy lines. Same-area latent bug fixed in
passing: `kStep` used the zone-1 cost grid for step validation regardless of
the entity's zone (`applyWorldCommand` now takes `gridOf(e)`).

## Gate evidence

Chain: `tools/m2b_gate_chain.sh` — resumable legs (session restarts prove
persistence; each leg replays bit-exact before the next starts).

**12 legs × ~9 min, ~95 min wall, freshDB→campaign.** Every leg replayed
**/OK — 1,139 hash markers, 0 mismatches**, across server wipes, zone
transfers, and the v1→v2 journal format bump mid-chain (leg 3 old-format
journal caught the zone bug; v2 verified legs both sides of it).

Campaign pacing table (first time each level was reached, by bot):

| bot | L2 | L3 | L4 | L5 | L6 | L7 | L8 |
|---|---|---|---|---|---|---|---|
| m2b__00 | 8 s z1 | 173 s z1 | 289 s z1 | 537 s z1 | 267 s z1 (leg 9) | — | — |
| m2b__01 | 33 s z1 | 235 s z1 | 4 s z1* | 237 s z1 | 158 s z1 (leg 10) | — | — |

*m2b__01 re-entered leg 3 later (wipe-resume), so the fast L4 is a persisted
leg-2 carry. Chain totals: 519 kills, 216 deaths, 38 XP-debt level drops,
peak level reached: **6**. Cross-zone presence recorded on maps 1/2/3.

## Verdict (and what it means for the curve)

M2b exit text says "take a Ravager 1→8 in one sitting." With disciplined
singles play the curve delivers L5 at ~9 min and then plateaus at L6:
ghoul-spine throughput (~50-67 kills/leg at 90 xp) can't outrun late-band XP
bars (3 660 / 4 690) plus a ~20 % death tax. **L1→8 within a sitting is not
true yet** — the era gate stays *open*, carried to Phase 3 as T-033 already
deferred, but now with instrumented proof instead of vibes:

- replay-across-wipe is *done* (the hard substrate);
- the duel table says exactly which lever moves it: Hound L5 on-level contact
  TTK is 2.0 s but camp aggro-ten pulls 2-3 — camp respawn caps and leash
  radii (not mob stats) are the plateau mechanics to retune. Tuning pass
  belongs to the Phase 3 balance card, values in logs/duel-table-s12.csv.

**Retrospective on wording:** "L1→8 in a sitting" will ship behind party XP
(the phase-3 reason to group). Solo-8 today would take the grind era's real
cost: roughly 3-4 h by extrapolation of legs 9-12 (~1 level per ~30 min
post-L5 with zero deaths — and there are never zero deaths).
