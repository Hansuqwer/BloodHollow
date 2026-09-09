# T-ART-06 remainder — NPC kinds 67/70–73 (engine side; art placement parked)

## Context
T-ART-06 partial since S26: 68 Confessor + 69 Fence shipped; 67/70–73 reserved. NPC briefs (`docs/art/30-npcs-players.md` rows 2,6,7,8) name them; sheets/portraits/placement need art-side input. This card ships the engine side.

## Scope
- `shared/content/wirekind.h`: `kWireKindBonesmith=67`, `kWireKindGuardAshen=70`, `kWireKindGuardSynod=71`, `kWireKindRegistrar=72`, `kWireKindSteward=73` (furniture floor 64 unchanged — all classify furniture, never mob).
- `server/src/world.h`: generic `debugSpawnFurniture(kind,name,at)` + 5 named seams (mirror 68/69 shape). No live spawner touched.
- `client/src/game.cpp`: stale furniture-band comment corrected (64/65 → 64–73); the generic rect+label branch already renders every new kind — no draw change needed.
- No wire change (stays 237 — content constants, zero messages), no epoch bump (stays 12 — test seams only), no map change.

## Acceptance criteria
- [x] All 5 kinds (+68/69 regression) spawn with right kind+name via seams.
- [x] All classify furniture / non-mob; hp 1/1; spawn bestows no karma.
- [x] Karma-refusal lane untouched (keys off vendor/fence proximity, not kind counts).
- [x] Suite green; replay clean.

## Tests required
- `tests/test_npcs.cpp`: sweep (7 spawns, kind/name/furniture/non-mob/hp pins, floor bounds) + karma-neutral spawn pin. Suite 165→**167**, 328,566 assertions.

## Out of scope (art-side follow-ups, needs input first)
- Thornwall/Marrowgate placement, idle 4f×8-dir sheets, dialogue portraits, EK-ledger/pledge logic (registrar), guard patrol/AI (guards are furniture posts until a behavior card).

## Evidence
- Suite: **167/167 (328,566 assertions)**, ctest 2/2, warning-free.
- Wire 237 untouched (`messages_gen.h`); replay `logs/t084.bwj` OK 6001/718/241 mm=0.
- Devlog: `docs/devlog/0056-npc-kinds.md`.
