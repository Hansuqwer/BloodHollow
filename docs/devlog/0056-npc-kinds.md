# 0056 — Five NPCs walk onto the furniture band (T-ART-06)

T-ART-06 remainder, engine side. The art briefs already named all five (twins, two guards, registrar, steward); the engine just never reserved their chairs.

## What landed

- Five constants + one generic spawn seam + five one-line wrappers + a sweep test. The client needed zero draw changes — the S26 generic rect+label branch renders every furniture kind by construction, which is exactly why 68/69 "shipped" without anyone noticing. The stale comment saying otherwise is now fixed.
- The karma test is deliberately weak (spawn bestows no karma): it pins what this card guarantees — new furniture is inert. The refusal lane keys off vendor/fence *proximity*; five more names on the band cannot move it, and the test says so in prose rather than faking a refusal matrix.

## Judgment calls

- No live spawns: placing Ashen/Synod guards at the gates or the registrar in a nonexistent Marrowgate is content + art, not engine enablement. They spawn in tests today and in Thornwall the day art says where.
- Guards are furniture posts (hp 1, non-combat) like every other NPC — the L15 gate-guard mob (1011) is a different entity that already exists. If a behavior card ever arms these two, it starts from this seam.
- No epoch bump: test seams don't shift the sim under old journals (same class as the 68/69 seams before them).
