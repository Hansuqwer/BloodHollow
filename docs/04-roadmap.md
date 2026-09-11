# 04 — Roadmap (part-time calibrated)

> **⚠ SUPERSEDED CALENDAR (T-110 truth-up, 2026-09-11).** This plan's dates are
> ~5 months behind shipped reality and are maintained no more: Phase 0 was slated
> Sep 7 – Oct 4 2026 and Phase 3 for Jan–Feb **2027**, but Phase-3 content
> (parties, kits, anvil, trades, PK/karma, zones 2–5, bosses through T-103) had
> shipped by **2026-09-10**. Actual velocity ≈ 5× the part-time calibration.
> Treat this document as the historical planning artifact and the *scope*
> definition (what belongs in which phase); for current status use
> `docs/tasks/` (board) + `docs/devlog/` (per-card evidence) + README §Status.

**Pace assumptions:** director ~12 h/wk (4h agent direction/review · 4h
integration/playtest · 4h design/devlog); agent throughput high but human review is
the bottleneck; roadmap dates use **calendar weeks** at that pace. If pace doubles,
compress phases ×0.6, not scope ×2. Start: **Mon 2026-09-07**. Every phase ends in a
**playable milestone demo** — that rule is what keeps a 9-month part-time project alive.

## Phase overview

| # | Phase | Weeks | Calendar | Milestone demo |
|---|---|---|---|---|
| 0 | Foundations | 4 | Sep 7 – Oct 4, 2026 | **M0 "Empty World"** — walk an iso map offline |
| 1 | Netcore | 5 | Oct 5 – Nov 8, 2026 | **M1 "Ghost Town Online"** — real players + bots share one zone |
| 2 | Combat & Progression | 6 | Nov 9 – Dec 20, 2026 | **M2 "Blood on the Fields"** — grind, level, die, loot |
| 3 | The Loop | 7 | Jan 4 – Feb 21, 2027 | **M3 "The Grind Is Real"** — parties, buffs, anvil, PK |
| 4 | Pledges & Siege | 6 | Feb 22 – Apr 4, 2027 | **M4 "Siege Night"** — 40-bot castle siege, 3 times a week |
| 5 | Hardening & Closed Alpha | 8 | Apr 5 – May 30, 2027 | **M5 — Closed Alpha invites (target: week of Jun 1, 2027)** |

Buffer: every phase has 1 implicit slack week before it counts as "slipping".
Holiday dead-time Dec 21 – Jan 3 is deliberately excluded.

---

## Phase 0 — Foundations (Sep 7 – Oct 4, 2026)

**Goal:** boring, solid skeleton. No game yet — engine + pipeline + CI discipline.

- Repo + CMake presets (linux gcc/clang, macOS arm64) + CI matrix + clang-format +
  doctest harness → **done week 1**.
- BloodEngine core: window/input, atlas sprite loader, **iso 2:1 renderer** with
  depth sort, camera (click-drag + edge pan), fixed interpolator.
- Tiled → `.bhmap` converter (tiles, blockers, zones, portals, spawner stubs).
- Placeholder asset pipeline: 1 test map (Thornwall square), 1 placeholder hero
  (LPC-style stand-in), day/night tint ramp stub.
- `sim/` seeds: grid, xoshiro RNG, tick loop, A* (with tests).
- Docs: ADR-001..008 committed; task-card template; first 12 task cards filed.

**M0 exit criteria — the demo:** client opens Thornwall (placeholder art), hero
click-to-moves with 8-dir anim, camera follows, day/night tint cycles on debug key,
map blocks correctly; `ctest` green on both OSes; 60 s replay journal round-trips.

## Phase 1 — Netcore (Oct 5 – Nov 8, 2026)

**Goal:** retire netcode risk while the game is still tiny. Online-first by choice.

- ENet transport, protocol v0 (`messages.md` → generated serializers), login stub
  (account/char create, SQLite v1 schema).
- Zone server (1 process): tick loop, entity store, spatial hash, **AoI sync**,
  authoritative movement (server A* + speed validation), chat (local/global/EK-tag).
- Client: connect → spawn → interpolated movement of others; era HUD frame; chat box.
- **`tools/bots` v1**: 20 headless wanderers; CI soak smoke (10 bots × 60 s).
- Persistence v1: position/login/logout save; itemless.

**M1 exit criteria:** 1 human client + 20 bots visible simultaneously in Thornwall,
p99 tick < 10 ms on the VPS, zero desync-hash mismatches over a 30-min soak, chat
works, reconnect works, deploy = one `git push` + systemd restart.

## Phase 2 — Combat & Progression (Nov 9 – Dec 20, 2026)

**Goal:** the core single-player-able loop exists *multiplayer-native*.

- Stats/XP/levels (3 pts/level), HP/MP/regen, hit/damage formulas, crits, gore decals.
- Melee + basic casts; cooldowns; potion sip; weapon/armor slots + paper-doll stats.
- Mob system: spawners, leash, chase/attack AI, 6 starter mobs (fields); NPC vendors
  (potions, repair); town vs field safe-zone flags.
- Death: XP debt, de-level, respawn, corpse marker; inventory + loot tables; trade
  window (transactional).
- Skills v1: each class's first 4 skills; class choice at CharCreate.
- **Weapon mastery spine (Soma adoption)**: use-based weapon skill, +1 atk per 20
  skill; aura *combat* hooks wired (learn/buy at the Anvil in Phase 3, see below).
