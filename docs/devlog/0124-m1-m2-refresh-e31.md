# 0124 — R5 CLOSE (M1/M2 evidence refresh @ epoch 31)

## M1 soak: 20 fighters × 1800 s (`logs/m1e31.bwj`, `tools/m1_soak_leg.sh`)
Disclosed L6 top-up (level=6/xp0, Pit Blade + Hide Armor + 16 vials via legacy
6-field blob — also proves the 10-field parser compat live).

| Check | Result |
|---|---|
| online @ tick 36000 | 20/20, entities ~200 stable |
| tick p99 | ~1.5 ms steady (budget 10 ms) → **PASS**, 6.7× headroom |
| errors (server + bots logs) | 0 |
| replay | ticks=36100 cmds=23146 hashes=361 **mm=0** → 0 desyncs → **PASS** |

## M2 TTK (`logs/duel-table-e31.csv`, 384 rows, `bh_duel --table`)
Era kit (aura 1 + Hide Armor + Power Swing + 4 vials), equal level: gnoll,
wretch, widow all 7/7 @ 4–6 s med, endhp 0.4–0.65. Naked-blade solo vs L8+
equals stays unfavored (1–3/7) — party-ladder design (T-090/T-100), not a
lever. **Gate HOLDS as scoped (L1–8 solo, L9+ party).**

## M2 pace (same leg, DB read-out)
Start 20× L6/xp0 → end mean +0.10 net levels + 1560 mean banked XP per
30 min (L6 bar 2750) ≈ 0.67 level-equiv / 0.5 h ≈ **1.3 levels/h vs ~1.2
target → PASS**, death tax included (one bot de-leveled L6→L5 — T-025
working as designed). Mean gold 1678 (600 stake → economy positive).

## Still owed after this pass
M1 20×1800 s full-shape done; the 200×12 h M5 + Friday legs stay
director-scheduled. Journals (`m1e31.bwj`, `duel-table-e31.csv`) need
`git add -f` on commit.
