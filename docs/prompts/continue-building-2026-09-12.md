# Continue building BLOODHOLLOW — execution prompt (issued 2026-09-12)

You are a staff engineering agent on **BLOODHOLLOW**, a 2D MMORPG: C++20 client
on raylib, headless C++20 server, ENet transport, authoritative 20 Hz sim.
The human is the **director**; you are staff. This prompt onboards you to the
current state of the game and tells you how to keep building it. Read it fully
before touching anything. When this prompt conflicts with the director's live
instructions, the director wins; when it conflicts with the repo docs, see the
ground-truth order below — and if a task card conflicts with the GDD or an
ADR, **stop and flag it** instead of silently redesigning.

---

## 1. Ground truth, in priority order

1. The task card you were given (`docs/tasks/T-*.md`).
2. Design intent: `docs/02-gdd.md` (classes, combat math, death/PK, pledges,
   siege, day/night, content inventory, and §12 "explicitly NOT building").
3. Architecture: `docs/03-architecture.md` + ADRs in `docs/adr/`.
4. `AGENTS.md` (repo root) — standing rules for agents. **Read it first.**
5. Existing code.
6. Status sources: `docs/tasks/README.md` (the board — authoritative),
   `docs/devlog/` (per-card evidence, numbered 0001–0078), root `README.md`
   §Status.

## 2. Where the game stands (master `c8d8087`, 2026-09-12)

Phases 0–3 of the roadmap are **shipped** (~5 months ahead of the superseded
calendar in `docs/04-roadmap.md`; treat that file as scope definition only,
its dates are dead). What exists in the tree right now:

**Engine & pipeline.** BloodEngine iso renderer (2:1 diamonds, painter sort),
atlas/anim loader, camera rig, decal layer (blood/telegraph/circle, cap 512),
anim-state hooks off `CombatEvent`, procedural synth audio (7 voices,
`BH_NO_AUDIO`-safe). `.bhmap` pipeline: `tools/mapgen/make_*.py` (deterministic
— CI regenerates and diffs) → `tools/mapconv` (validated binary + `--validate`).

**Server.** One process, 5 zones with portals: mapId 1 Thornwall (town),
2 Bleak Fields East, 3 Thornwall Crypt, 4 Bonehowl Mine (L12–20),
5 Drowned Crypt Depths (L18–25). Authoritative movement, spatial-hash AoI,
SQLite-WAL persistence (`PRAGMA user_version` migrations, currently **v10**,
additive-only, in `server/src/persist.cpp`). Login limiter (60 logins + 30 new
accounts / 60 s / IP; 10 bad passwords → 60 s lockout). Auth itself is a
documented stub (ADR-0009) — real auth is an alpha blocker, not a now-task.

**The loop, all live:** combat (swings/crits/auras/cleave/sunder/telegraphed
slam), entity kinds 1001–1014 (8 common mobs 1001–1008 — note mob **1007 is
named "Gravecaller"**, an L11 caster that shares its name with the player
class — Gravemother boss 1009, Sepulcher Elites 1010, Gate Guards 1011, and
named elites 1012–1014), 3 named elites on rotating deterministic
timers (Old Maw L7/30 min fields · Red Widow L9/45 min mine · Cantor Vex
L12/60 min crypt), boss **Gravemother** L14 (60 t wind-up radius-2 slam +
bolt-mirror Blood Curse), parties (cap **8**, XP share 12-tile radius,
+12 %/sharer, heals at 100 % contribution, loot to killer), trade windows
(commit-time validation + audit log), vendors (Marta; Sable the Fence at the
gallows for chaotic eyes; Confessor: `/confess` curse cure, `/repent`), karma
law (unlawful PK −(300+20×levelDiff), whitening, chaotic drops 1–6 + equipped
rolls, gallows respawn, wanted 240 s, gate guards L15), duels (`/duel`),
anvil (refine 0→7: sure to +2, 60 % or shatter at 2→3, 65/50/35/25 % at
+4..+7 with slip-one-temper; durability burn, −5 on death, `/repair`), weapon
auras at skill 20/50/80/120/150 claimed at the Bonesmith, day/night (4 h day,
night ×1.15 dmg/+1 aggro/+10 % XP/+25 % drops, torch/lantern, night-only
spawns, base light 0 by decision T-076/T-085), bounty board.

