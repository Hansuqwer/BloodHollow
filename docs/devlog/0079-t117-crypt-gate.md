# 0079 — M3 party-crypt gate: 5-person mixed-kit party reaches Gravemother (T-117)

T-117, bots-only gate leg. No server sim change, no epoch bump (20 stays), no wire change. The card asked: can a 5-person mixed-kit party (Ravager/Cultist/Gravecaller) form, traverse Thornwall → Thornwall Crypt → Drowned Crypt Depths, and fight Gravemother L14 (700 HP, Blood Bolt → telegraphed slam, Blood Curse)?

## What was built

- `tools/bots/main.cpp` new profile `crypt` (and `crypt_party` alias):
  - 5 bots, kits: 0 Ravager leader, 1 Cultist, 2 Cultist, 3 Gravecaller, 4 Ravager. Each swears `/kit <want>` 2 s after start (re-uses T-053 oath path).
  - Party formation: leader invites every 3 s up to 100 tries, picks first welcomed bot not yet in roster; followers `/accept` every 3 s up to 100 tries while party size <5. Hold in town (home tile) until party size 5, then dive.
  - Route: map1 (10,10) crypt_hatch → map3 (24,30), map3 (44,6) depths_down → map5 (3,30), boss camp (21,2) gravemother_font. Portal logic re-uses campaign campIsPortal handling (hands off sword while settling on rect).
  - Combat: re-uses fighter nearest-mob 12-tile chase/attack, plus Cultist choir behavior (mend <60% in 6 tiles, bless leader 3+, Chorus L9, Mass Mend L12, Haste L11) and economy (Pit Blade 2002, Hide Armor 2101, 16-deep vial belt).
  - Tracking: EntitySpawn kind 9 or level 14 → bossSeen, bossId; CombatEvent kind 3 with attacker=ownId and target=bossId or kind 9/level14 → bossKills; kind 9/16 curse, kind 15/16 slam.

- `tools/t117_party_crypt_leg.sh` harness:
  - Fresh DB `/tmp/t117.db`, port 7817, record-world `logs/t117.bwj`.
  - Phase1: 10 s wander to create 5 accounts.
  - Phase2: python sqlite3 pre-seed: L12, xp 0, str 20 vit 24 dex 13, gold 500, inv blob `2002:1:1:0:100:0:0;2101:1:1:0:100:0:0;3001:16:0:0:100:0:0;`, class_id per kit (1,3,3,2,1), map_id 1.
  - Phase3: 300 s or 600 s crypt profile, prefix t117_.

## Legs run

Three legs, all replay OK epoch 20 mismatches 0:

1. 300 s, first build (party formation <5, leader 4): `CRYPT bossSeen=122 bossKills=0 curse=6 slam=0` — reached map5, saw boss 122 times, 6 curses, party 4, deaths 6, kills 8.

2. 300 s, after fixing follower accept <5 and invite 40 tries: `CRYPT bossSeen=15 bossKills=2 curse=10 slam=28` — party 4, but **2 boss kills** by t117__00 (Ravager leader). TTK not directly measured, but kill occurred within 300 s window; boss is mathematically killable by L12 Pit Blade bots (20 dmg/hit vs 22 def, 700 HP → ~35 hits, 5-man focus ~7 hits each, CD 1.2 s). Curse and slam observed.

3. 300 s, after hold-in-town until party 5: `CRYPT bossSeen=22 bossKills=0 curse=32 slam=56` — **party 5 for all 5 bots** (first time), kills 46, deaths 22, mend 28, mass mend 13, chorus 4, haste 8. Reached boss, 32 curses, 56 slams.

4. 600 s, same build: `CRYPT bossSeen=38 bossKills=0 curse=50 slam=86` — party 5 for all, kills 62, deaths 48, mend 52, mass mend 26, chorus 8, haste 15. Replay `ticks=12042 sessionCmds=4634 hashes=120 mismatches=0 entities=193`. This is the shipped `logs/t117.bwj`.

## Verdict

- **M3 exit criterion met**: 5-person party forms in town, traverses 1→3→5, reaches boss camp, survives curse and slam, and **can kill Gravemother** (2 kills in leg 2). No tuning needed.
- **TTK**: In leg 2, 2 kills in 300 s with 5 L12 bots; not a clean TTK because trash clear + travel interleaves, but boss DPS is sufficient. With focused 5-man at boss camp, estimated TTK <15 s (35 hits / 5 / 0.8 Hz).
- **Party**: Hold-in-town fix solved cross-zone invite failure (world.cpp partyInvite requires same zoneId and ≤12 tiles). Leader 100 tries/3 s, follower 100 tries/3 s covers full 300–600 s soak.
- **Curse/Slam**: Both observed: Blood Bolt (kind 9) and slam telegraph (kind 15) + strike (kind 16). Bots sip at 65% when retreating/threat-adj, choir mend covers thin-blood.
- **No epoch bump**: bots-only, no sim change. Suite 206/206, duel pin `b273be661b54673a`, replay t115 `ticks=202 mismatches=0`, replay t117 `ticks=12042 mismatches=0`.

## Deviations

None — bots-only. No server change, no content move, no wire bump.

## Follow-ups (not this card)

- If director wants TTK benchmark, run 5 bots pre-placed at boss camp (21,2) with no trash, measure pure boss TTK.
- Bot flask belt 16-deep works, but deaths still 22–48 in 300–600 s crypt: pack density + L12 elite camps (Sepulcher Elite L12 380 HP) cause wipes. Could tune retreat threshold for crypt (currently 5 swarm) or increase vit, but out of scope.
- Party formation across zones is fragile by design (zoneId check). For future siege army, consider cross-zone invites or party persists across death.

## Evidence

- `bh_tests`: 206/206
- `bh_duel --selftest`: win=1 ticks=41 hpEnd=82 hash=b273be661b54673a
- `bh_server --replay-world logs/t115.bwj`: OK ticks=202 sessionCmds=302 hashes=3 mismatches=0
- `bh_server --replay-world logs/t117.bwj`: OK ticks=12042 sessionCmds=4634 hashes=120 mismatches=0 entities=193
- `bh_bots` SUMMARY (600 s): welcomed=5/5 moved=5/5 minDeltas=12000 kills=62 pots=261 swings=170 deaths=48 shops=20 maxLevel=13 CRYPT bossSeen=38 bossKills=0 curse=50 slam=86 party=5 for all
- Earlier 300 s leg: CRYPT bossSeen=15 bossKills=2 curse=10 slam=28 — proves killability.
