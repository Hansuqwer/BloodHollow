# 0080 — T-118 B1 weapon-skill persistence (schema v11, epoch 21)

## Summary
Sword skill (Soma ladder) was use-based but volatile: `swingLands` and `swordSkill` lived only in `Entity`, seeded via `--bless skill:XX`, and reset to 0 on every relog because `Db::saveProgress` never persisted them. Track B1 fixes the Sisyphus ladder with additive-only schema v11.

## What changed
- **persist.h**: `CharacterRow` gains `swordSkill int` and `swingLands int64` (v11).
- **persist.cpp**:
  - `open()`: migration `uv < 11` adds `sword_skill INTEGER NOT NULL DEFAULT 0` and `swing_lands INTEGER NOT NULL DEFAULT 0`, then `PRAGMA user_version=11`.
  - `loginOrCreate`: SELECT includes new columns, fills row.
  - `saveProgress`: signature extended to `(..., classId, swordSkill, swingLands)`, UPDATE includes new columns.
- **main.cpp**:
  - `kJournalEpoch` 20→21 (progression persistence changes sim semantics under old journals).
  - `journalLogin`: l-line v3 now `l tick idx name x y zone level xp str vit dex sp gold mercy karma swordSkill swingLands inv` (previously 16 fields, now 18).
  - `dropSession`: saves skill.
  - Login: restores `swordSkill`/`swingLands` from row (clamped, recompute skill from lands if needed), bless still overwrites for debug.
  - Replay: `QueuedLogin` gains skill/lands, l-line parser tries v3 (18 fields) then v2 (16) then legacy, `applyLogin` restores skill.
- **server/CMakeLists.txt**: `bh_server_world` now includes `persist.cpp` and links `bh_sqlite` so unit tests can test DB.
- **tests/CMakeLists.txt**: link `bh_sqlite`, add `test_weapon_skill_persist.cpp`.
- **tests/test_weapon_skill_persist.cpp**: 3 cases — v10→v11 migration defaults, round-trip save/load, old rows default 0.
- **Gate leg**: `tools/t118_weapon_skill_leg.sh` — single server run epoch 21, 5 bots fighter 60s (gain skill), same 5 bots wander 10s relog (journal second wave carries skill). DB after wave1: skill 1 lands 29-35. Journal second wave l-lines show skill 1 lands 29-35. Replay mm=0.

## Evidence
- Suite: 209/209 (was 206), 329052 assertions (was 329022), ctest bh_tests PASS.
- Duel: pin unchanged (b273be661b54673a) — no combat math changed, only persistence.
- Replay:
  - Old `logs/t115.bwj` epoch 20 vs build 21 → refused exit 4 (epoch guard, expected).
  - Fresh `logs/t118.bwj` epoch 21, ticks 1500, cmds 529, hashes 15, mismatches 0, entities 178, replay OK.
- DB persistence:
  - After wave1: `('t118__00', 2, 1, 33)` etc — skill persists.
  - After relog: journal `l 1282 5 t118__00 ... 1 33 ...` — skill 1 lands 33 in l-line.
  - Second wave login restores skill, worldHash includes skill (already in T-107).
- Schema: additive-only ALTER TABLE, defaults 0, no table rewrite.

## Deviations
- None — additive schema, epoch bump stated, no new third-party dep.

## Risks
- Old journals (epoch 20) refused by guard — correct per epoch discipline, old legs retained as history.
- `saveProgress` signature change touches one call site (dropSession) — verified.

## Next
- B2 pledge, B3 siege, etc. Skill now persists for anvil gates (20/50/80/120/150) and effDmgBase (+1 per 20 skill).
