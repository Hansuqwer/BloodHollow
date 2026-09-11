# 03 — Technical Architecture

## 1. Stack decision (ADR-001)

**Chosen: C++20 everywhere.** Client = custom engine **BloodEngine** layered on
**raylib 5** (render/input/audio). Transport = **ENet**. Headless dedicated server
= same C++20, no raylib. Persistence = **SQLite (WAL)**. Build = **CMake presets**,
CI on ubuntu-latest + macos-latest. Formats: Tiled TMX/JSON → custom binary maps;
Aseprite-style JSON + PNG sheets → packed atlases; nlohmann/json for static data.

| Candidate | Verdict | Why |
|---|---|---|
| **C++20 + raylib (chosen)** | ✅ | True "custom engine" as requested; zero-dep, tiny C API agents model perfectly; one language client→server→tools; raylib proven on Linux/macOS ([raylib.com](https://www.raylib.com/)); raylib+ENet dedicated-server precedents exist ([example](https://github.com/ThatsAMorais/networking-game-raylib)). |
| Rust (macroquad/ggez) | ❌ | Excellent server story, but borrow-checker + slower iteration multiplies review burden for a 12h/wk human directing agents. Revisit only if perf forces it (it won't at our scale). |
| C# + MonoGame | ❌ | Fastest iteration and nice packaging, but "custom engine" read weaker, AOT/packaging quirks on macOS, and the shared-code advantage is matched by C++ `shared/` anyway. |
| Unity/Godot | ❌ | Not a custom engine; ECS-editor tooling fights the 1999 aesthetic we're hand-tuning; headless MMO server story is worse. |

ENet gives ordered-reliable channels (chat/inventory/combat commands) and cheap
unsequenced movement updates — the standard modern answer for this class of game
([wirepair](https://wirepair.org/2023/06/29/so-you-want-to-build-an-mmorpg-server/)),
replacing the raw-TCP era norm without redesign.

## 2. Repo layout (monorepo)

```
bloodhollow/
  CMakeLists.txt  CMakePresets.json  .clang-format  AGENTS.md
  client/        # game client executable (bh-client)
    src/ (app/, render/, ui/, net/, states/)
  server/        # headless world+login server (bh-server)
    src/ (net/, world/, ai/, persist/, gm/, botsink/)
  shared/        # THE contract between client & server
    sim/         # grid, A*, movement step, combat calc, rng (xoshiro256**), buffs
    protocol/    # message structs, serializers, PROTOCOL_VERSION
    data/        # JSON game data (stats, items, monsters, skills, maps index)
  engine/        # BloodEngine: window, atlases, sprites, iso renderer, camera,
                 # particles, audio, ui, shaders, assets
  tools/
    mapconv/     # Tiled JSON -> .bhmap binary
    atlaspack/   # sheets -> trimmed atlases + anim JSON
    bots/        # headless scripted clients (soak/load/fake-siege)
    replay/      # session recorder + deterministic re-sim
    gm-cli/      # admin console over a control socket
  assets/ (placeholder/, aigen/, final/)
  data/   (maps-src/*.tmx, sheets-src/)
  tests/  (doctest)
  docs/   (01..05, adr/, tasks/, research-notes/)
  .github/workflows/ (ci.yml: linux+mac build, ctest, mapgen determinism diff,
                      mapconv validation — NO soak/replay/sanitizer legs at HEAD;
                      those run manually via tools/*_leg.sh + *_smoke.sh)
```

**Dependency graph (acyclic):** `shared` ← `engine` ← `client`; `shared` ← `server`.
Server never links `engine`. Tests link `shared` + `server` core only.

## 3. Simulation model (the heart)

- **Fixed 20 Hz tick** (`SIM_TICK_HZ=20`, 50 ms). Order per tick: drain input queues
  → player intents → movement steps → AI think/move → combat resolution →
  buffs/DoTs → respawns/spawner → persistence dirty-set → snapshot build → net send.
  This is the canonical server loop ([reference](https://wirepair.org/2023/06/29/so-you-want-to-build-an-mmorpg-server/)).
- **Grid world:** int tile coordinates, 8-dir movement, 1 tile/step cadence from
  speed stat. *Shipped reality (T-110 sync):* walker baseline is **4 ticks/tile**
  (256 of 1024 Q10 substep units per tick), and there is **no dynamic-entity
  occupancy** — mobs stack on tiles (the devlogs' "perch" phenomena are downstream
  of this). Tile-flag blockers only. Occupancy/soft-push remains an open design
  decision, not a shipped law.
- **Pathfinding:** A* on a cached cost grid, server-side authoritative; client runs
  the identical `shared/sim` A* purely for the click path-preview (no divergence
  possible — same code, same grid version).
- **Determinism:** all RNG via `sim/rng.h` streams (world, loot, per-entity); tick
  N + seed + input-log ⇒ bit-identical replay via `bh_server --record-world` /
  `bh_server --replay-world` (there is no separate `tools/replay` binary — the
  server *is* the replay harness; gate legs live in `logs/*.bwj`). This is our
  debugging superpower *and* the QA agent's test oracle.
- **Data structures:** *design goal:* flat `EntityStore` (struct-of-arrays;
  id = slot+gen), arenas per zone, no per-tick heap allocation. *Shipped reality
  (T-110 sync):* AoS `std::deque<Entity>` with linear `find()`/name scans and
  per-tick `std::vector`/`unordered_set` churn in `tickServer`/`distributeEvents`/
  `queryAoi`; mid-tick `despawn()` erases invalidate all deque references (the
  T-047/T-106 cards are scar tissue from this). The slot+gen store remains the
  prescribed fix — it is a deferred card, not a silent abandonment. Spatial hash:
  uniform grid, **`SpatialGrid<8>` (8×8-tile cells**, not the originally sketched
  16×16), per zone.

## 4. Networking

- **Server-authoritative, client-interpolated.** Client sends *intents* (move path,
  attack target id, slot use, chat). Server simulates and broadcasts **deltas**.
  Client renders remote entities 100 ms in the past with interpolation; own player
  moves with local echo + server correction (tile snaps are rare & era-forgivable).
- **Interest management (AoI):** visibility radius 26 tiles (≈1.5× viewport at 1×);
  each client receives only entities in its AoI, enter/leave events, and delta fields
  of changed entities. O(n·k) instead of O(n²) — the standard requirement
  ([game-ace](https://game-ace.com/blog/mmorpg-games-and-how-to-develop-them/)).
- **Channels (ENet):** *design:* 0 = control (reliable, ordered: login, chat,
  inventory, trades); 1 = state (reliable-ordered per-entity deltas, drop-old for
  movement); 2 = unreliable-unsequenced (spammy cosmetics: particles, swing
  swishes). *Shipped reality (T-110 sync):* **everything runs reliable-ordered on
  channel 0**, including the 20 Hz EntityDeltas; the host opens 2 channels and
  uses 1. The 3-channel split is unbuilt — do not cite it as current behavior.
- **Bandwidth budget** (200 CCU one zone, avg AoI 15 ents): ~24 B/entity-delta ×
  15 × 20 Hz ≈ 7 KB/s per client down — trivially fine; design target ≤ 20 KB/s even
  in sieges (cap AoI entity count; crowd = stutter-frame anim throttling).
- **Protocol sketch** (`shared/protocol/` hand-rolled LE byte streams, no protobuf —
  messages are few and small):

| Dir | Msg | Notes |
|---|---|---|
| C→S | Hello(auth token, client ver) · InputPath(tile list hash? no — waypoints) · Attack(entityId) · UseSlot(idx,target?) · CastSpell(id,target?) · Chat · Trade* · Guild* | intents only |
| S→C | Welcome(you, zone, clock) · Spawn(ents) · Despawn(ids) · Delta(pos/animstate/hp/buffs) · CombatEvent(src,dst,amount,flags) · ChatMsg · Death/Respawn · SiegeState | deltas only |

  Full registry generated from `protocol/messages.md` (single source the agents edit).

- **Zones:** v1 = **one process, zones as in-process modules** (town, fields, mine,
  crypt, castle) with portals. Splitting to multi-process world-servers is an ADR
  away (message-router + shared SQLite) — design `server/world` API so a zone object
  is location-agnostic. Login/auth = same process, separate port thread (MVP:
  account+pass, bcrypt, 4 chars/account — Helbreath rule).

## 5. Persistence (SQLite, WAL)

Tables: `accounts`, `characters` (stats, position, karma, xp, flags), `items`
(instance rows w/ affix JSON, refine level, durability), `inventories` (char/bank/
trade-log), `pledges`, `pledge_members`, `castle_state` (owner, tax, next siege),
`kill_log`, `trade_log` (anti-dupe audit).

- Save policy: dirty entities flushed every 30 s + on logout + pre-shutdown; items
  mutate **transactionally** (trades/refines are single transactions — dupes and
  poof-bugs die here).
- Schema versioned; `migrate.sql` files run at boot. Daily offsite backup cron
  (alpha: simple `sqlite3 .backup` + rsync).

## 6. Client engine (BloodEngine) subsystems

| Module | Responsibility |
|---|---|
| `iso` | 2:1 tile renderer, painter's-algorithm depth sort, elevation layers |
| `sprite` | atlases, 8-dir anim sets, palette-swap shader (class/gear tints) |
| `daynight` | global tint ramp + per-entity additive light masks (torch/Vigil/player) |
| `gore` | render-to-texture decal layer per map chunk (10-min decay), gib particles |
| `net` | ENet peer, delta applier, interp buffers, prediction cache |
| `ui` | immediate-mode era-widgets (orbs, hotbar, chat, paperdoll, trade, anvil), bitmap font |
| `debug` | rlImGui overlay (tick times, AoI counts, entity inspector) — dev builds only |

- Client frame: variable (vsync 60), sim-viz interpolation factor from server tick
  timestamps. Resolution-independent UI (anchor-based, scale steps 1×/1.25×/1.5×).

## 7. Tooling & data pipeline

1. **Maps:** authored in Tiled (orthogonal view with 2:1 tilesets — standard trick),
   `mapconv` emits `.bhmap` (RLE tiles, blocker grid, spawner defs, zones, portals).
   CI fails if a map lacks spawn-block metadata.
2. **Sprites:** `atlaspack` (Python) packs PNG sheets → atlas + `anim.json`;
   validates 8-dir completeness, palette ≤32 colors, frame budgets (fails the build
   if a mob exceeds 100 frames — the era discipline, enforced).
3. **Static data:** `shared/data/*.json` validated by `datacheck` (refs resolve,
   min≤max, etc.) and baked into a header at build (no runtime JSON parse in server
   hot path).
4. **Bots (`tools/bots`):** protocol-level headless clients with behavior scripts
   (`grinder`, `support`, `loser` (wanders), `siege-attacker`, `siege-defender`).
   Use cases: CI smoke (10 bots, 60 s), load soak (200 bots scenario file), demo
   population, siege rehearsal. **This tool exists before combat does** — it is how
   a part-time solo human tests an MMO.
5. **Replay (`bh_server --record-world` / `--replay-world`):** the server writes a
   seed+login+command+hash journal (`.bwj`, epoch-stamped); replay re-sims offline
   and re-hashes world state per cadence for bisect/debug. Gate legs: `logs/*.bwj`
   (current: `t107.bwj`, epoch 18).
6. **gm-cli:** kick/ban/teleport/spawn/item-create/broadcast/set-clock/siege-now.

## 8. Testing strategy

| Layer | Tool | Gate |
|---|---|---|
| Unit (sim, combat, items, enhancement odds, karma math, A*, codecs) | doctest | CI, every PR |
| Property/fuzz | fixed-seed 1M-iteration combat & refine Monte Carlo (assert EV bounds) | nightly |
| Protocol | golden-byte encode/decode vectors; version-mismatch tests | CI |
| Integration | scripted bot scenarios (login→grind→die→refine→trade) vs deterministic server | CI |
| Load | 200-bot soak, budget: tick p99 < 25 ms, RSS < 1 GB, no desync hash mismatch | pre-milestone |
| Replay | any CI failure auto-dumps journal; replay must reproduce | manual debug |

## 9. Build, CI, shipping

- `CMakePresets`: `linux-gcc`, `linux-clang`, `macos-arm64`, `macos-x86_64`
  (universal2 via two-arch lipo step, later). Deps via CMake FetchContent pinned by
  tag (raylib 5.x, enet 1.3.x, sqlite amalgamation, doctest, nlohmann/json).
- CI matrix builds + tests on push; ASAN/UBSAN job nightly; `-Werror` everywhere.
- Shipping (alpha): Linux = tar.gz + AppImage later; macOS = unsigned
  `.app` with a `right-click → Open` note (notarization = $99/yr, buy at alpha wave 2).
- **Patcher/launcher [P5]:** tiny `bh-launcher` (HTTP manifest + sha256 file list,
  rsync-style download), version-gated login (`PROTOCOL_VERSION`).

## 10. Security & anti-cheat posture (from day 1)

Server owns *everything*: movement speed per tick validated against stats+terrain;
attack range/cooldown checked at resolution; inventory mutations transactional;
client is a dumb terminal + predictor. Obfuscation/anti-debug = explicitly out of
scope at alpha (private-server veterans will cheat eventually; server truth is the
only durable defense). Rate-limit packet classes; reject malformed by disconnect.
GM audit log for all `gm-cli` actions.

## 11. Hosting & ops (alpha)

Single VPS (4 vCPU/8 GB, ~€10–15/mo — e.g. Hetzner ash/hel1): `bh-server` under
systemd, SQLite on NVMe, nightly backups offsite, metrics = log lines + a
`/srv status` gm-cli (prometheus = LATER). Expected headroom at 20 Hz, 1 zone
process: ~500 CCU before we need to care (tile sims are cheap; AoI keeps bandwidth
linear). Staging world runs on the same VPS, separate port/db.

## 12. AI-agent development pipeline (how the swarm builds this)

- **Docs-as-spec** (`docs/01–05` + ADRs) are the prompt ground truth referenced by
  every task card (`docs/tasks/T-###.md`): *Context / Scope / Acceptance criteria /
  Tests required / Out of scope*. One card per agent session, ≤1 PR each.
- **Agent roles** (prompt personas, not separate models): Architect (ADRs, reviews),
  Netcode, Gameplay-sim, Content/tools, QA (writes the fuzzes/bots, tries to break
  merges). Human rotates as: task author → reviewer → playtester. Estimated human
  time split: 4h direction/review, 4h integration/playtest, 4h design & devlog per week.
- **Guardrails enforced by CI** (see AGENTS.md): warnings-as-errors, tests required,
  replay determinism, frame/frame-budget asset checks, map metadata checks. Agents
  cannot pass CI without evidence — this replaces human pre-review vigilance.
- **Cadence:** weekly "director review" = human plays the build 30 min, files
  polish tasks; sprint boundaries per `docs/05-mvp.md`.
- **Public devlog** from Phase 1 (screenshots/week): accountability + community seed
  (these games' audiences *find* projects early; helbreath.net/top-eks shows the
  niche is alive).

## 13. ADR index

| # | Decision |
|---|---|
| 001 | C++20 + raylib + ENet stack (this doc §1) |
| 002 | Fixed-step 20 Hz server-authoritative sim; no client authority |
| 003 | Monorepo, zones-in-process v1, split-ready API |
| 004 | SQLite WAL; transactional item mutations |
| 005 | Deterministic RNG + replay journals as QA oracle |
| 006 | Hand-rolled protocol from `protocol/messages.md`; version byte on every packet |
| 007 | Tiled + mapconv as the map pipeline; asset budget checks in CI |
| 008 | Bots-first testing strategy (soak before features) |
