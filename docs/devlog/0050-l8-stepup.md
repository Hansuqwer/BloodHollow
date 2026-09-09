# 0050 — L8 gets a camp: widow north edge (T-084)

T-084, bot-only. The L7 camp (53,41, devlog 0030) proved the bridge crossing after T-068 but stopped at gnolls. Widow_glade shares the bank 7 tiles east — same river crossing, same retreat-home, harder pullers.

## Change (`tools/bots/main.cpp`, +13 lines)

`if (b.level >= 8)` overrides the L7 camp with (60,41): widow rect x[58,63] y[42,45] north edge centre. Widow stats (mobs.h: L9, hp 220, dmg 22, aggro 6, wander 6, leash 10) vs gnoll (L7, dmg 17, aggro 7, wander 8, leash 14): widows arrive in smaller, shorter-leash pulls; gnolls still overlap from the west. No spawner moved, no numbers touched — the step-up is a waypoint, not content.

## Val (resumed-DB, sqlite-set per the brief's val pattern)

`/tmp/t083b.db` → `/tmp/t084.db`, campaign pair set L8/xp0/map1/(32,16), gold+inv kept. 240 s campaign-only, target-level 9:

| | val |
|---|---|
| kills / deaths | 86 / **0** |
| killerByLvl | *(empty)* |
| maxLevel | 8 (held, no L9 in 240 s — expected; the climb needs a full leg) |
| shops / regear / mend | 9 / 3 / 17 |
| replay | OK 6001/718/241 mm=0 entities=358 |

Zero L11 kills (barricade stays clean from the far-marsh post), zero road-bat deaths on route (bots start in town and walk south — the crossing holds). The L9 verdict (can a pair *climb* 8→9 here at a sane death price?) needs a 540 s climb + chain evidence — parked as the follow-up, not claimed here.

## Judgment calls

- Bot-only, epoch stays 12 (no sim/map change — same class as T-074/T-077).
- Did not move the barricade or add a crossing (options B/C from devlog 0030 stay parked).
- `anvilTries=0` again on this leg (campaign pair, expected — the anvil lane is pilgrim-gated; task 6 owns it).
