# ADR-0013 — Enhancement/economy shipped law (refine odds, shatter, no purity/fodder)

**Status**: accepted (T-158 truth-up, 2026-09-16 — records shipped law; GDD §7 amended to match)

**Context**: GDD §7 specified refine odds 100/90/80/65/50/35/25, failure −1 level (+4..+6) and reset to +0 (+7), ore purity 1–10 (±%) and fodder tiers (±%). The tree shipped different numbers (`server/src/world.cpp` `kRefineChance`, T-079 frozen rows) and the M2/M3 economy legs plus the T-159 era table were measured against the shipped numbers.

**Decision (adopt shipped)**:
- Odds per target tier: +1 100 / +2 100 (mercy) / +3 60 (+destruction) / +4 65 / +5 50 / +6 35 / +7 25 (`kRefineChance = {100,100,60,65,50,35,25}`).
- Failure at refine 2 SHATTERS the item (destroyed); other failures slip one temper (−1); +7 failure resets to +0. Toll: one junk + 50g per attempt; gear-only; refine feeds weapon dmg (+2/tier) and armor def (+1/tier).
- Ore purity and fodder tiers are NOT implemented: Blackiron Ore is flat + 50g toll. They stay [LATER] with +8..+10 scrolls.

**Consequences**: (+) M2/M3 gate evidence stays valid; (+) shatter at +2 keeps the 2→3 coin meaningful without mass-quit risk; (−) GDD-spec'd purity/fodder builds are closed — reopen only via change control (05-mvp §1).

**Alternatives rejected**: revert to GDD odds (invalidates all refine economy evidence + T-159 era table for zero player-visible gain).
