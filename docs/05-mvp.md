# 05 — MVP Plan

**MVP = Closed Alpha (M5, target week of Jun 1, 2027).** The MVP is not "some
systems" — it's the smallest game where the *genre fantasy* is fully present:
**grind together, gamble your gear, fear the red-names, fight for the castle on
Saturday.** Anything that doesn't serve one of those four verbs gets cut.

## 1. Scope — IN / OUT

### IN (MVP)

| Area | Exactly this much |
|---|---|
| World | 4 outdoor/dungeon maps + castle map (Thornwall, Churchyard & Bleak Fields, Bonehowl Mine, Drowned Crypt, Weeping Castle) |
| Classes | Ravager, Gravecaller, Cultist — kits per GDD §3, level cap 25, 3 pts/level |
| Combat | Server-auth 20 Hz, hit/crit/stagger, potions, gore decals |
| Progression | XP debt + de-level, mob/elite/named/boss tiers, bounty board |
| Party | 5-man, XP bonus curve, contribution share (heals count), party frames |
| The Anvil | Refine +0..+7 with table GDD §7; ore mining; durability/repair |
| Loot | 4 rarity tiers, ~40 affixes, ~12 boss/named uniques, player trade |
| PK | Karma/alignment, chaotic drops, guards, duels, town-of-19 war + EK board |
| Day/night | Darkness+light radius, night spawns, Blood Curse, Blood Moon |
| Siege | Weekly 90-min Weeping Castle siege (gates → Heartstone → crown channel), taxes, holder buff; bot-filled rehearsals |
| Pledges | Lite: create/emblem/ranks/chat/vault(tax only) |
| Account | 4 chars/account, login, offline char slots |
| Ops | Launcher+patcher, GM CLI, backups, crash reporting, bot soak suite |

### OUT (not before alpha+; see GDD §10/§12)

Vampire race · pledge XP/wars/storage · +8..+10 scrolls · Blood Bibles · Crusade
nation-war · second town/castle playable politics (Marrowgate exists as red-named
EK targets in the field war & roadmap, not a full second city simulation) ·
stalls/auction · usage-based weapon %-skills · crafting professions beyond
mining/refine · quests-as-content · 8-man parties · any platform but Linux/macOS ·
localization.

**Change control:** adding anything to IN requires cutting an IN item of equal cost
+ an ADR. Signed, future-you.

## 2. Sprint plan (2-week sprints; agents work task cards, human checks gates)

| Sprint | Phase | Theme & gate |
|---|---|---|
| S1 (Sep 7) | P0 | Repo/CI/toolchain; iso renderer walking sim begins |
| S2 (Sep 21) | P0 | mapconv + placeholder map; A* + sim seeds; **gate M0** |
| S3 (Oct 5) | P1 | ENet + protocol v0 + login; entity store + spatial hash |
| S4 (Oct 19) | P1 | AoI deltas, movement auth, chat; bots v1; **gate M1 (Nov 8)** |
| S5 (Nov 9) | P2 | Stats/XP/levels; melee resolve; first 3 mobs + spawners |
| S6 (Nov 23) | P2 | Inventory/equip/loot; death/XP-debt; vendors; skills v1 |
| S7 (Dec 7) | P2 | Fields content to L12; trade window; bots v2 + balancer; **gate M2 (Dec 20)** |
| — | Dec 21–Jan 3 | **dark weeks**: devlog only, backlog grooming |
| S8 (Jan 4) | P3 | Party system + frames UI; Cultist buff engine (auras, Chorus) |
| S9 (Jan 18) | P3 | Alignment/PK/guards/duels/EK board; Blood Curse + chapel |
| S10 (Feb 1) | P3 | The Anvil + mining + durability; affix loot tables |
| S11 (Feb 15) | P3 | Mine + Crypt + Gravemother; nameds; VFX/audio pass 1; **gate M3 (Feb 21)** |
| S12 (Mar 1) | P4 | Pledge-lite; castle map; siege scheduler + registration |
| S13 (Mar 15) | P4 | Siege battle logic; taxes/vault/holder buff; siege bots |
| S14 (Mar 29) | P4 | Blood Moon; siege rehearsals ×3; anti-grief pass; **gate M4 (Apr 4)** |
| S15 (Apr 12) | P5 | Launcher/patcher; crash reporter; rate limits + fuzz pass |
| S16 (Apr 26) | P5 | Persistence hardening; backup drill; 200-bot soak gate |
| S17 (May 10) | P5 | UI final skin; SFX/content finish; alpha playbook |
| S18 (May 24) | P5 | Bug-fix only (feature freeze); invite wave 1; **gate M5 (Jun 1)** |

### First task cards (ready to file Monday, S1)