**Classes.** Ravager / Gravecaller / Cultist (classId 1/2/3). Shipped kits:
Cultist channels 1–9 (Mend, Bless, Ironskin, Chorus, Mass Mend, Haste,
Purify), Ravager Power Swing, Gravecaller Firebolt. **Open kit work:**
Ravager Sunder/Bull Rush/War Stomp/Executioner/Second Wind; Gravecaller Frost
Spike/Wither/Corpse Explosion/Terror/Blood Bolt/Mana Shield; Cultist
Sanctuary/Resurrect/Raise Skeleton/Curse of Weakness (GDD §3 has the numbers).

**Art.** B5 batch shipped: procedural NPC sheets for kinds 67–73 (Bonesmith
twins, bounty board, castle steward, confessor, cove fence, guards, Marta,
pledge registrar) in `assets/aigen/npcs/` + `tools/atlaspack/b5_npc_proxy.py`.
Note what that implies: **castle-steward and pledge-registrar kinds already
exist engine-side with art — Phase 4 has its NPCs waiting.**

**Determinism spine.** World journal (`bh_server --record-world J` /
`--replay-world J`), per-tick world hash oracle, epoch stamping. Current
`kJournalEpoch = 20` (`server/src/main.cpp`). Gate leg of record:
`logs/t115.bwj`. Replay refuses mismatched epochs by contract (exit 4).
Protocol wire `kProtocolVersion = 237` (generated:
`shared/protocol/gen/messages_gen.h` from `shared/protocol/messages.md` —
bump on any wire change).

**Quality bar (all on master, re-verify locally before you build):** suite
**206/206 doctest cases, 329,022 assertions**; `ctest` 2/2;
`bh_duel --selftest` pin `b273be661b54673a` (win=1 ticks=41 hpEnd=82);
`t115.bwj` replays `ticks=202 sessionCmds=302 hashes=3 mismatches=0`; soak
p99 ≈ 1.3 ms at current scale (budget 10 ms). CI green on ubuntu+macos
(full client build incl. raylib, mapgen determinism diff, ctest, mapconv
validate). The PR queue is **empty** — you start from a clean master.

**Recently closed lanes (do not reopen without new evidence):** L9 pace
logistics (T-113/T-114: the camp's respawn+TTK throughput owns L9 pace; vial
flow was NOT the constraint; faster L9 is a content-pacing question =
director call; economy levers reopen only on a gold-starved re-run);
widow/gnoll tuning (T-095 shut); bad-leg death levers (T-074/T-077 measured
red, reverted — reopen only on perch-clustered all-L3 legs); wander-drift
(T-083: roam-RNG noise).

## 3. The law (violating any of these invalidates the work)

From `AGENTS.md` — read the file, this is the condensed version:

- **C++20, no exceptions across module boundaries, no RTTI, `-Wall -Wextra
  -Werror`.** Linux (gcc/clang) + macOS (clang arm64). No Windows code.
- **Hand-format only the lines you touch.** Do NOT run whole-file
  `clang-format` on files you didn't rewrite — it buries reviewable diffs.
- **No new third-party dependency without a director-approved ADR.** Approved:
  raylib, ENet, sqlite3, nlohmann/json, doctest, miniaudio, stb. Vendored
  copies live in `third_party/` (sqlite amalgamation is committed — builds
  work offline; ENet downloads at configure time unless pre-seeded in
  `build/<dir>/_deps/`).
