# BLOODHOLLOW *(working title)*

> A dark-horror, blood-soaked 2D MMORPG in the spirit of **Helbreath**, **Lineage 1**,
> **Dark Eden**, and **Legend of Mir 2** — isometric pixel art, brutal grind, open PK
> with red-name chaos, castle sieges, gamble-enhancement, and support classes that
> make or break a party. Built for **Linux & macOS** on a **custom C++ engine**,
> developed by a human director + a swarm of AI agents.

---

## One-paragraph pitch

The old kingdom of **Vessalia** has bled out. A plague of undeath crawls out of the
marshes at night, two successor cities — **Thornwall** and **Marrowgate** — pay bounties
for each other's heads, and the last **Weeping Castle** on the border changes hands
every week in a scheduled bloodbath. You are one of the desperate: a **Ravager** who
settles arguments with steel, a **Gravecaller** who weaponizes the plague, or a
**Cultist of the Pale Choir** whose buffs and heals keep a warband alive. Grind the
fields and mines by day, fear the dark by night, gamble your best blade on the
blacksmith's anvil, and bleed for your bloodpledge on siege night.

## Design pillars

1. **Era-authentic feel.** Isometric 2:1 tiles, 8-direction sprites, click-to-move,
   deliberate swing/cast timings, potion-chugging, chunky parchment UI. It should
   *feel* like 1999–2003, not "retro-styled 2024".
2. **Group-first.** Content is tuned so a 5-man party with a support beats any solo
   grind; party XP bonus outperforms solo at every level.
3. **Stakes.** XP debt on death, item-drop risk for red players, enhancement that can
   eat your favorite weapon, castle tax that actually matters.
4. **Politics as endgame.** Bloodpledges, scheduled sieges, town taxes, EK
   (enemy-kill) leaderboards. The game manufactures drama on purpose.
5. **Horror with teeth.** Darkness is a mechanic, not a filter. Blood persists on the
   ground. Some things only hunt at night.

## Locked decisions (2026-09-04)

| Decision | Choice | Rationale |
|---|---|---|
| Language / engine | **C++20 + custom engine on raylib 5** ("BloodEngine") | True "custom engine", zero-dependency, trivially portable to Linux/macOS, single language client+server, extremely agent-friendly API. |
| Networking | **ENet (reliable UDP)** | Purpose-built game transport, tiny C lib, proven in raylib MMO projects. |
| Server model | **Single authoritative world-server process** (zones as modules), 20 Hz sim tick | These games are tile-grid RPGs, not shooters; 20 Hz + interest management is plenty for hundreds of CCU. |
| Persistence | **SQLite (WAL)** | Zero-ops for solo dev; migrate to Postgres only if load ever demands. |
| Team shape | **You (~12 h/wk) + AI agents** | Roadmap calibrated to part-time human oversight; agents do the volume work under task cards. |
| MVP strategy | **Online-first** | Netcode risk retired in Phase 1; every later system is built multiplayer-native. |
| Art | **Mixed**: free/placeholder sprites for MVP, AI-generated monsters w/ cleanup, human-made key art later | Art volume is the #1 scope risk; decouple it from engineering. |

Full reasoning + alternatives matrix: [`docs/03-architecture.md`](docs/03-architecture.md) §1.

## Documents

| Doc | Contents |
|---|---|
| [`docs/01-research.md`](docs/01-research.md) | Anatomy of Helbreath / Lineage 1 / Dark Eden / Legend of Mir 2: what to steal, what to modernize, era tech, sources. |
| [`docs/02-gdd.md`](docs/02-gdd.md) | The game: world, classes, skills, stats, combat math, grind loop, loot, enhancement, PK/alignment, party, siege, day/night, art direction, content budget. |
| [`docs/03-architecture.md`](docs/03-architecture.md) | Engine + server architecture, netcode & protocol, persistence, tooling, data pipeline, testing, CI, hosting, the AI-agent development pipeline. |
| [`docs/04-roadmap.md`](docs/04-roadmap.md) | Phases 0–5 with calendar dates (part-time pace), milestones M0–M5 with playable exit criteria. |
| [`docs/05-mvp.md`](docs/05-mvp.md) | The MVP: exact in/out scope, sprint-by-sprint plan, risks, metrics, the "Friday-Night Test", alpha checklist. |
| [`AGENTS.md`](AGENTS.md) | Standing rules every AI coding agent must follow in this repo. |

