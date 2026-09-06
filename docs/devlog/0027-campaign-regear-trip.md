# 0027 — the campaigner learns to go shopping (T-034d)

T-034c named the plateau: a death-tax treadmill whose trigger is a bot-profile
gap — the campaign bot only touched town on death-respawn, so its gear-churn
was hitched to dying. T-034d gives it a re-gear trip.

## The change

In the campaign movement block, before the camp-waypoint walk, the bot now
checks affordability: no blade and 260g → walk to the bindstone; blade armed,
no armor, 120g → walk back for Hide Armor. Once near town the existing economy
block does the buy/equip unchanged. Map-1 only; client-side only; movement is
server-validated, so replay stays bit-exact by construction.

## The smoke

`tools/t034d_smoke.sh` — 2 campaign bots, 240 s, fresh DB:

```
[bots] SUMMARY … kills=65 … deaths=22 shops=6 … regear=2 maxLevel=3 …
[replay] OK ticks=4841 sessionCmds=791 hashes=193 mismatches=0
```

The trip fires (regear=2) and the journal round-trips bit-exactly. A 240 s
leg only reaches L3, so the plateau verdict still waits on the full M2b chain
rerun — but the mechanism the evidence pointed at is now wired and
replay-clean.

## Next

M2b chain rerun on this build; if the plateau parks at the armor band, Hide
Armor 120→80 (T-034c lever 2).
