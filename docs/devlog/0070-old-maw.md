# 0070 — The Maw pit opens (T-101)

T-101, content (epoch 14→15). First of the three named elites, and the card that carries the shared first-blood announce for the other two.

## What landed

- Row 1012 (Gnoll ×1010-pattern to the number), pit spawner in open south-center fields grass, session-scoped first-blood broadcast on the system channel, GDD "random"→"rotating" (the determinism law reaches the design doc at last).
- The announce helper maps all three mobIds now (1013/1014 reserved): when Widow and Cantor land, they get first-bloods with zero announce code — only their rows, spawners, and tests.

## Judgment calls

- Pit at (28,38), derived not designed: open grass off roads and camps, no art input available. If art wants a real pit (terrain, bones, dread), the spawner moves in one mapgen line — positions are cheap, the row and the announce are the card.
- 30-min rotation (not faster): elites are appointments, not farms. maxAlive 1 + 36000t means one Maw per half hour per reboot; the purse (1200 xp) prices a detour, not a camp.
- Soak honesty, stated in the card: map-1 bots will never meet the Maw. The soak proves no regression (boot with 5 spawners, regen flow, replay clean); the unit pins prove the elite. Claiming soak validation of unmet content would be the exact unseen-render fiction the brief forbids.

## Soak (epoch-15 validation leg)

`logs/t101.bwj`: 14-bot grinder mix, 540 s → `[replay] OK ticks=12801 sessionCmds=6760 hashes=513 mismatches=0 entities=178`. Bots: campaign 0/L3, fighter 54, pilgrim 46, wander 56 — healthy bands, entities stable ~170s post-anvil-fix. The Maw kept its pit undisturbed (no map-2 traffic, as stated).