## Title candidates

BLOODHOLLOW (current favorite) · Gravemark · Redfang Chronicle · Duskfall ·
Hexmoor · Ashenveil — finalize before the public devlog starts.

## Status

**Phase 0 — Foundations: DONE (Sprints 1–2, 2026-09-04).** M0 met and hardened:
BloodEngine iso renderer, `.bhmap` pipeline (generator → Tiled JSON → validated binary),
fixed-point hashable movement sim (`shared/sim/walker`), **replay journal with per-tick
hash oracle** (222/222 verified in the Phase-0 gate runs — historical: today's CI runs
build + ctest + mapgen-diff; replay/soak legs run via `tools/*_leg.sh` scripts), real
4-hour day/night clock, camera rig,
12 documented ADRs in [`docs/adr/`](docs/adr/). Evidence:
[devlog 0001](docs/devlog/0001-sprint-1.md) · [devlog 0002](docs/devlog/0002-sprint-2.md)
(screenshots) · [task board](docs/tasks/README.md).

Build & run:

```bash
cmake --preset linux-gcc && cmake --build --preset linux-gcc   # or macos-arm64
ctest --preset linux-gcc
./build/linux-gcc/client/bh_client                       # play offline
./build/linux-gcc/client/bh_client --record demo.bhj     # journal record
./build/linux-gcc/client/bh_client --replay demo.bhj     # verify determinism (exit 0 = clean)
```

**Phase 1 — Netcore: M1 "Ghost Town Online" PASSED (Sprints 3–4, 2026-09-04).**
Generated protocol v0 (kProtocolVersion=1) over hash-pinned ENet/SQLite, a 20 Hz
authoritative zone server with spatial-hash AoI ≤26 tiles, SQLite-WAL login +
character persistence (auth is a documented stub — ADR-0009), client-side 120 ms
interpolation, and in-world chat. Gate evidence: 20 bots × 600 s soak,
**tick p99 = 0.30 ms vs 10 ms budget** (p50 99 µs), 3.23 M packets, zero errors;
plus a human client in the same world (screenshot in devlog).
[devlog 0003](docs/devlog/0003-sprint-3-4.md) · [ADR-0009](docs/adr/ADR-0009-vendored-deps-pinned-hashes-auth-stub.md).

Play online:

```bash
./build/linux-gcc/server/bh_server --map assets/maps/thornwall.bhmap    # term 1
./build/linux-gcc/tools/bots/bh_bots --count 20 --secs 600              # term 2 (ghost town)
./build/linux-gcc/client/bh_client --server 127.0.0.1 --name yourhero   # term 3 (you)
# soak gate: bh_server --soak-secs 600 --p99-budget-ms 10  (exit 0 = pass)
```

**Phase 2–3 — Combat, The Loop: SHIPPED through T-103 (Sprints 5–22+, 2026-09-05 →
2026-09-10).** The roadmap calendar (`docs/04-roadmap.md`) is superseded — actual
velocity ran ~5 months ahead of plan. What is in the tree today: full combat law
(swings/crits/auras/cleave/slam telegraphs), mobs + spawners + named-elite bosses
(Old Maw, Red Widow, Cantor Vex), parties + XP share, class kits (Ravager/Gravecaller/
Cultist), anvil enhance/refine + durability, vendors + fence economy, trades with
commit-time validation, karma/alignment + duels + wanted law, day/night + light +
torch/lantern, 5 zones in one process (town, fields, crypt, mine, drowned crypt) with
portals, journal **epoch 18** with the gate leg `logs/t107.bwj`
(`ticks=12801 cmds=7830 hashes=513 mismatches=0`). Task board and per-card devlogs are
the authoritative status: [`docs/tasks/`](docs/tasks/) · latest devlog
[`0072`](docs/devlog/) (T-103).

**Next:** hardening wave from the 2026-09 external review (T-104+: input parsing,
trade symmetry, deque-lifetime UB, hash-oracle widening, headless preset, auth
hardening) — see the review's findings→card map.