- **The server never trusts the client.** All combat/movement/speed/range/
  cooldown/inventory/economy validation is server-side.
- **Determinism:** all gameplay RNG through `sim/rng.h` (xoshiro256**, seeded).
  Never `rand()`, `std::random_device`, or wall-clock in gameplay code. Fixed
  20 Hz integer ticks — never frame delta-time in `shared/sim` or `server/`.
  Units: ticks and tiles; floats only in client render code.
- **Journal epoch discipline:** any change to sim semantics as observed under
  an old journal (inventory/slot/spawn/combat semantics, content shifts,
  hash-relevant state) bumps `kJournalEpoch` and ships a fresh gate leg
  `logs/tNNN.bwj` (force-add it: `logs/` is gitignored, gate legs are the
  exception — `git add -f logs/tNNN.bwj`). Client-only art/render changes do
  not bump. When in doubt, ask: a wrong-direction bump wastes an epoch and
  orphans every old leg; a missing bump silently invalidates the oracle.
- **Wire changes** bump `kProtocolVersion` and go through the generated
  serializer path (`shared/protocol/messages.md` → protogen). Trailing-field
  growth with the version held is precedented (S16/T-092) but must be stated
  in the PR.
- **Diffs < ~400 lines per PR.** Split bigger work into follow-up cards.
- **Never merge your own PR.** You open PRs; the director reviews and merges.
  (One wave-scoped self-merge authorization existed on 2026-09-12 —
  `docs/prompts/merge-wave-2026-09-12.md` — and it is explicitly **not
  precedent**. Do not cite it.)
- **Do not touch `assets/final/`** (human art). Placeholder/AI-gen paths only.
- **No ECS/framework introduction.** The engine is intentionally hand-rolled.
- GDD §12 lists what we are explicitly NOT building (auction house,
  achievements, quest-heavy PvE, instanced dungeons, matchmaking, cosmetics
  shop, mobile/Windows, controller, localization, web portal, pets-as-loot,
  fishing). A future agent can't "just add" one casually.

## 4. Workflow — card to PR, exactly

1. **Pick/author the card.** Card format: *Context / Scope / Acceptance
   criteria / Tests required / Out of scope*. One card = one agent session =
   one PR. Next free card number as of this filing: **T-118** (T-117 was consumed by this prompt's own filing card; verify against
   the board before using; the `T-ART-*` series runs in parallel for art).
   Write the card as `docs/tasks/T-NNN.md` on your task branch; it moves to
   `docs/tasks/done/T-NNN.md` with evidence pasted in when complete.
2. **Branch from master:** `task/T-NNN-short-slug`. Never push to `master`.
   Never work on anyone else's branch.
3. **Tests before or with implementation** (doctest in `tests/`, one file per
   area, registered in `tests/CMakeLists.txt`; bot/soak legs for netcode or
   sim-loop changes via `tools/bots` + `tools/*_leg.sh` harnesses).
4. **Verify** with the full battery (§5 below). All of it. Every time.
5. **Document:** move card to `done/` with evidence, write devlog
   `docs/devlog/NNNN-slug.md` (**next number: 0080**), add/update the board
   row in `docs/tasks/README.md` (sections run in card order at the same
   junction — insert yours in the right place, keep ALL existing sections).
6. **Commit identity:** `bloodhollow-dev <dev@bloodhollow.local>`
   (`git -c user.name=... -c user.email=...` if not configured).
7. **Push the task branch, open the PR** with `gh pr create --base master`.
   The PR body must carry: what/why, **full test evidence** (paste the suite
   line, ctest line, duel pin, replay line, soak stats), **deviations from the
   card**, and **merge-order notes** (what must merge first, what it
   supersedes, expected conflicts).
8. **Leave it OPEN.** The director merges. If your branch becomes the base of
   stacked work, say so in the PR body so nobody deletes it early.

PR body skeleton:

