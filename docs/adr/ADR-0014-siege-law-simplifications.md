# ADR-0014 — Siege-law simplifications + Blood Moon lite (shipped law)

**Status**: accepted (T-158 truth-up, 2026-09-16 — records shipped law; GDD §8/§9 amended to match)

**Context**: GDD §8 specified 100k-HP gates felled by ×3 siege-damage skills, a 60 s crown channel, pledge creation at CHA ≥20 + 100,000g, and Blood Moon as [v0.2]. The tree shipped simpler numbers and the M4 gate (3/3 flips, devlog 0098) plus the pledge ceremony legs were proven on them.

**Decision (adopt shipped)**:
- Gates: 300 HP breach objectives (kind 75, zone 6), felled by `/breach` ram (10 dmg, ~30 actions). No siege-damage skills in MVP.
- Crown: 60 s uncontested presence attunement (`kHeartCaptureTicks=1200`) THEN a 10 s kneel channel (`kCrownChannelTicks=200`); re-kneel mid-channel is a no-op (server guard); interrupt restarts.
- Pledge creation: L ≥10 + 10,000g (`kPledgeMinLevel`, `kPledgeCreateGold`). CHA ≥20 returns with factions in v0.2 (T-160 owns the stat-model decision).
- Blood Moon: MVP ships the lite version — curse duration ×2 + night bite 130% (`nightBiteNum`), GM-raised session flag to next dawn. Full moon (spawn ×2, Pale Sow, +loot tier) stays [v0.2].
- Change-control note (05-mvp §1): the lite moon is funded by cutting purity/fodder tiers, 100k-gate skills, and the 60 s crown — equal-cost cuts, no scope growth.

**Consequences**: (+) M4 evidence stays valid; Friday-Night leg 3 runs on these numbers; (+) Friday scheduling (90-min window) actually fits humans; (−) 100k-gate fantasy builds are closed for MVP.

**Alternatives rejected**: revert to 100k gates/60 s crown (no bot wave could flip in a leg; M4 would need re-proof from zero).
