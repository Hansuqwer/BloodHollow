# 0037 — bad-leg death levers: one measured move (T-074)

S25 of the overnight queue. Bot-only: no epoch bump (stays **11**), no sim
change, no replay impact.

## Bench (desk analysis)

Rerun-chain legs 3/5/7 (`logs/m2b_rerun_leg{3,5,7}_bots.log`):

| leg | deaths | killerByLvl | lastDeath | maxLevel |
|---|---|---|---|---|
| 3 | 142 | L3:64+78 (ALL) | (55,14) | L3/L2 |
| 5 | 134 | L3:66+68 (ALL) | (52,11) | L2/L1 |
| 7 | 116 | L3:52+64 (ALL) | (49,24)/(49,25) | L4/L3 |

**L3-ghoul treadmill**: pin at ghouls_east/orchard edges, ~130 deaths/leg,
de-level to L1–L3, mend 128+ (heal through it, still lose), shops ~100.

Mechanism (poverty trap): the pack cap `b.level >= 3 && pack >= 2`
(`tools/bots/main.cpp:611`) turns OFF below L3 — when the bots are weakest.
`pack` counts only aggressive mobs, so the gate's starter-diving reason is
half-stale (rats exempt by passivity; only bat packs change).

## The one lever

Dropped the `b.level >= 3` gate on `pack >= 2`. Hypothesis: de-leveled bots
stop diving 3+ ghoul packs; starter diving survives on passive rats +
singles. Retreat (`swarmOnUs >= 5`) untouched.

## A/B (2-bots-alone, 540 s each)

| arm | start | deaths | killerByLvl | maxLevel | notes |
|---|---|---|---|---|---|
| fresh BEFORE | L1 town, current bin | 0 | — | 5 | no treadmill on fresh starts |
| L3-park BEFORE | L3 @ (55,14)/(54,13) | 24 | L3 all | 5 | signature reproduces in miniature |
| L2-park BEFORE | L2 @ same, gate ON | 32 | L3 all | 5 | gate-off window shows |
| L2-park AFTER | L2 @ same, gate OFF | **104** | L3:102 + L1:2 | 3 | lever rejected, reverted |

BEFORE journals: `logs/t074_before.bwj`, `logs/t074_tread_before.bwj`,
`logs/t074_gateoff_before.bwj`. AFTER journal: `logs/t074_after.bwj`
(unreplayed — bot-only, no sim change; regime evidence only).

## Verdict: lever REJECTED, reverted

Dropping the level gate **tripled** deaths (32 → 104), halved the ceiling
(L5 → L3), and let rats onto the killer board (L1:2 — full poverty
spiral). Same start state, same ground (54,14), same killers: the effect
is the lever, not the seed.

Post-mortem hypothesis (for the next iteration, not a claim): the cap
helps bots that can afford to be picky and hurts bots that must race —
skirting a ghoul pack edge-pulls the whole pack onto a bot that is now
focusing a single, while diving in thins the pack before it compounds.
The L1–L2 gate window is also too short to matter (both arms leave L2 in
~13 s with gear); the divergence compounds through debt (15 drops).

**Shipped: nothing.** `tools/bots/main.cpp` reverted to v5c, bots rebuilt.
Next measured pit, in order: (1) retreat threshold 5→4 in the same L2-park
regime (untouched this round); (2) repeat arms (n=1 pair each — chaos
dominates, the 0025→0029 discipline wants triples); (3) L1-naked variant
to isolate the gate with the climb confound removed. No blind tweaks.
