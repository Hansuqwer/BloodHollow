# 0031 — M2 gate de-QA: the 14-bot grinder mix replays bit-exact (T-033)

The open card of record (T-033) asked for a soak with a grinder mix, balance
bands checked, a wipe replay attached, and a human trade pass. This devlog
closes the programmable half; the human trade pass remains for the director.

## The soak

`bh_server` (fresh DB `/tmp/t033_deqa.db`, `--soak-secs 900`, journal
`logs/t033.bwj`, epoch 7) + a **grinder mix** of four bot groups against one
server (port 7855):

| group | profile | count | runtime |
|---|---|---|---|
| t33w | wander | 5 | 720 s |
| t33f | fighter | 4 | 720 s |
| t33p | pilgrim | 3 | 720 s |
| t33c | campaign (target L8) | 2 | 720 s |

14 concurrent bots, one zone mix, full combat/economy cross-traffic.

## Results (group SUMMARY lines)

| group | kills | deaths | maxLevel | shops | regear | notes |
|---|---|---|---|---|---|---|
| campaign ×2 | 55 | **0** | **3** | 1 | 1 | L1→L2 t=26/70 s, L3 t=322/383 s; killerByLvl **empty** |
| fighter ×4 | 180 | 76 | 5 | 4 | 0 | levelDrops 8; killerByLvl **L11:60** of 76 |
| pilgrim ×3 | 130 | 46 | 4 | 2 | 0 | anvilTries 0 (parts/gold gates — watch item); L11:37 |
| wander ×5 | 0 | 34 | 1 | 0 | 0 | pacifist decoys; deaths spread L2/L3/L5/L11 |

- **Campaign route health under load:** 2 campaigners climbed L1→L3 with
  **zero deaths and an empty killer histogram** while 12 other bots churned
  the map — the T-068 route holds in a crowded world, not just in a quiet
  2-bot leg.
- **L11 gravecaller dominance (watch item):** 97 of 156 fighter/pilgrim
  deaths are the relocated barricade mobs catching *roaming* melee bots in
  the far south-west — era-correct "die red-faced" content (an L4 fighter
  diving an L11 with 260 hp / 30 dmg should die), and the campaign route
  itself stays clean. But the death coordinates `(12,38)/(13,38)` sit 1–2
  tiles west of the bank road corner: a maximally-wandered gravecaller
  (anchor (1,44) + wander 5 → (6,44)) reaches the road corner at **night**
  (aggro 7 + T-061's +1 = 8). No campaign-route pull was observed, but if
  one ever shows up, the lever is rect `(1,44,1,2)` — a content tweak,
  director-flagged, not a bot fix.
- **Pilgrim anvilTries=0:** the pilgrim profile never reached the anvil this
  leg (parts/gold gates). Watch item for a later economy pass.

## Balance bands (server-computed, live kills)

| mob | L | kills | TTK (25t hps) | xp | band |
|---|---|---|---|---|---|
| Marsh Rat | 1 | 44 | 2.9 s | 40 | ✓ starter |
| Plague Bat | 2 | 22 | 3.1 s | 55 | ✓ swarm-fodder |
| Feral Ghoul | 3 | 52 | 8.9 s | 90 | ✓ |
| Hollow Hound | 5 | 17 | 18.2 s | 150 | ✓ punitive +2 |
| Bonepicker Gnoll | 7 | 6 | 21.0 s | 300 | ✓ |
| Gravecaller | 11 | 3 | 169.6 s | 560 | wall by design |

The L3→L5 step (8.9→18.2 s, ~2×) is the punitive jump the GDD wants;
the L11 is a wall, not a band mob. No off-band outlier.

## Perf

`[soak]` p50 tick 6.9–7.6 ms / **p99 12.9–16.4 ms** at 14 online bots
(bot-mix combat is heavier than the 2-bot chain legs' p99). Under the M5
budget (25 ms), above the M1 target (10 ms) — recorded as the 14-bot
reference point for the M5 sizing work. Entity count stable 544–584, no leak.

## Wipe replay (the T-032 contract, on a real soak)

```
[replay] OK ticks=18001 sessionCmds=7052 hashes=181 mismatches=0 entities=584
```

Journal `logs/t033.bwj` (epoch 7) committed as evidence.

## What remains for the director

- **Human trade pass** — the soak logged trade/anvil/potion activity
  (`shops=7` across groups, `regear=1`, pilgrim part-farming
  `mGold=570 mPelts=3`); the UX pass over the trade window/repair/anvil is
  yours, not mine.