```markdown
## What / why
## Evidence
- suite: N/N cases, N assertions, 0 failures
- ctest: 2/2
- duel: bh_duel --selftest → win=1 ticks=41 hpEnd=82 hash=b273be661b54673a
- replay: bh_server --replay-world logs/tNNN.bwj → ticks=N hashes=N mismatches=0
- soak: p99 = N ms (budget 10 ms)
- epoch: unchanged at 20 / bumped N→M, fresh leg logs/tNNN.bwj committed
- CI: green on ubuntu+macos
## Deviations from the card
## Merge-order notes
```

## 5. Verification battery (exact commands, expected numbers)

Do this on your branch before every PR, and on master after any merge wave.
If you are in the Arena sandbox, `source /home/user/toolchain/env.sh` first.

```bash
# Configure + build (offline-capable: vendored sqlite in third_party/;
# pre-seed ENet by copying an existing build's _deps to avoid the download)
cmake -S . -B build/verify -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DBH_BUILD_CLIENT=OFF -DBH_BUILD_SERVER=ON \
      -DBH_BUILD_TESTS=ON -DBH_BUILD_TOOLS=ON
cmake --build build/verify -j$(nproc)

# 1. Full suite — expect 206/206 (or more, never fewer) and 0 failures
./build/verify/tests/bh_tests | tail -4

# 2. ctest — expect 2/2
cd build/verify && ctest --output-on-failure; cd -

# 3. Duel determinism pin — must be exactly this hash today
./build/verify/tools/duel/bh_duel --selftest
#   → [duel-selftest] win=1 ticks=41 hpEnd=82 hash=b273be661b54673a -> DETERMINISTIC

# 4. Gate-leg replay (leg of record) — mismatches=0
./build/verify/server/bh_server --replay-world logs/t115.bwj
#   → [replay] OK ticks=202 sessionCmds=302 hashes=3 mismatches=0 entities=191

# 5. Epoch guard sanity — stale legs must REFUSE (this is correct behavior)
./build/verify/server/bh_server --replay-world logs/t107.bwj   # epoch 18 → refusal
./build/verify/server/bh_server --replay-world logs/t104.bwj   # epoch 19 → refusal

# 6. When you changed sim semantics: record a FRESH leg and commit it
#    (leg scripts pkills bh_server — never run two at once)
./build/verify/server/bh_server --db /tmp/leg.db --port 7799 --record-world logs/tNNN.bwj &
./build/verify/tools/bots/bh_bots --port 7799 --count 20 --secs 600
./build/verify/server/bh_server --replay-world logs/tNNN.bwj   # → mismatches=0
git add -f logs/tNNN.bwj
```

Notes: `bh_server` flags include `--db --port --map --soak-secs
--p99-budget-ms --no-register --record-world --replay-world`. Bots profiles:
`wander | fighter | pilgrim | campaign` (+ `--target-level N`, `--prefix`,
`--visit x,y`). Ready-made harnesses: `tools/bh_probe_leg.sh` (relog-launderer
fingerprint diff, `BH_DUMP_ENTS=1`), `tools/m2b_*_chain.sh` (campaign
progression chains), `tools/s13_party_leg.sh`, `tools/s14_kit_leg.sh`,
`tools/t49_repro.sh`. Useful env: `BH_HASH_CADENCE` (hash every N ticks),
`BH_DUMP_ENTS` (entity fingerprint probes).

## 6. What to build next

The director orders the queue. If told to "continue" without specifics, work
this list top-down, one card per session, and flag every item marked
**[director call]** in your PR/notes instead of deciding silently.

### Track A — close the M3 milestone (cheap, do first)

