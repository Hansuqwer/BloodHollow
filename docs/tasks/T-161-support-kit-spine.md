# T-161 — Support-kit spine: Resurrect first (P1, audit 12/20)

## Context
Audit findings **B2.3 / B5.3 / R5**. Nine channels ship (ch1 Power Swing, ch2
Mend, ch3 Bless, ch4 Ironskin, ch5 Firebolt, ch6 Chorus, ch7 Mass Mend, ch8 Haste,
ch9 Purify — `server/src/world.h:251-258`, `shared/content/kits.h`). GDD §3 lists
~23, and calls **Resurrect (L20, 5 min CD, returns 50 % of XP debt) "the reason
every party wants one"** — the mechanical basis of design pillar 2 (group-first)
and of the M3 metric "party ≥1.4× solo". Without it, and without Sanctuary /
Raise Skeleton / Curse of Weakness, the Cultist is a heal-and-buff box and the
Friday-Night leg-1 pass criterion ("the Cultist players are fought over in chat")
has nothing to be fought over.

## Scope (priority order; each skill is its own commit)
1. **Resurrect** (ch10, Cultist L20): target must be a dead player within N tiles
   and within M ticks of death; returns 50 % of the XP debt; 5 min CD; MP cost;
   journaled; client callout + sound.
2. **Sanctuary** (ch11): ground healing circle, 600t CD, radius + tick heal,
   decal-visible (T-ART-09 decal layer exists), no stacking.
3. **Curse of Weakness** (ch12): −15 % target dmg/def, duration, purge interaction
   with Purify (one law, tested both ways).
4. Then, per director priority: **Raise Skeleton** (pet entity: new kind, leash,
   despawn law — largest of the four), **Corpse Explosion** (needs a corpse
   entity/decay law), Gravecaller control set (**Frost Spike**, **Wither**,
   **Terror**, **Mana Shield**), Ravager set (**Sunder**, **Bull Rush**, **War
   Stomp**, **Executioner**, **Second Wind**).
- Channels append; `chUnlock[10]` must grow (array width change = wire/DB visible;
  call it out). Hotkeys: extend beyond `1..5` (era games used F-keys — pick a
  scheme and put it in the help panel, T-169).
- Every skill: server-authoritative range/CD/MP/target-legality, journaled, unit
  pinned, and **replay-exact** (no wall-clock, RNG via `sim/rng.h` only).
- OUT: skill % -based weapon skills beyond the shipped mastery, respec, pet loot.

## Acceptance criteria
1. Resurrect: two live clients (or a bot + client) — one dies, one resurrects;
   XP debt halved (sqlite before/after), 5 min CD enforced, refusal cases
   (too far, too late, alive target, non-Cultist) each produce a directed line.
2. Unit pins per skill (range/CD/MP/effect/duration/interaction), plus
   `test_cmdparity` extended for each new `Command::Kind`.
3. Replay: fresh leg with every new skill fired at least once → `mm=0`; epoch
   bumped once for the batch (coordinate T-151/T-159 so the tree takes one bump).
4. M3 party-vs-solo XP/hr re-measured with a full support kit (feeds T-157).
5. GDD §3 skill numbers reconciled with what shipped (T-158).

## Tests required
Per-skill doctest; parity test; a kit leg script (`tools/t161_kit_leg.sh`) with
bots firing every channel.

## Evidence owed at merge
Suite count, leg + replay line, screenshots of Resurrect/Sanctuary, M3 numbers,
devlog, board rows (one per skill batch).
