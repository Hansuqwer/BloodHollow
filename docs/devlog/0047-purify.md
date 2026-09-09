# 0047 — Cultist Purify: the field cleanse (T-082, S35)

S35 of the extended queue. GDD §6 kit-list content, fully derived pins.

## Scope as pinned

- Channel 9 (first free; 1–8 taken): `trySkill` gate widened 8→9,
  `chUnlock` grown to 10 in all three kit rows, `kitSkillUnlock` guard
  9→10. No command/main changes — `SkillUse` already carries any byte,
  and the client has no keys for 6–8 either (verified precedent: bots send
  raw channels, which is also how Purify is exercised live).
- Cultist unlock **6** (utility-tier parity with Ironskin — flagged).
  Gates mirror Mend exactly: 8 MP, 25t CD (`lastPurifyTick` stamp), 6
  tiles, self-or-party via `choirTarget` — stated, not designed. Effect:
  clears `curseUntil` only (no heal, no bless touch); quiet fail on clean
  blood.
- No epoch bump (stays **12**): existing field, deterministic. Short soak
  + replay to be safe (below).

## Soak + replay

Fresh 540 s grinder mix (port 7893, fresh DB `/tmp/t082.db`, epoch-12
journal `logs/t082.bwj`), same 14-bot shape. No Cultist hit the curse
live (expected — curse is L14-crypt-adjacent); unit tests pin the rite:

| group | kills | deaths | maxLevel | notes |
|---|---|---|---|---|
| campaign ×2 | 69 | **0** | 4 | killerByLvl empty; mend=10 live |
| fighter ×4 | 84 | 46 | 4 | spread |
| pilgrim ×3 | 49 | 12 | 3 | quiet leg |
| wander ×5 | 0 | 56 | 1 | decoys |

Bands: Rat 4.9 / Bat 7.7 / Ghoul 12.8 / Hound 66.8 (n=12) / Gnoll 118.7
(n=4) — ordering preserved; mid-band wobble stays a standing watch item.
Entities ~345–356, p99 ~3.4–3.9 ms.

`./build/server/bh_server --replay-world logs/t082.bwj` →

`[replay] OK ticks=12801 sessionCmds=7391 hashes=129 mismatches=0 entities=356`

## Files

`shared/content/kits.h`, `server/src/{world.cpp,world.h}`,
`tests/{test_purify.cpp,CMakeLists.txt}`. Suite **160 / 328,489**,
ctest 2/2, warning-free.