- `T-001` repo+CMake+CI matrix+doctest (AC: green pipeline both OSes)
- `T-002` raylib window/input + fixed interpolator harness
- `T-003` iso renderer: tile blit + painter sort + debug grid (AC: 10k-tile scroll @60fps)
- `T-004` atlas/anim loader + placeholder hero 8-dir walk
- `T-005` Tiled→bhmap converter v1 (+CI map validation)
- `T-006` sim: grid/blockers/A* + unit tests incl. 1000-random-paths property test
- `T-007` camera rig (follow/drag/edge) + zoom steps
- `T-008` day/night tint ramp v1
- `T-009` replay journal v0 (input log + state hash/tick)
- `T-010` Thornwall placeholder map pass 1 (human, in Tiled)
- `T-011` ADR-001..008 write-up review with human
- `T-012` M0 demo script automation (headless screenshot + checklist)

## 3. Definition of Done — per sprint

CI green (both OS) · new sim logic has tests · determinism replay green · a GIF in
the devlog · GDD numbers updated if changed · human playtested ≥30 min and filed
polish cards.

## 4. Key metrics (what "working" means at each gate)

| Gate | Metric | Target |
|---|---|---|
| M1 | 20-bot 30-min soak: tick p99 / desyncs | <10 ms / 0 |
| M2 | Solo TTK vs equal mob; XP/hr at L6–8 | 6–10 s; ~1.2 levels/h |
| M3 | 5-man vs solo XP/hr party advantage | ≥1.4× total |
| M3 | Refine EV: expected ore+gold cost of a +5 | ≈ cost of 45–60 min of farming |
| M4 | Siege sim (40 bots): tick p99, capture flip reliability | <25 ms, 3/3 flips |
| M5 | 200-bot 12h soak; client min-spec fps | no leaks/crashes; 60 fps 2018 MBA |
| Alpha | D1/D7 retention of invited wave; median session | ≥60%/≥30%; ≥45 min (genre norm) |

## 5. Budget & ops costs (alpha)

| Item | Cost |
|---|---|
| VPS (4 vCPU/8GB, Hetzner-class) | ~€12–18/mo |
| Domain + static site/EK board | ~€15/yr |
| Apple developer (notarization, wave 2) | $99/yr |
| Emergency pixel-art budget (boss + UI splash) | ~€300–600 reserve |
| Total to alpha | **< €300 committed** |

## 6. The Friday-Night Test (MVP acceptance scenario)

Scripted with real players, wave 2:

1. Two 5-man parties (invite friends; bots backfill to 20 online) race to level 12
   in 3 hours. **Pass if** the Cultist players are fought over in chat.
2. A named-elite timer overlap causes a 3-way fight at The Red Widow. **Pass if**
   someone goes red and loses an item, and it becomes a story.
3. Impromptu siege rehearsal (`gm siege-now`): 20 attackers vs 10 defenders + bots.
   **Pass if** ownership flips at least once and the run is screenshot-worthy.
4. Survey: "would you come back for Saturday's real siege?" **Pass at ≥70% yes.**

## 7. Alpha launch checklist

- [ ] Launcher updates client against manifest; wrong-version login refused
- [ ] `bh-server` systemd auto-restart; disk-space/alert cron; backup restore *drilled*
- [ ] GM runbook: kick/ban/rollback/siege/announce; audit log reviewed weekly
- [ ] Spawn tables & boss timers seeded from data (not code); hot-reload works
- [ ] Social: Discord, devlog RSS, EK board page; siege countdown visible on site
- [ ] Legal-lite: name/trademark sanity check on "BLOODHOLLOW"; asset-license
      manifest for every placeholder/AIGen asset (`assets/LICENSES.md`)
- [ ] The game boots from a clean machine install on Linux **and** macOS by a person
      who is not you

## 8. Risk register (top 8)

| # | Risk | P | Impact | Mitigation (owner: you + agents) |
|---|---|---|---|---|
| R1 | Netcode perf/desync discovered late | M | High | Online-first (P1), bots before combat, replay oracle, perf gates each gate |
| R2 | Art volume explosion (mob frames!) | **H** | High | Budget caps enforced in CI (≤100 frames/mob), palette swaps, dark palettes hide reuse, AI-gen + cleanup pipeline, cash reserve for boss UI art |
| R3 | Scope creep toward "real MMO" | **H** | High | MoSCoW wall (§1), change-control rule, roadmap outward-visible |
| R4 | Part-time burnout (~9-month haul) | M | High | Playable demo every 2 weeks, public devlog, dark weeks over holidays, zero-crunch rule |
| R5 | Party/support math lands wrong (solo meta) | M | Med | Nightly balance Monte Carlo from S7; pillar rule: buff party bonus, not nerf solo |
| R6 | Cheaters/dupe at alpha | M | High | Server-auth everything, transactional trades, item audit queries, ban playbook, no client trust ever |
| R7 | macOS friction (signing/notarize) | M | Low-Med | Unsigned-app instructions at wave 1; $99 notarization at wave 2; CI builds both archs from day 1 |
| R8 | Empty-world problem at alpha | M | Med | Bot population ("vagrant souls" flavor), scheduled events concentrate presence, invite waves sized to one town |
