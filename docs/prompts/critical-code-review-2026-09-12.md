# Critical code review prompt — BLOODHOLLOW (2026-09-12)

> Executable agent brief. Goal: analyse the monorepo, critically review
> shipped code (post T-103 / T-104–T-110 hardening wave), land reviewable
> patches with tests, and open PRs. Branch lock for this Arena session:
> `arena/01a09574-bloodhollow` only.

## Ground truth (priority order)

1. This prompt + any `docs/tasks/T-*.md` cards you open.
2. `docs/02-gdd.md`, `docs/03-architecture.md`, ADRs, `AGENTS.md`.
3. Existing code and tests.

## Mission

1. **Map the tree** — server (`server/src/world.cpp` ~3.3k LOC is the risk
   surface), shared sim/protocol/content, client, tools/bots, tests, CI.
2. **Critically review** for correctness, UB, trust-boundary holes,
   determinism/replay breaks, economy asymmetry, and silent law drift.
3. **Patch** findings with tests first-or-with; keep diffs reviewable
   (~400 LOC/PR aspiration). Hand-format touched lines only.
4. **Evidence** — suite green when buildable; journal epoch only if sim
   semantics under old journals actually change; document deviations.
5. **PR** from `arena/01a09574-bloodhollow` → `master` (do not switch branches).

## Review checklist (run every pass)

### Trust boundary (server never trusts client)

- [ ] Chat/slash parsers: no `std::stoi`/`stoul`/`stol` on client digits
      (T-104 class — remote DoS via `std::terminate`).
- [ ] Inv-blob / bless / journal loaders: throw-free numeric parse.
- [ ] Protocol `Reader::str` caps honored; Hello/auth rate limits (T-109).
- [ ] Movement speed, range, CD, gold/qty validated server-side.

### Determinism & lifetime

- [ ] All gameplay RNG via `sim/rng.h` only.
- [ ] No wall-clock in sim; fixed 20 Hz ticks.
- [ ] `std::deque<Entity>` erase invalidates **all** refs/iters
      ([deque.modifiers]) — every `killMob`/`despawn` site snapshots
      identity before erase (T-106 class).
- [ ] `applyWorldCommand` is the single live+replay mutation path (T-049).
- [ ] `worldHash` covers economy/progression (T-107); epoch bump + fresh
      gate leg when values change.

### Economy / inventory symmetry

- [ ] Trade validator and swap share one “what counts” grammar (T-105).
- [ ] Anvil/refine: slot indices re-resolved after any `inv.erase`.
- [ ] Durability/affix/refine survive login blob parse (T-049x).

### Spatial / zone

- [ ] `walker.place` and `spatial.insert/move` always agree on (x,y).
- [ ] Cross-zone death → town bind uses the **same** tile for body + grid.
- [ ] Portal transfer, gallows bind, spawn protect stay coherent.

### Build / CI

- [ ] `headless` preset builds without raylib/X11 (T-108).
- [ ] Client-law tests gated on `BH_BUILD_CLIENT`.
- [ ] Warnings-as-errors; no new third-party deps without ADR.

## Findings log (this session)

| ID | Sev | Area | Summary | Card / fix |
|---|---|---|---|---|
| F1 | **P0** | `World::respawnTick` | Cross-zone respawn inserts spatial at `spawnPoint` while walker is placed at `home_` (gallows/bind). AoI lies until first step. | T-111 |
| F2 | **P0** | `World::tryAnvil` | `InvSlot* wslot` dangling after part-stack `inv.erase` during toll consume; aura write / destroy hits freed/wrong slot (UB). | T-111 |
| F3 | **P1** | `parseInvBlob` | Digit check then `std::stoul` — oversized digit strings still throw `out_of_range` (login/replay DoS). | T-111 |
| F4 | **P1** | `initialMobSpawns` | `--n` on blocked re-roll: `uint32_t` wrap + possible tight spin when rect is mostly blocked. | T-111 |
| F5 | **P2** | `killPlayer` | Wanted-mark `for` nested under PK `if` with misleading indent (logic OK; readability landmine). | T-111 (comment/brace tidy) |
| F6 | info | Architecture | AoS `deque` + per-tick alloc still debt (T-110 truth-up); slot+gen store deferred. | no code |
| F7 | info | T-104..110 | Hardening wave present in tree (parseRefineArg, trade equipped, deque snapshots, worldHash, headless tests, loginlimit) but **task cards/devlogs not filed** under `docs/tasks/done/`. | docs follow-up |

### Already closed upstream (do not re-litigate)

- T-104 refine parse throw-free · T-105 trade unequipped grammar · T-106
  kill-line snapshots · T-107 worldHash widen (epoch 18, `logs/t107.bwj`) ·
  T-108 headless test gate · T-109 login limiter · T-110 docs truth-up.

## Patch plan executed under this prompt

1. **T-111** — F1–F5 code + unit pins (respawn spatial, anvil slot index,
   throw-free inv blob ints, bounded mob seed retries).
2. Update task board / short devlog; open PR with evidence.

## Definition of done

- [x] Prompt filed under `docs/prompts/`.
- [ ] Patches compile warning-free; new tests cover each P0/P1.
- [ ] No unjustified epoch bump (spatial is derived; anvil UB→defined on
      rare erase path — call out in PR if gate leg needed).
- [ ] PR opened; human merges (agents do not merge).
