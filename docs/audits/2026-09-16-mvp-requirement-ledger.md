# MVP requirement ledger — 2026-09-16 (HEAD `2bd471d`, epoch 27, schema v14, protocol 237)

Flat, greppable companion to
[`2026-09-16-mvp-playable-audit.md`](2026-09-16-mvp-playable-audit.md).
One row per requirement id. `probe` = what was run/read · `observed` = the
decisive output · verdict vocabulary per the audit prompt §0.4.

Verdicts used: `SHIPPED` · `SHIPPED-INVISIBLE` · `PARTIAL` · `MISSING` ·
`STALE-DOC` · `DEFERRED-BY-DECISION` · `UNVERIFIED` · `SCOPE-CREEP`.

---

## A — build / boot / clean machine

| id | requirement | probe | observed | verdict | sev | owner | effort | card |
|---|---|---|---|---|---|---|---|---|
| A1 | headless preset builds everything incl. tests | `cmake -S . -B build/headless -DBH_BUILD_CLIENT=OFF && cmake --build build/headless` | server/bots/mapconv/duel OK; `bh_tests` → `engine/render/camera_rig.h:3: fatal error: raylib.h` (test_zoom.cpp); no `BH_BUILD_CLIENT` guard in `tests/CMakeLists.txt` | **PARTIAL** | P1 | agent | S | T-154 |
| A2 | Linux graphical build | local attempt; CI run 35059645385 | local blocked (no X11/GL headers, apt unreachable, `bootstrap.sh` fails: `Unable to locate package libwayland-dev wayland-protocols pkg-config`); CI ubuntu green 3m44s | UNVERIFIED (local) / SHIPPED (CI) | P2 | agent | S | T-168 |
| A3 | macOS arm64 build | CI job 104677050859 | green in 50 s | SHIPPED (CI-proxied) | — | — | — | — |
| A4 | suite green + count | static count; CI ctest | 303 `TEST_CASE` macros / 59 files; 58 compiled; `test_siege_stub.cpp` uncompiled; CI ctest green both OSes | PARTIAL (local UNVERIFIED) | P2 | agent | S | T-154 |
| A5 | 6 `.bhmap` artifacts | `cmake --build build/headless --target bh_maps; ls assets/maps` | 6/6 (walkable 83.9/86.7/42.9/26.0/31.1/86.6 %) | SHIPPED | — | — | — | — |
| A6 | all 6 mapgens deterministic | regenerate each + `diff` vs `data/maps-src` | **6/6 byte-identical**; but 4 generators ignore `--out` and write in place | SHIPPED + tooling gap | P2 | agent | S | T-168 |
| A7 | mapconv validates all 6 | `bh_mapconv … --validate` (inside `bh_maps`) | 6/6 clean | SHIPPED | — | — | — | — |
| A8 | portal/link integrity | `python3 tools/mapgen/validate_links.py` | `0 problems across 6 maps` | SHIPPED | — | — | — | — |
| A9 | server boots all zones | `bh_server --db /tmp/audit_boot.db --port 7841 --soak-secs 45` | zones 2–6 `online`, `[journal] … (epoch 27)`, auth posture printed | SHIPPED | — | — | — | — |
| A10 | bots connect | `bh_bots --count 5 --secs 12 --profile wander` | 5/5 online, `rc=0`, `[bots] OK` | SHIPPED | — | — | — | — |
| A11 | record→replay clean | `--record-world` then `--replay-world` | `ticks=901 sessionCmds=17 hashes=10 mismatches=0 entities=210` | SHIPPED | — | — | — | — |
| A12 | epoch guard refuses old legs | replay `t140`(26) `t138`(26) `t139`(26) `t122`(22) `t107`(18) | all **exit 4** + correct message | SHIPPED | — | — | — | — |
| A13 | substantive epoch-27 leg of record | `--replay-world logs/t146.bwj` | `ticks=242 cmds=16 hashes=2 mm=0` — 5 wanderers × 10 s only | **PARTIAL** (thin) | P1 | agent | M | T-157 |
| A14/A15 | client offline + online modes | requires display | not runnable here | UNVERIFIED (no display) | P1 | director | S | RUN-FIRST #3 |
| A16 | clean-machine path is one law | `grep -n "build/" tools/bootstrap.sh tools/t146_merge_repair_leg.sh README.md` | 3 conflicting build dirs; `assets/maps/` absent pre-build | PARTIAL | P1 | agent | S | T-168 (+T-149) |
| A17 | bootstrap works as written | `bash tools/bootstrap.sh` (read + partial run) | `apt-get download` list fails; `pip install cmake` needs `--break-system-packages` | **MISSING (broken)** | P2 | agent | S | T-168 |
| A18 | CI gates the risky paths | `.github/workflows/ci.yml` | no replay leg, no soak, no headless preset, no client/xvfb visual check, 1/6 mapgens, 1/6 mapconv validates | PARTIAL | P2 | agent | M | T-157 |