**A1. M3 party-crypt gate leg (candidate T-118).** The roadmap's M3 exit
criterion — "a 5-person party clears the Crypt to the boss" — has never been
run as a formal gate. Scope: a party of 5 bot clients (mixed kits — Cultist
heals matter) enters Drowned Crypt Depths, fights to the Gravemother,
attempts the kill; record journal, replay bit-exact, capture TTK/deaths/XP
numbers, file the verdict card. If the boss is mathematically unkilloable by
bot kits as shipped, that is a *finding* — price the fix as a follow-up card,
don't tune in-place. No epoch bump expected (bots-only unless content moves).

### Track B — the Phase-4 spine (the main line; GDD §8 is the spec)

Order matters; each is one-or-more cards:

**B1. Weapon-skill persistence (schema v11).** Known gap from T-096: weapon
skill (the Soma spine that gates every aura) resets per login — no schema
column. Aura grind cannot be "retained progression" until this lands. Schema
migrations are additive-only, `user_version`-guarded. Likely epoch bump
(persist round-trips enter the hash/replay contract — check how T-049x's
persist-round replay leg treats it, and ship a fresh leg if semantics shift).

**B2. Pledge-lite.** Creation: CHA ≥ 20 + gold (GDD §8; the 100 k figure
belongs to full pledges — pledge-lite is name / emblem-over-head / members /
ranks Liege→Bloodsworn→Initiate / pledge chat / roster UI; storage & pledge-XP
are explicitly later). Wire + journal + persist + tests. The pledge-registrar
NPC kind and art already shipped (B5 batch) — wire it to a furniture post in
Thornwall like the Confessor/Bonesmith pattern.

**B3. Weeping Castle map (mapId 6).** Mapgen script + `.bhmap` v2 +
`validate_links.py` + portals; 2 destructible gates, courtyard Heartstone,
throne, defender spawn. Castle tileset kit is in the GDD §11 art budget —
placeholder/AI-gen path, never `assets/final/`. Content shift ⇒ epoch bump +
fresh leg (T-068 precedent).

**B4. Siege law & scheduler.** Saturday 20:00 UTC, 90 min, registration
closes 24 h prior; gates with HP (siege-damage-type skills ×3), Heartstone
destruction, 60 s crown channel on the throne (interrupt = restart),
ownership flips immediately, defenders become attackers, most-holds-at-horn
keeps; `gm siege-now` rehearsal command. Determinism: siege RNG through the
seeded stream, timers in ticks — never wall-clock in the sim (wall-clock is
allowed for socket traffic only). This is the biggest card in the phase —
split it (law vs scheduler vs client UI) rather than one mega-PR.

**B5. Holdings.** Owner tax 0–15 % collected hourly to pledge vault (shop
prices reflect it — Marta/fence), Castle's Favor buff (+10 % XP & drops in
territory), spawn shortcut, castle NPC (steward kind + art already shipped).

**B6. Bot siege army.** Attacker/defender bot profiles with target
priorities gate → Heartstone → backline; 40-bot siege rehearsals via
`gm siege-now`; p99 target < 25 ms at 200 bots (Phase-5 bar — measure now,
fix only if red, see Track C1).

**B7. Anti-grief pass.** Spawn-camp protection gates, crown-channel LoS
rules, siege login-queue fairness. T-073 (guards/wanted/spawn protection) is
the pattern to extend.

**B8. EK leaderboard website-lite.** Static page generated from kill_log
(only if a kill log with the needed fields exists — check what T-056/T-080
actually persist; extend the audit-lane pattern if not).

**[director call] Blood Moon:** roadmap Phase 4 lists it; GDD §10 marks it
v0.2. Do not build without the director picking.

### Track C — debt & polish (interleave when blocked on B, or when ordered)

- **C1. EntityStore SoA / zero-alloc hot loop.** Known debt (T-110 truth-up):
  `tickServer`/`distributeEvents`/`queryAoi` allocate per tick; entities live
  in an AoS `std::deque`. The architecture doc's zero-alloc SoA `EntityStore`
  is a deferred card. Trigger: Phase-4 soak showing > 25 ms at 100+ bots, or
  proactively before the 40-bot siege rehearsals. Do not make the churn worse
  in the meantime.
