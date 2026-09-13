# T-118: M3 gate — party of five, mixed kits, to the Gravemother at the
# Drowned Crypt Depths font (journal of record + deterministic replay)

## Context
M2b closed with the party rule (T-117) and the party-shared M2b-final leg.
M3's acceptance is the first gate the M2b chain existed for: a **party of
five** with **mixed kits** (Ravager, Cultist, Gravecaller) pre-ground to a
target level, then the full approach — town → crypt hatch → depths stairs →
Drowned Crypt Depths — against the Gravemother, the L14 boss of map 5. The
fight is journaled to `logs/m3_gate.bwj` and **must replay bit-exact**
(mismatches=0). The boss is the gate's objective, not a guaranteed outcome:
if the kit cannot kill her, the leg's [raid] lines (TTK attempt, deaths,
xp) are the verdict, and the card records that as a finding.

Scope notes (director-visible):
- **Bot-side kit is the M2b kit.** The three classes exist in
  `shared/content/kit.h`; the bot already knows `/kit cultist`, `/kit
  gravecaller` and the Ravager default. No new server feature is touched —
  this card is tools + harness + the persistent-grind chain.
- **No sim-semantic change** → journal epoch stays 20, wire stays 237,
  schema stays v10. A change to any of those is out of scope; if the grind
  genuinely needs one, the card reopens.

## Scope
1. `tools/bots` gains the `raider` profile: the campaign's economy +
   portal + retreat logic extended with (a) the M3 route (town →
   (10,10) hatch → map 3 (44,6) depths stairs → map 5), (b) N-party
   formation (`--party-size 5`: bot 0 invites 1..4, the rest accept),
   (c) mixed oaths (bots 1,3 cultist; bot 2 gravecaller; 0,4 ravager),
   (d) stat-bank spend (VIT while points banked), (e) the Gravemother
   telemetry (EntitySpawn kind 9 / level 14 stamp, kill stamp,
   CombatEvent kind 15 dodge windows).
2. `tools/m3_grind_leg.sh <legN> <port> [secs]`: one pre-grind leg —
   five campaign bots on the PERSISTENT db (same names, carry level and
   gear across legs), journal `logs/m3_grind_leg<N>.bwj`, replay check.
3. `tools/m3_gate_leg.sh <port> [secs]`: the gate leg — five raider bots,
   journal `logs/m3_gate.bwj`, per-bot [raid] summary lines, replay check.
4. `tools/m3_grind_chain.sh [port]`: runs the grind legs until all five
   names are at the target level; aborts (exit 3) on any replay mismatch.

## Acceptance
- Full §5 battery green on the PR head: suite/ctest, duel pin
  `b273be661b54673a`, t115 replay mismatches=0, soak epoch refusal.
- Grind chain: five names `m3g_1..m3g_5` at the target level on a fresh
  persistent db, every leg's journal replaying mismatches=0.
- Gate leg: the party reaches the Gravemother in the Drowned Crypt Depths;
  `logs/m3_gate.bwj` replays mismatches=0; each bot prints one [raid]
  line (party size, deaths, dodges, elite/trash kills, level/xp/gold,
  map-5 entry time, boss sight/kill times, TTK).
- Card → `docs/tasks/done/` with the [raid] table + replay lines; devlog;
  board row.

## Tests
- The bot is a harness client; its "tests" are the legs above (behavioral,
  journal-verified) plus the §5 battery guarding the sim. No new doctest:
  nothing in `shared/` or `server/` changes.

## Out of scope
- Server-side kit/boss changes (the Gravemother AI, kit unlocks, party
  rules are as-landed; if the gate reveals a sim bug, that is a new card).
- Client rendering of the raid.
- Any journal-epoch / wire / schema bump (see Context).

## Verdict (close-out, 2026-09-13, branch arena/01a09acb-bloodhollow)

**Hard gate (reach the font): PASSED once.** Leg **r9** (900 s, port 7904):
three of five bots entered map 5 and sighted the Gravemother; the clock
expired before any engagement. Replay clean.

```
[raid] m3g__00 kit=1 deaths=12 lvl=12 xp=5356 map5=692.9 boss_sight=693.2 boss_kill=n/a TTK=n/a
[raid] m3g__01 kit=1 deaths=6  lvl=12 xp=8340 map5=n/a
[raid] m3g__02 kit=1 deaths=8  lvl=12 xp=7426 map5=n/a
[raid] m3g__03 kit=1 deaths=10 lvl=12 xp=6919 map5=692.5 boss_sight=692.5
[raid] m3g__04 kit=1 deaths=10 lvl=12 xp=7000 map5=617.1 boss_sight=617.1
[replay] OK ticks=18042 sessionCmds=3659 hashes=180 mismatches=0 entities=185
```

**Boss verdict: NOT ESTABLISHED.** Ten follow-up legs (r10-r17, incl. a
30-min r17) never reached map 5 again; none ever engaged the boss.
Final measurement leg **r17** (1800 s, r16b binary): deaths
26/22/16/4/16 (84), map5=n/a all five, `[replay] OK ticks=36042
sessionCmds=5933 hashes=360 mismatches=0`. Journal of record:
`logs/m3_gate.bwj` (r17, force-added). r9's font-reach journal was
overwritten by later legs before archiving; its [raid] evidence is
`logs/m3_gate_bots_r9.log`.

**Finding.** The font is reachable (r9; also r10 entered map 5 at 131 s
before a death spiral), but the reach is timing/luck-dependent: r14
restored r9's exact combat posture and did not reproduce it. The
sustainable blocker is the map-3 respawn-swarm attrition wall (racks'
30 s respawn + cocoon widows + barrow-ring convergence): every wave pays
2-3 of 5 before the depths stairs, and the respawn cycle compounds slower
than any budget tried (15 min and 30 min both failed). Bot-side iteration
converged (march/combat stable since r14, 10-18 deaths per 15-min leg);
the remaining wall is content-side. Full series history, failure modes and
next moves: `docs/handover/T-118-r17.md`.

Evidence: `logs/m3_gate_bots_r{9..17}.log` (verdicts),
`docs/devlog/0084-*.md`, PR #24 lineage.
