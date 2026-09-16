# 0109 — T-159 loot depth shipped (epoch 29, wire 242)

2026-09-16. Rarity 0..3 (78/17/4.6/0.4) + helm/amulet/ring slots + affixes 11..20, all hooked (Hollow, Grave-touched, Crypt, Marrow, Pall +2acc/+1evd, Boneyard +5% crit, Dirge, Husk, Tithemaster, Last Rites). 8-field blob, ItemSlot rarity wire (241→242), epoch 28→29.

Evidence: headless 314/314 (2,330,419 assertions), ctest 2/2; `logs/t159.bwj` (8 fighters ×30s, ticks=641 cmds=417 hashes=6) replays mm=0; `logs/epoch28.bwj` refuses exit 4. Card `docs/tasks/T-159-loot-depth.md` records 5 deviations (single-affix tiers, +9 rows not ~45, client chrome deferred, Boneyard slot-law pin, no live drop log). GDD §7 amended. Follow-up T-159f1 filed in-card. Next: T-154 headless gate + T-168 build-dir law.
