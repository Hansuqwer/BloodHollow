# 0100 — T-139 pledge bands (no bump, stays 26)

Sworn captains now muster the sworn war-host: `siegeRegister` keys bands by
pledge id (`siegeBandPledges_` parallel to the 8-band count), late members
muster into the enlisted band without consuming a slot, desertion never
un-enlists. Party shape preserved for the unaffiliated.

Evidence: 286/286 cases (4 new) · `logs/t139.bwj` (rehearsal, epoch 26)
`band enlisted (5 members, 1/8 bands)`, replay mm=0 · `t138.bwj` re-replays
mm=0 (neutrality, no bump) · validate 0/6 · duel selftest green.
One self-caught fix during the card: the 8-band cap initially blocked
late-muster; cap now gates new bands only.

Next: T-140 vault spending integrates the pledge vault (Phase P last).