- Content density rule (learned S5): planned hunter throughput per field ≥
  expected concurrent grinders × kill rate; spawner counts are content, not
  afterthought.
- Content: Thornwall + Churchyard + Bleak Fields; XP curve to level 12.
- Bots v2: `grinder` + `support` behaviors; balance Monte Carlo (kill rates, TTK,
  XP/hr by level band) as a nightly report.

**M2 exit criteria:** you can take a Ravager 1→8 killing ghouls with 2 friends online,
die red-faced to a Plague Bat swarm, sell loot, buy pots, and the XP-debt sting makes
someone swear out loud. Deterministic replay reproduces a recorded wipe exactly.

## Phase 3 — The Loop (Jan 4 – Feb 21, 2027)

**Goal:** the *reasons to group up and the reasons to fear other players*.

- **Party system**: invites, frames UI, XP share w/ contribution (heal-weighted),
  aura radius; **full Cultist kit** (Bless/Ironskin/Haste/Chorus/Sanctuary/Resurrect/
  skeleton pet) + remaining class kits to MVP levels.
- **Alignment & PK**: karma engine, red-name rules, guards, chaotic drops, `/duel`,
  EK leaderboard (two-town war state at 19), safety bubbles for <level 8.
- **The Anvil**: ore purity, refine +0..+7, durability/repair, Blessed-scroll stub
  (data-only until LATER), affix loot (Magic/Rare/Unique tables).
  - **Weapon Auras (Soma)**: weapon-skill-gated (20/50/80/120/150) permanent weapon
    gifts, claimed with monster-part + gold tolls at the Bonesmith; includes the
    3-target multiattack tier. Failed refines destroy items (Mir), auras persist
    through them -> the aura grind is the *retained* progression through gear churn.
- **Day/night**: darkness + light radius + night spawns + torch/Blessed lantern items;
  Blood Curse debuff + chapel cure.
- Content: Bonehowl Mine (L12–20, mining nodes), Drowned Crypt (L18–25) +
  **Gravemother** boss; 3 named elites; bounty board (kill-N dailies).
- VFX/audio pass #1: swing/cast/gib/buff feedback, night ambience, heartbeat vignette.

**M3 exit criteria:** a 5-person party (2 humans + 3 support/grinder bots if needed)
clears the Crypt to the boss; a chaotic player gets guard-murdered at the Thornwall
gate and drops items; a +6 refine attempt fails in voice chat and everyone groans —
that's the sound of the game working.

## Phase 4 — Pledges & Siege (Feb 22 – Apr 4, 2027)

**Goal:** endgame politics, weekly rhythm, the screenshot-generator.

- Pledge-lite: create (CHA 20 + gold), ranks, chat, emblem-over-head, roster UI.
- Weeping Castle map (2 gates, courtyard Heartstone, throne, defender spawn).
- Siege scheduler (Sat 20:00 UTC, 90 min), registration, gates/Heartstone HP,
  crown-channel capture, ownership/taxes/vault, holder buff, castle NPC.
- **Bot siege army**: attacker/defender profiles with target priorities (gate →
  stone → backline); rehearsal command `gm siege-now` for dev runs.
- Blood Moon event; EK leaderboard website-lite (static page from kill_log).
- Anti-grief pass: spawn-camp protection gates, crown-channel LoS rules, siege
  login-queue fairness.

**M4 exit criteria:** 3 full scripted sieges with 40 bots + 2 humans complete without
a restart; ownership flip works; taxes show up in shop prices next day; the losing
pledge immediately argues about rematch rules in chat. *(Film it — this is the
trailer shot.)*

## Phase 5 — Hardening & Closed Alpha (Apr 5 – May 30, 2027)

**Goal:** survive strangers.

- Patcher/launcher (`bh-launcher`) + version-gated login; crash reporter
  (minidump → server bucket); GM/audit tooling; rate limits + packet fuzzing pass.
- Persistence: migrations v2, item-dupe audit queries, backup/restore drill
  (prove you can resurrect the world from yesterday).
- Perf: 200-bot siege soak p99 tick < 25 ms; memory-flat 12h run; client 60 fps on a
  2018 MacBook Air at 1×.
- Content finish: all 12 mobs + 3 nameds + boss AI-tuned; era-final UI skin; SFX pass;
  tutorial-as-world-design (quests minimal: 6 bounty-board rails + NPC barks).
- Alpha ops: invite waves (10 → 30 → 100), Discord, devlog cadence locked, ban/rollback
  playbook written *before* it's needed.

**M5 / alpha definition of done:** the "Friday-Night Test" (see `05-mvp.md` §6)
passes with real invited players, two weeks running, and the top-10 EK board has
names the devs don't recognize.

## Risk-driven re-orderings (standing rules)

- If Phase-1 soak shows >25 ms ticks at 100 bots → pause features, perf sprint
  (profiler-driven entity-store/AoI fixes) until green.
- If art throughput < 1 mob/week by Phase 2 → drop roster 12→8, combine mine/crypt
  tilesets, and buy a pixel artist for the boss (budget line in `05-mvp.md` §5).
- If playtests show solo out-farming parties → raise party bonus before touching XP
  curve (group-first is a pillar, not a number).