## B — MVP §1 IN rows

| id | requirement | probe | observed | verdict | sev | owner | effort | card |
|---|---|---|---|---|---|---|---|---|
| B1.1 | 6 zones in one process | boot log | zones 1–6 online | SHIPPED | — | — | — | — |
| B1.2 | client renders all 6 | `sed -n '/mapFileFor/,/^}/p' client/src/game.cpp` | cases 2,3,4,5 + `default: thornwall`; **no case 6** (the two `case 6:` hits are audio + terrainColor) | **PARTIAL → castle invisible** | **P0** | agent | S | T-150 |
| B1.3 | portals work for humans | code + `validate_links` | links valid; human walk UNVERIFIED (no display) | UNVERIFIED | P2 | director | S | RUN-FIRST #4 |
| B1.4 | map set matches MVP row | `ls data/maps-src` | thornwall, fields_overflow, thornwall_crypt, bonehowl_mine, drowned_crypt, weeping_castle | SHIPPED | — | — | — | — |
| B1.5 | Marrowgate as EK targets | `shared/content/towns.h`, `grep -rn kTownMarrowgate server/src` | identity + oath + 1 guard kind (71); **no Marrowgate population** | PARTIAL | P1 | agent | M | T-163 |
| B2.1 | 3 kits selectable in-game | `kits.h`; `grep -n '"/kit ' server/src/main.cpp` | `/kit <name>` chat verb only; no client UI | PARTIAL | P1 | agent | M | T-167 |
| B2.2 | class at creation | `persist.cpp` login path; `grep -rni "\bsex\b" server/src shared client/src` | `class_id` defaults 1; **no sex field anywhere** | MISSING | P1 | agent | M | T-167 |
| B2.3 | kit skills per GDD §3 | `world.h:251-258`, `kits.h chUnlock[10]` | 9 channels (PowerSwing, Mend, Bless, Ironskin, Firebolt, Chorus, MassMend, Haste, Purify) vs ~23 GDD skills; **no Resurrect/Sanctuary/RaiseSkeleton/CorpseExplosion/Terror/ManaShield/FrostSpike/Wither/CurseOfWeakness/Sunder/BullRush/WarStomp/Executioner/SecondWind** | **PARTIAL** | P1 | agent | L | T-161 |
| B2.4 | cap 25 + XP curve | `shared/sim/combat.h:21,26` | `kLevelCap = 25`, `xpNext` capped | SHIPPED | — | — | — | — |
| B2.5 | 6 stats, 3 pts/level | `world.cpp:338-344`; `grep -rniE "\bcha\b\|charisma" server/src shared client/src` | assignable = STR/VIT/DEX only; INT/MAG kit-seeded; **CHA absent** (`world.cpp:2994` comment admits it) | **PARTIAL** | P1 | agent+director | M | T-160 |
| B2.6 | no respec | grep | none shipped | SHIPPED (OUT respected) | — | — | — | — |
| B2.7 | weapon mastery + auras | `auras.h` (5 tiers), `sword_skill`/`swing_lands` columns | auras I–V live; sword family only; claim path via anvil/Bonesmith proxy | PARTIAL | P2 | agent | M | T-158 |
| B3.1 | server-auth combat | `command.h` dispatch, `trySwing` | all resolution server-side | SHIPPED | — | — | — | — |
| B3.2 | hit/crit/stagger math | `shared/sim/combat.h`, `tests/test_combat.cpp` (24 cases) | present + tested | SHIPPED | — | — | — | — |
| B3.3 | deliberate cadence | tick constants + `tests/test_burst_stamp.cpp` | present | SHIPPED | — | — | — | — |
| B3.4 | potions | `useItem` by id, stackMax 16, `Q` sip | present | SHIPPED | — | — | — | — |
| B3.5 | gore decals persist | `engine/render/decals.h`, `tests/test_decals.cpp`, `00-VERIFY.md #21` | decal layer shipped (T-ART-09); 10-min blood persistence not confirmed | PARTIAL / UNVERIFIED | P2 | agent | S | T-158 |
| B3.6 | telegraphs legible | `tests/test_slam.cpp`; client anim state | sim telegraphs exist; **player attack/cast/hurt/die frames cannot play (no sheets)** | SHIPPED-INVISIBLE | P1 | agent | M | T-156 |
| B4.1 | XP debt + de-level | `tests/test_*`, `world.cpp` death path | shipped (T-025) | SHIPPED | — | — | — | — |
| B4.2 | tier multipliers ×8/×20/×100 | `mobs.h` xp column | 1010 elite 3200 vs 1008 base 820 (×3.9), 1009 boss 4000 vs 820 (×4.9), nameds 1200–4100 | **PARTIAL (drift)** | P2 | agent | S | T-158 |
| B4.3 | bounty board kill-N/pickup-N | `world.h:438-440`, `grep -rn pickup` | kill-N quarry, **session-scoped by design**, no pickup-N, not persisted | PARTIAL | P1 | agent | S | T-166 |
| B4.4 | named-elite timers + announce | `mobs.h` 1012-1014, T-101/102/103 cards | 3 nameds on timers; announce via ch-2 lines | SHIPPED (client text only) | P2 | — | — | — |
| B5.1 | party cap 5 + verbs | `tests/test_party.cpp`, `/invite /accept /leave /kick` | shipped | SHIPPED | — | — | — | — |
| B5.2 | XP share law | `world.cpp` party share (12-tile, +12 %/sharer) | shipped; heals counted | SHIPPED | — | — | — | — |
| B5.3 | M3 metric ≥1.4× | `tools/m3_*` legs | evidence predates epoch 27 | STALE | P1 | agent | M | T-157 |
| B5.4 | party frame UI | `game.cpp` PARTY panel (roster/leader/HP) | present | UNVERIFIED (no display) | P2 | director | S | RUN-FIRST #4 |
| B5.5 | no 8-man parties | grep | absent | SHIPPED (OUT respected) | — | — | — | — |
| B6.1 | refine cap +7 + GDD odds | `world.cpp:2114,2153` | cap +7 ✓; odds `{100,100,60,65,50,35,25}` vs GDD `{100,90,80,65,50,35,25}` | **PARTIAL (drift)** | P1 | agent+director | S | T-158 |
| B6.2 | failure law | `world.cpp:2162-2173` | `refine==2` fail → **SHATTER** (not in GDD §7); `refine==6` fail → reset 0 ✓; else −1 ✓ | **DRIFT, no ADR** | P1 | director | S | T-158 |
| B6.3 | ore purity + fodder tiers | `grep -rn "purity\|fodder\|510[123]" server/src shared/content tests` | **0 hits**; refine eats a monster part + 50 g | **MISSING** | P1 | agent+director | M | T-158/T-159 |
| B6.4 | ore mining | `world.cpp:2014,2047`, `/mine`, kind 74, `tests/test_mine.cpp` | shipped (nodes zone 4, pick required) | SHIPPED | — | — | — | — |
| B6.5 | durability + repair | `InvSlot.durability`, `/repair`, `ItemSlot.durability` on wire | shipped; client draws durability in the bag panel | SHIPPED | — | — | — | — |
| B6.6 | glow from +5 | `glowTier` (0/1 +5..9/2 +10+), `refine_glow.h` | tier 1 reachable; **tier 2 unreachable at a +7 cap** | PARTIAL (dead tier) | P3 | agent | S | T-158 |
| B6.7 | anvil panel states real odds | `game.cpp` anvil panel (T-048) | panel exists; odds/shatter disclosure UNVERIFIED (no display) | UNVERIFIED | P1 | director | S | RUN-FIRST #4 |
| B7.1 | 4 rarity tiers | `grep -rn "rarity\|Rarity" server shared client tests tools` | **0 hits** | **MISSING** | P1 | agent | L | T-159 |
| B7.2 | ~40 affixes with live effects | `items.h:70 kAffixCount`, `tests/test_affix_v2.cpp` | **10** affixes, all with effect hooks | PARTIAL | P1 | agent | M | T-159 |
| B7.3 | ~12 uniques | `items.h kUniqueDrops` | **12 rows** over 1009/1012/1013/1014, titles + slot-law pins | SHIPPED | — | — | — | — |
| B7.4 | 11 gear slots | `items.h:13` | **2** (weapon, armor) + consumable + junk | **PARTIAL** | P1 | agent | L | T-159 |
| B7.5 | item ladder for L1→25 | `items.h` row count | ~28 rows; 8 weapons / 6 armors across the whole career | PARTIAL | P2 | agent | M | T-159 |
| B7.6 | trade + audit log | `tests/test_trade_equipped.cpp`, `test_tradelog.cpp`, ADR-0011 | shipped, commit-time validation, `logs/trades.log` | SHIPPED | — | — | — | — |
| B7.7 | no dupe/launderer class | `parseInvBlob` single grammar, `test_inv_blob.cpp`, live relog probe | single parser; relog preserved level/gold/karma/class/skill/town/ek/pledge/inv (7-field blob) | SHIPPED | — | — | — | — |
| B8.1 | karma law | `karmaBandOf`, T-056/057, `tests/test_alignment.cpp` | shipped | SHIPPED | — | — | — | — |
| B8.2 | chaotic drop risk legible | red nameplates (karmaBand on wire), sheet stakes | shipped server+wire; visual UNVERIFIED | SHIPPED / UNVERIFIED | P2 | director | S | RUN-FIRST #4 |
| B8.3 | guards + wanted | mob 1011 `guard=1`, kinds 70/71, `tests/test_guards.cpp` | shipped | SHIPPED | — | — | — | — |
| B8.4 | duels | `/duel`, `/forfeit`, `bh_duel` harness | shipped | SHIPPED | — | — | — | — |
| B8.5 | chapel repentance | `/confess`, `/repent`, kind 68, `tests/test_repent.cpp` | shipped | SHIPPED | — | — | — | — |
| B8.6 | town-of-19 war reachable | `kTownOathLevel=19`, `/oath`, `town_id`/`ek` | oath + EK persist; **no enemy-town players/mobs to kill** → war unreachable in practice | **SHIPPED-INVISIBLE** | P1 | agent | M | T-163 |
| B8.7 | EK leaderboard surface | `grep -n '"gm ek"' server/src/main.cpp` | readout exists **only as a GM chat verb**; no player verb, no page | **SHIPPED-INVISIBLE** | P1 | agent | M | T-163 (+T-147) |
| B8.8 | Blood Curse / Blood Moon | `tests/test_curse.cpp`, `test_blood_moon.cpp`, `world.cpp:1523-1550` | curse ✓; moon = curse ×2 + night bite 130 %, **no spawn ×2 / Pale Sow / loot tier**; GDD lists it [v0.2] | **PARTIAL + SCOPE-CREEP** | P1 | director | M | T-158 + §7 C3 |
| B9.1 | 4 h clock + phase split | `sim/clock.h`, `tests/test_clock.cpp`, `Welcome.hourCenti` | shipped | SHIPPED | — | — | — | — |
| B9.2 | darkness tint | `daynight.cpp` peak α150, HUD untinted | shipped; world floaters tinted (known) | SHIPPED | P3 | — | — | — |
| B9.3 | light radius law | `grep -n light server/src/world.h`, `lightmask.h`, items 3003/3004, affix 8 | `light` on wire, torch/lantern items, **no additive light mask**; GDD §9 itself says radius shipped 0 | **PARTIAL / contradictory spec** | P1 | agent+director | M | T-164 |
| B9.4 | night-only spawns | `SpawnDef.nightOnly` (bhmap v2), `world.cpp:244,4112` | mechanism shipped; GDD's Wraiths/Bloodfiends are not in `kMobs` | PARTIAL | P2 | agent | M | T-162 |
| B9.5 | night economy + nightcreep | `tests/test_night.cpp`, ×1.15 dmg / +10 % XP / +25 % drops | shipped | SHIPPED | — | — | — | — |
| B10.1 | weekly 90-min window + registration | `world.h kSiege*`, `tests/test_siege_sched.cpp`, live rehearsal | window math correct (288 000 ticks/day = 4 h real; 108 000 = 90 real min; Sat 20:00); `/siege-reg` live-proven | SHIPPED | — | — | — | — |
| B10.2 | 2 gates, 100k HP, siege skills ×3 | `world.h:368 kSiegeGateHp`, live drill | **300 HP + `/breach` ram**; Outer + Inner breached live; no siege-damage skill class | **PARTIAL (spec drift)** | P1 | director | S | T-158 |
| B10.3 | heartstone → 60 s crown → flip | `world.h:373-374`, `world.cpp:1795-1940`, live drill | attune = 60 s uncontested ≤3 tiles; **crown = 10 s** (GDD 60 s); live drill: `attuned=0 crowned=0` in 210 s (defenders contested), bots sent 8 crown attempts | PARTIAL + evidence gap | P1 | agent | M | T-157/T-158 |
| B10.4 | taxes 0–15 % + holder buff | `tests/test_siege_taxes.cpp`, `siege_state` table | shipped + persisted (holder/vault/crowns); vault spend deferred (deposit-only) | SHIPPED (partial by decision) | P2 | — | — | — |
| B10.5 | bot-filled rehearsals | `--profile siege`, `--defenders`, live drill | 8 atk + 4 def choreography works; `reg=8 breach=240 crown=8`; p99 733 µs | SHIPPED | — | — | — | — |
| B10.6 | siege legible to humans | `grep -rniE "siege\|pledge" client/src` | **0 real hits** (5 false positives from "place**holder**"); no wire messages | **MISSING** | **P0** | agent | L | T-151 |
| B10.7 | castle map fit for 30 players | `weeping_castle.tmj` 40×30, walkable 86.6 %, 2 spawners, 1 portal | small but walkable; occupancy UNVERIFIED with humans | PARTIAL / UNVERIFIED | P2 | director | M | RUN-FIRST #5 |
| B11.1 | pledge creation gate | `world.h:295`, live leg | L≥10 + 10 000 g (GDD wants CHA≥20 + 100k) — deviation flagged in code, **no ADR** | PARTIAL (documented deviation) | P1 | director | S | T-158 |
| B11.2 | emblem visible | `grep -rn emblem client/src` | 0 hits; emblem stored (0–9) + persisted | **SHIPPED-INVISIBLE** | P1 | agent | M | T-151 |
| B11.3 | ranks + verbs + pledge chat | live leg: kinds 34–41, `/p `, `/pledge …` | create/invite/accept/leave/kick/rank/tithe all fired; ranks 3/2/1 restored after relog | SHIPPED | — | — | — | — |
| B11.4 | vault (tax-only) | `pledges.vault_gold = 500` after tithe | shipped + persisted; spend deferred by decision | SHIPPED (partial by decision) | — | — | — | — |
| B11.5 | pledge bands in siege | `tests/test_pledge_bands.cpp`, live drill `band enlisted (4 members, 1/8)` | shipped | SHIPPED | — | — | — | — |
| B11.6 | pledge persistence | schema v13/v14, live relog | `integrity ok`, membership restored | SHIPPED | — | — | — | — |
| B11.7 | no pledge XP/wars/storage | grep | absent | SHIPPED (OUT respected) | — | — | — | — |
| B12.1 | 4 chars/account + offline slots | `persist.cpp` `SELECT … WHERE account_id=? LIMIT 1` + single INSERT | **1 char per account**, no select screen, no offline slots | **MISSING** | P1 | director (cut?) | M | T-167 / §7 C1 |
| B12.2 | login + version refusal | `main.cpp:329`, `Hello.protoVersion` | reason=4 refusal shipped | SHIPPED | — | — | — | — |
| B12.3 | real password hashing | `persist.h` `stubPasswordHash` + comment | salted iterative **FNV-1a stub**; argon2id owed "before any public alpha wave" | **MISSING** | **P0** | agent+director | M | T-153 |
| B12.4 | rate limits + registration gate | boot log, `loginlimit.h`, `tests/test_loginlimit.cpp` | 60 logins + 30 new accounts/60 s/IP, 10 fails → 60 s lockout, `--no-register` | SHIPPED (default OPEN) | P2 | director | S | T-153 |
| B12.5 | name law | `persist.cpp` (reason=2 invalid name), `COLLATE NOCASE` unique | uniqueness + case-insensitivity shipped; charset/length rules UNVERIFIED | PARTIAL | P2 | agent | S | T-153 |
| B12.6 | reconnect / crash safety | SIGKILL probe + restart + replay | journal tail intact, restart 0 errors, pre-crash replay `mm=0` | SHIPPED | — | — | — | — |
| B13.1 | launcher + patcher | `grep -rn launcher` + runbook §6 | **absent**; director card **T-146** already filed | MISSING / DEFERRED (director) | P1 | director | L | T-146 |
| B13.2 | GM CLI: kick/ban/rollback/siege/announce | `grep -n '"gm ' server/src/main.cpp` | `gm blood-moon`, `gm siege-start`, `gm ek`, `gm siege` only; **no `/ban`, no announce, no kick, no GM authority**; `gm siege-now` cited by ops docs **does not exist** | **PARTIAL + P0 authority hole** | **P0** | agent | M | T-152 |
| B13.3 | backups + drilled restore | `tools/ops/bh_backup.sh`, T-143 evidence | shipped + drilled (accounts=5 chars=5 pledges=1 uv=14, integrity ok) | SHIPPED | — | — | — | — |
| B13.4 | systemd auto-restart | `tools/ops/bh-server.service` | unit shipped (static check only here) | SHIPPED (UNVERIFIED install) | P3 | director | S | T-149 |
| B13.5 | crash reporting | runbook §5 (journald + coredumpctl) | host-side only; **no path for remote players to report** | PARTIAL | P2 | director | M | T-146/T-147 |
| B13.6 | bot soak suite | `tools/bots` 8 profiles + 20 leg scripts | rich; **none wired into CI**; several hard-code `build/linux-gcc` | SHIPPED / PARTIAL | P2 | agent | M | T-157 |
| B13.7 | observability | `ServerStats`, `--soak-secs`, `--p99-budget-ms`, balancer feed | tick p99 + online + worldHash; **no time-series, alerting, or disk cron** | PARTIAL | P2 | director | M | T-152 |
| B14 | OUT-list compliance | greps per item | only **Blood Moon** shipped out of scope (partial); everything else absent | 1 SCOPE-CREEP | P2 | director | S | §7 C3 |

