# ADR-0015 — Five-stat model (CHA cut for MVP)

**Status**: accepted (wave-2, T-160(B) 2026-09-16 — implements the staff recommendation in `T-WAVE2-EPOCH30.md`)

**Context**: GDD §3 specified six stats with CHA driving aura radius, pet slots, pledge creation (≥20) and vendor prices. The tree ships STR/VIT/DEX assignable + INT/MAG kit-seeded, no CHA anywhere; the pledge gate was already re-cut (L≥10+10k, world.h). T-160 asked for six stats or an ADR'd five.

**Decision (B)**: five stats. INT/MAG become assignable (stat 3/4, client F8/F9) since casters exist; aura radius stays hard-coded 12; pledge gate stays L≥10+10k; vendor prices stay flat; pet slots die with no-pets-ship. GDD §3 amended. No schema/wire change (OwnStats already carries intg/mag; StatAssign channel is u8).

**Consequences**: (+) zero migration risk (no stored CHA anywhere); (+) casters scale past creation; (−) CHA-flavored builds wait for factions v0.2 with the rest of the cut list.

**Alternatives rejected**: (A) six stats — schema + wire + UI + three economy hooks for one stat, all unproven, against an alpha clock.