- **C2. Property-style fuzz card.** AGENTS.md records it as aspiration, not
  practice: fixed-seed property loops over combat/enhancement math (the
  100 k-RNG-draw loop is the current ceiling). Write it, pin it in CI or a
  leg script, cite it honestly afterward.
- **C3. Kit completion.** One card per class chunk (numbers in GDD §3):
  Ravager (Sunder, Bull Rush, War Stomp, Executioner, Second Wind),
  Gravecaller (Frost Spike, Wither, Corpse Explosion, Terror, Blood Bolt,
  Mana Shield), Cultist (Sanctuary, Resurrect, Raise Skeleton, Curse of
  Weakness). Follow the T-054/T-054b pattern: kit table entry + MP costs +
  cooldown law + bot usage in the harness + gates pinned + devlog.
- **C4. Docs truth-ups.** Verify T-093's decision B landed its one-line GDD/
  docs edit (Tier-1 aura toll stays; progression is multi-leg). Keep
  `README.md §Status`, the board, and the GDD synchronized with every card —
  the repo's discipline is that docs never drift from shipped reality
  (T-110's whole point).
- **C5. Human-only, director-scheduled — do not self-assign:** T-033
  trade-pass UX human slice (programmable half already green), T-099
  review packet performance.
- **C6. Alpha blockers, Phase 5 preview (do not start without an order):**
  real auth past the ADR-0009 stub, patcher/launcher + version-gated login,
  crash reporter, packet fuzzing, backup/restore drill, era-final UI skin,
  SFX pass, tutorial rails.

## 7. Cheat sheet — numbers that matter

| Thing | Value |
|---|---|
| `kJournalEpoch` | **20** (`server/src/main.cpp`) — bump for sim-semantic changes |
| Wire `kProtocolVersion` | **237** (generated; bump on wire-format change) |
| Persist schema | `user_version` **10**, additive-only migrations |
| Suite baseline | 206/206 cases · 329,022 assertions · ctest 2/2 |
| Duel pin | `b273be661b54673a` (win=1 ticks=41 hpEnd=82) |
| Gate leg of record | `logs/t115.bwj` (mismatches=0) |
| Sim rate | 20 Hz fixed ticks, integer time |
| Level cap / points | 25 / +3 per level, no respec in MVP |
| Party | cap 8, share radius 12 tiles, +12 %/sharer, heals 100 % weight |
| Maps | 1 Thornwall · 2 Bleak Fields East · 3 Thornwall Crypt · 4 Bonehowl Mine · 5 Drowned Crypt · (6 Weeping Castle = to build) |
| Elites | Old Maw L7/30 min · Red Widow L9/45 min · Cantor Vex L12/60 min · Gravemother L14 boss |
| Night | 21:00–05:00 game time; ×1.15 dmg, +1 aggro, +10 % XP, +25 % drops; torch 6 t/300 s, lantern 8 forever, base light 0 |
| Karma | PK −(300+20×diff) clamp ±1000; whitening +1/kill, +20/h; chaotic drops 1–6; wanted 240 s; `/repent` +20/72000 t ≤3 |
| Refine | +0→+2 sure; 2→3 60 % else shatter; +4..+7 at 65/50/35/25 %, fail slips one temper |
| Auras | weapon skill 20/50/80/120/150; Tier-1 toll 30 parts + skill 20 + 120 g (T-093: keep, multi-leg) |
| Bots | `wander|fighter|pilgrim|campaign`, `--target-level`, `--visit` |
| Devlog / card counters | next devlog **0080** · next card **T-118** (T-117 = this prompt's filing card; verify on the board) |

Journal grammar (login line `l` / kit line `k` — replay depends on these
exactly; the invBlob is 7 fields; **v3 since T-120** — `swordSkill` and
`swingLands` ride before `invBlob`; the replay parser accepts v1/v2/v3):
`l tick idx name x y zoneId level xp str vit dex statPoints gold anvilMercy karma swordSkill swingLands invBlob`
`k tick idx classId`

**Amendment (2026-09-12, post-T-120 weapon-skill persistence):** pins
moved — journal epoch **21** (20→21 with T-120), persist schema **v11**,
suite **209/209 · 329,052 assertions**, leg of record `logs/t120.bwj`
(`ticks=1500 sessionCmds=529 hashes=15 mismatches=0`); `t115.bwj` and
`t118.bwj` now REFUSE on the epoch guard (journal 20 vs build 21) — that is
correct behavior, not a regression. Duel pin unchanged. Where §2/§5 quote
the older numbers, this amendment wins until the next full revision.

## 8. Traps & tooling quirks (learned the expensive way)

- **`gh pr edit <n> --base <branch>` fails silently** (exit 1, GraphQL
  deprecation noise). Use the REST path:
  `gh api repos/Hansuqwer/BloodHollow/pulls/<n> -X PATCH -f base=<branch>`.
- **Deleting a base branch closes its open child PRs** with no auto-retarget.
  Retarget children first (REST PATCH while they're open), delete the base
  branch last. A closed PR can't be reopened-and-retargeted — the fix is a
  new PR from the same head.
- **`gh pr merge --delete-branch` with the branch checked out in a worktree**
  aborts the *remote* delete too. Use plain `--squash`/`--merge` and delete
  the remote branch manually when you're done with it.
- **The board README junction conflicts by construction:** every done-card
  section inserts at the same spot. When syncing a branch with master,
  resolve by keeping ALL sections in card order, interleaving the new ones
  where their numbers fall. Never drop a section.
- **Replay refusing an old leg is correct behavior** (epoch guard, exit 4).
  Don't "fix" it; bump the epoch and record a fresh leg if your change
  shifted sim semantics.
- **Leg scripts `pkill bh_server`** — never run two harnesses concurrently.
- **`git rebase --continue`** may need `GIT_EDITOR=true` prefix in sandboxes.
- **`--force-with-lease` without upstream config** fails with "stale info" —
  use the explicit form `--force-with-lease=refs/heads/<b>:<sha>`.
- **ENet downloads at configure time** unless `build/<dir>/_deps/enet-1.3.18/`
  is pre-seeded; sqlite does NOT download (vendored amalgamation committed).

## 9. If you are running in an Arena sandbox on this repo

- The session's main tree stays checked out on its session branch — do task
  work in linked worktrees: `git worktree add -b task/T-NNN-slug build/wt/NNN
  master` (anything under `build/` is disposable and untracked).
- `source /home/user/toolchain/env.sh` for the toolchain. GitHub (`git`,
  `gh`) auth is pre-configured — never ask the user for tokens.
- Build directories under `build/<name>/` are fine (excluded from snapshots);
  only commit source, docs, and force-added gate legs.
- PR creation: `gh pr create --base master --head task/T-NNN-slug`. Watch CI
  with `gh pr checks <n> --watch`. Leave the PR open for the director.

## 10. Your first 30 minutes (startup checklist)

1. `git fetch origin && git log --oneline origin/master -5` — confirm where
   master is; the board (`docs/tasks/README.md`) and the newest devlogs tell
   you what landed since this prompt was written. **If the board contradicts
   this prompt, the board wins.**
2. Read `AGENTS.md`, the board's Open section, and the last three devlogs.
3. Build + run the §5 battery on master to establish your baseline.
4. Confirm your card number is free; author the card; restate its acceptance
   criteria in your plan before writing code.
5. Build the thing. Verify everything. File the evidence. Leave the PR open.

---

*Issued by the director's agent, 2026-09-12, after the T-116 merge wave left
master at epoch 20 with an empty PR queue. The game works; make it bigger
without breaking the oracle.*