## C — GDD numeric delta

See the audit report §3.C table (18 rows). Rows needing an ADR or a GDD edit:
stats/CHA (C1), skills (C3), rarity (C8), affixes (C8), slots (C8), refine odds
(C9), shatter (C9), purity/fodder (C9), gate HP (C12), crown channel (C12),
pledge gate (C13), blood moon (C14), leash 18 vs 10–16 (C7), teleport sink (C11),
mob roster (C17), art budget (C16). → **T-158**.

## D — walkthrough (24 steps)

All 24 rows are `UNVERIFIED (no display)` for their *visual* half. Bot-proven
substitutes: step 9 party (party leg), 12 anvil/refine (pilgrim profile exists;
not exercised here because `--bless` overwrite gave no gear — see F6.3), 14 trade
(`test_trade*` + audit log), 18 zone tour (siege drill crossed 1↔6 repeatedly),
20 pledge (full ceremony + relog), 21 siege (gates breached), 23 relog
persistence (two probes, all columns). Steps 1–8, 10–11, 13, 15–17, 19, 22, 24
need a human at a display → **RUN-FIRST #4** in the report.

Two steps were established without a display and are **P0/P1**:
- **D-key help**: 18 chat verbs + skills 2–5 + F5/F6/F7 + F/G/F1–F12 absent from
  the two help lines (`game.cpp:1586-1588`) → P0-5.
- **D-debug keys**: `H`/`N` hour offset and `F3`/`F4` overlays are player-reachable
  (`game.cpp:118-121`) → P2.

## E — client legibility

| id | requirement | probe | observed | verdict | sev | card |
|---|---|---|---|---|---|---|
| E1.1 | players drawn by class/sex | `atlasFor(0)`, `test_clientlaw` `mobSheetPaths(0)` | hero placeholder; no class/sex on the snapshot | MISSING | P1 | T-142 (PR #49) |
| E1.2 | mob sheets for all 14 mobs | `find assets/aigen/mobs -name sheet.png` (10) vs `kMobs` (14) | 1011–1014 have no sheets | PARTIAL | P1 | T-162 |
| E1.3 | NPC/furniture kinds 67–73 | `assets/aigen/npcs/*` (`atlas.draft.json`, some `sheet.png`) | procedural proxies shipped (T-ART-B5); AI plates partially packed | PARTIAL | P2 | T-162 |
| E1.4 | terrain art on screen | `game.cpp:728,749`; `find assets/aigen/terrain -name '*.png' \| wc -l` = 709 | flat diamonds + untextured prisms; **T-ART-12 never filed** | **MISSING** | **P0** | T-156 |
| E1.5 | 18 player sheets (D7) | `find assets/aigen/players -name sheet.png` = 2 | ravager m/f only (+40 raw plates); T-141 R-LUMA FAIL, PR #50 provisional accept | PARTIAL | P1 | T-142 stack |
| E1.6 | ~25 VFX | `find assets/aigen/vfx -name '*.png'` = **0** | callouts + decals + glow only; `loadAtlas` rejects `dirs:1` (`atlas.cpp:29`) → **T-ART-14 never filed** | MISSING | P1 | T-156 |
| E1.7 | ~60 icons/widgets | `find assets/aigen/icons -name '*.png'` = **0** | text-only panels; **T-ART-15 never filed** | MISSING | P1 | T-156 |
| E1.8 | era font | `grep -rn "LoadFont\|DrawTextEx" client/src engine` = 0 | default raylib font; `callout_font.png/.fnt` designed, not shipped; **T-ART-13 never filed** | MISSING | P1 | T-156 |
| E2 | UI surface inventory | `DrawText` panel literals | HP/MP/XP, stats, kit + buff timers, karma, sword, gold, clock, chat, bag (+durability/affix/refine), vendor, fence, anvil, trade, party frame, nameplates, floaters. **Absent: siege, pledge, EK, bounty panel, target readout, settings, char select** | PARTIAL | P1 | T-151/T-163 |
| E3 | ~40 SFX + ambience | `synthkit.h` 7 voices, ~10 wired callouts | no footsteps/ambience/music/UI sounds; `BH_NO_AUDIO` safe | PARTIAL | P2 | T-156 |
| E4.1 | keybind discoverability | help lines vs server verbs | 2 help lines cover ~10 keys; 18 verbs + most hotkeys invisible | **PARTIAL** | **P0** | **T-169** |
| E4.2 | window scaling | `InitWindow(1024,768)`, zoom snap {1,1.5,2} ✓ | no resize/fullscreen path found | PARTIAL | P2 | T-156 |
| E5 | produced-but-unrenderable | 709 terrain + 43 player + 30 mob + 21 npc PNGs on disk | only mob/NPC/some-player art reaches the screen; terrain/VFX/icons/font gated on unfiled engine cards | **PARTIAL** | **P0** | T-156 |

## F — server / persistence / security / determinism

| id | requirement | probe | observed | verdict | sev | card |
|---|---|---|---|---|---|---|
| F1.1 | every client intent validated | `command.h` dispatch read-through | movement/range/CD/channel/MP/slot/price/stock/proximity/toll/rank/consent all server-side | SHIPPED | — | — |
| F1.2 | throw-free client-digit parse | `grep -rn "std::sto" server/src shared client/src tools/bots` | only `main.cpp:443,450,454` (the operator `--bless` parse) | SHIPPED (client paths) | P3 | T-155 |
| F1.3 | malformed-packet fuzz | 403 garbage datagrams (0–1400 B + ENet-ish absurd lengths) | server alive, 0 error lines; **no fuzz harness exists** | PARTIAL | P2 | T-157 |
| F2.1 | single mutation path | `applyWorldCommand` + `test_cmdparity.cpp` | shipped | SHIPPED | — | — |
| F2.2 | RNG law | `grep -rnE "rand\(\)\|random_device\|srand\|mt19937"` | 0 hits in server/shared/client | SHIPPED | — | — |
| F2.3 | no wall-clock in sim | `grep -rnE "steady_clock\|system_clock\|time\(nullptr\)"` | only shell pacing, limiter, telemetry | SHIPPED | — | — |
| F2.4 | worldHash coverage | `tests/test_worldhash.cpp` | economy/progression covered (T-107); pledge + siege-session state outside by design | SHIPPED (documented blind spots) | P3 | — |
| F2.5 | epoch discipline | A11–A13 live | `mm=0` fresh, exit 4 on 4 old legs | SHIPPED | — | — |
| F2.6 | deque-erase discipline | `test_deque_dangling.cpp`, T-111 fixes | shipped | SHIPPED | — | — |
| F3.1 | schema ladder v1→v14 | empty-file open probe | `user_version=14`, `integrity_check=ok` | SHIPPED | — | — |
| F3.2 | full row round-trip | live relog + column dump | level/xp/gold/karma/class/sword/town/ek/pledge/rank/map/x/y/inv all preserved | SHIPPED | — | — |
| F3.3 | one inv-blob grammar | `parseInvBlob` (`world.cpp:599`) | single shared parser, 7 fields | SHIPPED | — | — |
| F3.4 | WAL under load | 30-bot soak + crash probe | no errors; RSS not measured (short runs) | PARTIAL | P2 | T-157 |
| F3.5 | siege/pledge persistence | `siege_state`, `pledges` tables | holder/vault/crowns + membership persist; bands/registration/quarry/wanted are session-scoped by design | SHIPPED (documented) | P2 | — |
| F3.6 | audit trails | `logs/trades.log` (T-080) | trades logged; **no item/gold audit queries, no `--bless` audit** | PARTIAL | P2 | T-152 |
| F4.1 | protocol spec matches wire | `messages.md` (37 msgs, last 116) vs shipped systems | siege/pledge/EK/mine have **no messages**; post-party fields ride old messages | **STALE-DOC + MISSING** | P1 | T-151 |
| F4.2 | explicit protocol version | `protogen.py:82` | `kProtocolVersion = 200 + len(messages)` (implicit) | PARTIAL | P2 | T-151 |
| F4.3 | no committed generated code | `shared/protocol/gen/` vs `${CMAKE_BINARY_DIR}/generated` | committed copy unused | STALE artifact | P3 | T-165 |
| F4.4 | ENet channels + interpolation | code read (120 ms interpolation, reliable/unreliable split) | shipped; behaviour under artificial lag UNVERIFIED | UNVERIFIED | P2 | RUN-FIRST #6 |
| F5.1 | alloc/SoA debt | AGENTS.md truth-up + `wc -c` | unchanged (AoS deque, per-tick alloc); `world.cpp` 176 KB | DEFERRED-BY-DECISION | P2 | — |
| F5.3 | tick budget | 4 live runs | p99 128–733 µs vs 10/25 ms budgets | SHIPPED | — | — |
| F5.4 | M5 long soak | T-144 (60×10 min, epoch 26) | 200×12 h not run; needs a host + wall time | MISSING (director) | P1 | director |
| **F6.1** | **GM authority** | `grep -rniE "isGm\|gmName\|operator\|admin\|BH_GM" server/src` = 0; `main.cpp:520,526,635,638` | **any player** can `gm blood-moon` (until dawn: bite 130 %, curses ×2) and `gm siege-start`; `gm ek`/`gm siege` readouts open to all | **MISSING** | **P0** | T-152 |
| F6.2 | moderation + registration posture | runbook §2/§5 | no `/ban`, no announce, registration OPEN by default | MISSING | P0/P1 | T-152/T-153 |
| F6.3 | `--bless` discipline | 7 flags → 1 applied; usage string; runbook | `s.bless` map overwrites per name; undocumented; unaudited; `std::stoul` throws on bad operator input | PARTIAL | P2 | T-155 |
| **F6.4** | **`--bless` replay-exact** | A/B isolation, same waves/seeds | with bless: `[replay] FAIL ticks=1901 hashes=20 mismatches=10`; without: `OK … mismatches=0`. Live applies at Hello-handling, journals `s.tick+1` (`main.cpp:463`); replay applies from `queuedBlesses` in the login block (`main.cpp:1461`) | **MISSING (oracle trap)** | P1 | T-155 |
| F6.5 | asset licence manifest | `assets/LICENSES.md` vs disk | present + detailed; no machine check | PARTIAL | P2 | T-148 |
| F6.6 | no committed secrets | grep for credentials in `tools/ops`, configs | none found | SHIPPED | — | — |

## G — gate metrics

| gate | target | fresh at epoch 27? | observed | verdict | card |
|---|---|---|---|---|---|
| M1 | p99 <10 ms, 0 desyncs (20 bots × 30 min) | partial | 30 fighters × 60 s → `FINISH ticks=4001 p50=73 p99=172 µs`, `soak OK budget=10.00ms`; 5 wander × 12 s → p99 128 µs | PASS-signal (shape short) | T-157 |
| M2 | TTK 6–10 s, ~1.2 lvl/h L6–8 | no | last measured T-113/T-114 era | STALE | T-157 |
| M3 | party ≥1.4×; +5 EV ≈45–60 min | no | legs predate 27; **EV uncomputable without purity/fodder** | STALE + uncomputable | T-157/T-158 |
| M4 | p99 <25 ms, flips 3/3 (40 bots) | no | 12-bot 210 s drill: p99 733 µs ✓, `attuned=0 crowned=0` ✗; 3/3 claim is devlog 0098 @ epoch 25, 750 s, 12v6 | **NOT RE-PROVEN** | T-157 |
| M5 | 200 bots × 12 h, 60 fps MBA | no | T-144 = 60×10 min @ epoch 26 | MISSING (director) | director |
| §6 legs 1–4 | Friday-Night | legs 1–2 runnable; **leg 3 blocked** (P0-1/P0-2); leg 4 survey | readiness page pins epoch 26 and cites **`gm siege-now`, which no longer exists** | STALE-DOC + blocked | T-151/T-157 |
| §7 checklist | 7 boxes | 1.5/7 | launcher ✗ (T-146) · backup drill ✓ · runbook ✓-with-gaps · data-driven spawns ✓ / hot-reload ✗ · social+EK page ✗ (T-147) · legal ✗ (T-148) · clean boot ✗ (T-149) | PARTIAL | T-146..T-149 |

## H — hygiene

| id | finding | probe | verdict | sev | card |
|---|---|---|---|---|---|
| H1 | 8 open PRs; #49→#50→#51 is a 3-deep stack; #22/#23/#27/#28/#30 `mergeable=UNKNOWN` | `gh pr list/view` | triaged | P1 | T-165 |
| H2 | #22 pledge-lite and #30 affix-v2 content already in master | live pledge leg; `kAffixCount=10` + `test_affix_v2.cpp` compiled | CLOSE-SUPERSEDED | P2 | T-165 |
| H2b | **#23 carries `tests/test_castle.cpp` + `tools/t123_castle_leg.sh`, neither in master** | `gh pr view 23 --json files` vs `ls tests/` | cherry-pick before closing | P1 | T-165 |
| H3 | history = 1 commit | `git rev-list --count HEAD` | no blame/bisect | P2 | — |
| H4 | README §Status + board rows stale | README:120-130; `docs/tasks/` root vs `done/` | STALE-DOC | P3 | T-158 |
| H5 | `test_siege_stub.cpp` uncompiled | `tests/CMakeLists.txt` | dead file | P3 | T-165 |
| H6 | `.git` 101 MB / 2298 files | `du -sh .git` | no LFS decision | P3 | — |
| H7 | `logs/*.bwj` ignored but 118 tracked | `.gitignore:9`; `git ls-files 'logs/*.bwj' \| wc -l` | new legs need `git add -f` | P3 | T-158 |
| H8 | devlog id collisions across branches (0107/0108 on #49/#50 vs 0106 on master) | `gh pr view --json files` | this audit files **0109** | P3 | T-165 |
| H9 | AGENTS.md DoD contradicted by A1 + F6.4 | this ledger | STALE-DOC | P2 | T-154/T-155 |
| H10 | 7 shipped deviations with no ADR | §3.C | ADR backlog | P1 | T-158 |
| H11 | T-146..T-149 already filed elsewhere | `gh api …/docs/tasks?ref=task/human-only-mvp-cards` | audit cards start at **T-150** | info | — |
