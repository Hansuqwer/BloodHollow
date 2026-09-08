# Engine overnight build — execution brief (T-033 + S20–S25)

*Issued 2026-09-08 (overnight shift). Read `AGENTS.md` first; this brief
assumes it. Ground state: HEAD `afbcda3` (T-068), pushed to `origin/master`.
You are the only engineer awake for the next N hours. Work the queue in
order, one card = one sprint = one devlog = one commit, full Definition of
Done on each. Every card below is fully scoped so you never have to make a
game-design decision — pin numbers, validate, and flag any judgment call in
the devlog for the director's morning review.*

## Where things stand (HEAD `afbcda3`, pushed)

- Phase 3 (S15–S19) is **done**: T-056..T-067 all in `docs/tasks/done/`.
- T-068 (director option A — gravecaller barricade off the bridge) is done;
  `TARGET L8` fires end-to-end (~446 s). Journal epoch is **7**.
- Suite baseline to preserve: **105 tests / 327,925 assertions**, ctest 2/2,
  build warning-free (`-Werror`).
- Open on the board: **T-033** (M2 gate de-QA), **L8→L9 step-up** (DIRECTOR
  decision — do NOT touch), **bot bad-leg deaths** (tuning card, T-074).
- Parallel workstreams to coexist with:
  - Art enablement commits to `master` concurrently — **always
    `git pull --rebase` before push**; expect fast-forward races.
  - `docs/prompts/arena-parallel-eval-brief.md` is **untracked and belongs to
    another workstream — do not touch, move, or commit it.**
- Git identity for this workstream (match the existing author convention):
  `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`.
  Art commits use `Hans Vilund <HansuQWER@protonmail.com>` — leave theirs.

## Hard-won environment facts (do not re-learn)

- Per-command timeout ~30 s. Any run that lives longer must be detached:
  `(setsid ./build/<thing> ... > logs/<run>.log 2>&1 < /dev/null &)` then
  poll with short greps. A full 12-leg chain takes **~1 h 50 min**; leg
  cadence ~9 min. For per-card validation use **short soaks (540–720 s)**
  instead — do NOT spend the night on chain-length runs unless a card says so.
- Chain harness: `tools/m2b_rerun_chain.sh` (fresh DB at
  `/tmp/m2b_rerun_campaign.db`). Single leg: `tools/m2b_leg.sh`. Gate log:
  `logs/m2b_rerun_gate.log`. `.bwj` journal files ARE tracked (git add -f;
  they predate the `.gitignore` rule) — commit them as evidence.
- Resumed-DB validation (val pattern): copy a chain DB, then **set state via
  sqlite3** (`UPDATE characters SET level=?, xp=?, map_id=1, x=?, y=?...`) —
  do NOT resume a DB "as-is": saved positions can park characters inside a
  live aggro cluster and produce a false death-loop (see val5 in devlog 0030).
- `maxLevel` semantics: level at close (SUMMARY counter), NOT the peak. Peak =
  last `reached L<N>` line. Keep `peak`/`end` distinct in every table.
- Journal epoch: `constexpr int kJournalEpoch` in `server/src/main.cpp` with
  the documented chain. **Content changes that shift the sim under old
  journals bump the epoch** (T-030 dropped 4→5 for zones-in-worldHash;
  T-034b 5→6; T-068 6→7). Bot-only changes must NOT bump it (v5c stayed at 6).
  When you bump, record a fresh gate leg and paste `[replay] OK …` in the
  devlog; old journals must refuse cleanly.
- The replay runner refuses mismatched epochs with exit 4 and a plain
  sentence — that is a *feature* (stale by contract), not a failure.

## Standing constraints (AGENTS.md + inherited)

- C++20, no exceptions across module boundaries, no RTTI,
  `-Wall -Wextra -Werror`; `clang-format` before commit.
- Determinism: all gameplay RNG through `sim/rng.h` (xoshiro256**, seeded);
  no `rand()`/wall-clock in sim or server gameplay code. Night schedule is
  tick-derived. Fixed 20 Hz step (`SIM_TICK_HZ=20`), integer ticks/tiles.
- `worldHash` + journal/hash markers are the replay oracle; every new
  gameplay change must keep them deterministic and end-to-end replayable.
- Server never trusts the client; all combat/trade/economy validated
  server-side.
- No new third-party dependency without an ADR. Never touch `assets/final/`.
- GDD conflict rule: **anything that contradicts shipped values keeps the
  SHIPPED values; update the GDD** (`docs/02-gdd.md`), don't renegotiate.
- Numbers pinned (do not silently renegotiate): `kLevelCap = 25`, 3 pts/level,
  XP curve in `shared/sim/combat.h`, party cap 8, karma clamp ±1000,
  whitening +20/logged-hour, aura gates at 20/50/80/120/150 weapon skill,
  moral split ±15% (XP vs gold) live since T-046, mob XP/damage values fixed.
- Max diff < ~400 lines/PR. If a card is bigger, split it into follow-up cards
  (the queue below is already split that way).
- Do NOT work by yourself toward the L8→L9 step-up or any other game-design
  decision: brand it "awaiting director" and move on.

## Verification protocol — before ANY commit

1. `cmake --build build -j"$(nproc)"` — warning-free.
2. `ctest --test-dir build` — 2/2.
3. `./build/tests/bh_tests` — 105/105 (327,925 assertions), or the new
   baseline after your added tests (report the new number).
4. Replay **0 mismatches** for every journal/leg touched (fresh leg if the
   epoch bumped).
5. For map edits: the generator must be the source of truth — edit
   `tools/mapgen/*.py`, regenerate (`python3 tools/mapgen/make_thornwall.py`),
   commit the regenerated `.tmj` (CI diffs generator output), build rebuilds
   the `.bhmap`. Never hand-edit `.tmj`.
6. clang-format the touched C++ files (`clang-format -i <files>`).

---

## Work queue

Do S20 → S25 in order. Each item = full DoD (suites green, replay clean,
devlog, card moved to `docs/tasks/done/`, board updated, commit+pull+push).

### S20 — Close the programmable half of T-033 (M2 gate de-QA)

Card to create: `docs/tasks/done/T-033.md` scope = the agent-executable slice.

- **Soak:** launch `bh_server` (fresh DB, `--soak-secs 780`, `--record-world
  logs/t033.bwj`) + `bh_bots` with a **grinder mix** — profiles
  `wander|fighter|pilgrim|campaign` exist (no `grinder` profile; mix them),
  ~10–20 bots, 720 s. `tools/bots/bh_bots --help` shows the profile list.
- **Balance bands:** from the [soak]/[bots] lines, tabulate per-mob band
  sanity — actual TTK vs the T-030 density rule, no off-band mobs, deaths not
  clumped on one kind (killerByLvl). Reuse the devlog-0029 table style.
- **Wipe replay:** `--replay-world logs/t033.bwj` must be `OK … mismatches=0`.
- **Human trade pass is the one thing you CANNOT do** — reproduce it in the
  open-list as "remains for director"; do not fake it or skip-check it.
- Devlog `0031-m2-gate-deqa.md`. No epoch bump (no content change).

### S21 — Smugglers' Cove fence (closes T-056's named gap)

Card `T-069 — Fence at the Smugglers' Cove`. GDD §7; phase3-remainder line:
"Smugglers' Cove fence arrives with the content drop … refusal now, fence
later is the intended erasable gap." The gap is now overdue.

- **Fence NPC:** new furniture kind `kWireKindFence = 67` in
  `shared/content/wirekind.h` (64 vendor, 65 anvil, 66 bounty taken; art
  HANDOVER reserves 67–73 for NPCs). Place one `fence_hut` object in
  `tools/mapgen/make_thornwall.py` on a back-alley tile of town (e.g. the
  graveyard edge x[3..4] y[16..17], near the gallows) — reachable by chaotics
  without crossing a guard watch. Interact radius ≤ 3 (mirror the vendor,
  `world.cpp:915/950/993`, `kWireKindVendor`).
- **Semantics:** fence **buys junk/gear from anyone** but pays the fence cut
  (60% of `value` — Marta keeps `kSellRatioPct`); has a **secret 3-item stock**
  (potions + rare junk at a slight markup) visible **only to karma < 0**
  (reuse `World::karmaBandOf`, `bumpKarma`); lawful/neutral shoppers get a
  fiction line and refusal on the secret stock only. Trading with the fence
  changes no karma. Marta's chaotic-refusal (T-056) stays — do not regress it.
- **Wire/client:** reuse the vendor interaction paths server-side, branching
  on wireKind 67; client renders kind 67 with the existing furniture
  placeholder rect + name label (no new art), same way 64/65/66 arrive.
- **Tests:** fence pays 60% to chaotic, refuses secret stock to lawful,
  Marta still refuses chaotics, karma unchanged by fence trade, replay
  parity. **Epoch 7 → 8** (new furniture enters the world; same response as
  T-030/T-068) + fresh leg replay.
- Devlog `0032-smugglers-fence.md`.

### S22 — Day/night completion (two cards)

**T-070 — Blood Curse + chapel cure.** GDD §9; T-061/062 shipped night
buffs/economy but the debuff loop is "hook tag only" (and the client already
has the vfx kind-14 violet-eye slot reserved).

- **Curse:** `Entity.curseUntil` (ticks). The Gravecaller Blood Bolt
  (T-064) and the Gravemother bolt become the source: a bolt hit applies
  curse for 30 s (600 ticks). Effect (minimal, measurable): potions, Mend,
  and Bless heal for **75%** (25% penalty) while cursed; OOC regen untouched.
- **Confessor:** new furniture kind `kWireKindConfessor = 68` inside the
  chapel (thornwall, the (9,9)–(14,13) area); interact ≤ 3 clears
  `curseUntil` with a fiction line. Karma repentance is **out of scope** (a
  later card).
- **Client:** debuff-strip entry (violet eye) read from the entity snapshot;
  confessor name label. Reuse the existing strip, wire, and furniture
  rendering paths.
- **Tests:** curse applied on bolt hit, potion/Mend/Bless 75% pin, chapel
  clears, exact expiry at 600 ticks, bolt-cure round-trip, replay parity.
  **Epoch 8 → 9** (new furniture + effect) + fresh leg.
- Devlog `0033-blood-curse.md`.

**T-071 — Night light: torch + Blessed lantern + night-only spawns.**
GDD §9, greenfield (no `lightRadius`/`torch`/`nightOnly` exists anywhere).

- **Server:** `Entity.lightRadius` (u8, 0 default). **Torch** = new
  consumable (slot 2, `shared/content/items.h`, next free id, value ~8 g):
  `/use` grants `lightRadius 6` for 300 s (add a journaled `kUse` command —
  follow the `kDuel`/`kTrade` command pattern, journaled exactly, replay-safe).
  **Blessed Lantern** = a consumable that never expires while held: `/use`
  toggles `lightRadius 8` on/off. Potions/gold economy unchanged.
- **Client render:** at night, draw a radial light mask from the light-source
  tiles (own player + any lit entity in AoI) on top of the world, **but never
  below the documented tint floor** (alpha floor 150 — legibility is a locked
  pin, T-062; horror comes from content, not darkness). No wire change to the
  mask — it is render-only.
- **Night-only spawns:** add `nightOnly u8` to `sim::SpawnDef` + the `.bhmap`
  wire in `shared/sim/bhmap.cpp` (± `bh_mapconv` validator and all map srcs
  regenerated). A `nightOnly` spawner only refills/aggros inside the night
  window (deterministic from tick, same schedule as T-061's 21:00–05:00). Add
  one `night_ghouls` spawner to thornwall in a currently quiet patch
  (maxAlive 4) so bots see night variety.
- **Tests:** torch grants/expires at tick boundary, lantern toggle, light
  mask pinned constants, nightOnly refill gating at the hour edge, bhmap
  round-trip of the new field, replay parity. **Epoch 9 → 10** (new item,
  new sim field) + fresh leg.
- Devlog `0034-night-light.md`.

### S23 — Aura tiers III–V (T-072)

GDD §3 weapon mastery; RFC 0001 pinned tiers at skill **80/120/150** and the
"3-target multiattack tier"; T-042 shipped only I–II live (client already
renders +III/+IV/+V marks — `client/src/game.cpp:1058`).

- **Effects:** flesh out `shared/content/auras.h` rows III/IV/V. Proposed
  (era-flavored, numbers are tuning placeholders — validate then flag in the
  devlog): **III (80)** = 3-target multiattack cleave (the RFC anchor);
  **IV (120)** = cleave widens + crit channel tick; **V (150)** = lifedrain
  proc on cleave targets. Reuse the existing aura-proc wire kinds (1xxx
  cleave / 2xxx sunder / 3xxx graft per `client/src/game.cpp:476`) and the
  fx callout queue from T-066.
- **Gates:** find the current server gate (T-042's min-skill check + tier
  table) and extend to the three new tiers; persistence (auraTier in the item
  flags, persist v9) and refine-destroy retention (T-060) already exist.
- **Duel harness:** extend `tools/duel` with a gear/aura knob to prove tier
  III multiattack resolves 3 targets and stays replay-exact; add the
  solo-impossibility style gate numbers to the devlog.
- **Tests + soak:** unit pins per tier + a short fighter soak leg at
  weapon-skill ≥ 80 showing gain. **Epoch 10 → 11** if any sim semantics
  change (multiattack does) — else keep 10 and say why on record.
- Devlog `0035-aura-tiers-iii-v.md`.

### S24 — Gate guards + spawn-camp protection (T-073)

M3 exit (guard-murder) + M4 anti-grief list + Alpha hardening. Straightforward,
high value.

- **Guard mob:** new mob row 1011 `Gate Guard` (L15, hp ~400, dmg ~30, def
  ~18, xp 0, aggro 0, wander 0, leash 12 — aggressive only toward wanted
  players via a new `guard=1` flag on MobDef that makes aggro ignore players
  without `wantedUntil > tick_`). Place 2 guard spawners at Thornwall's east
  gate + bridge approach in `make_thornwall.py`.
- **Wanted state:** `Entity.wantedUntil` (ticks). A player who commits an
  unlawful PK (T-056's penalty path already detects this in
  `world.cpp:2072`) *within 8 tiles of a guard anchor* gets `wantedUntil =
  tick + 240 s`. While wanted: town vendors take the Marta rage line (refuse
  chaotics — already built) and **all** guard mobs aggro; death while wanted
  respawns at the gallows even if karma isn't red yet.
- **Spawn-camp protection (M4 item):** `Entity.spawnProtectUntil` — 5 s
  (100 ticks) after respawn, mob aggro skips the player (mob lookup ignores
  protected targets; the player may still fight back). Protects new/respawned
  players from instant corpse-camp death loops (devlog 0009's 75-death-loop).
- **Tests:** wanted set/expire ticks, guard aggro only on wanted, spawn
  protect suppresses mob aggro for exactly 100 ticks, gallows interplay
  (wanted respawns at gallows), vendor refusal while wanted, replay parity.
  **Epoch 11 → 12** (new mob + sim fields) + fresh leg.
- Devlog `0036-gate-guards.md`.

### S25 — Campaign follow-ups (no design decisions needed)

**T-074 — bad-leg death levers.** The handoff's open thread #3:
`v5c` still takes heavy legs (142/134/116 deaths on legs 3/5/7). Levers:
pack cap (`tools/bots/main.cpp:604`, `b.level >= 3 && pack >= 2`) and the
retreat/swarm thresholds (`swarmOnUs >= 5`, `main.cpp:578`). Bench = the leg-3
and leg-5 logs. Measure, then move one lever at a time (same 0025→0029
discipline), short soak per iteration, paste before/after tables. **Bot-only —
no epoch bump.** If the overnight budget runs dry, park it with a precise
"measured pit" note instead of a blind tweak.

**The L8→L9 step-up and anything adjacent to widow_glade/barricade
placement = director decisions. Do NOT implement. Leave the open-list wording
refreshed and a one-line "awaiting director (options in devlog 0030 scope
boundary)" note.**

---

## End-of-shift checklist (do this before you go)

1. Every card you completed: card file in `docs/tasks/done/` with evidence
   pasted, devlog written (one per sprint), board updated
   (`docs/tasks/README.md` — add a "Done — overnight" section + revisit the
   open-list), suite numbers (new totals) in the devlog.
2. All work committed (bloodhollow-dev identity) **and**
   `git pull --rebase` + `git push origin master`. Expect to be outrun by the
   art stream; rebase and re-push as needed.
3. Write a fresh handoff for the morning, following the shape of
   `docs/prompts/session-handoff-campaign-progression.md`, named
   `docs/prompts/session-handoff-overnight-<date>.md`: where HEAD is, what
   landed (table), the epoch number now, the suite + replay numbers, every
   judgment call you made (numbers you pinned, cards you parked), and the
   open director decisions.
4. Leave the arena brief untracked/untouched. No generated binaries staged.

## Stop conditions (never violate)

- Determinism break you can't root-cause in ~45 min: **park with artifacts**
  (recorded journal, `worldHash` mismatch line, failing test) and a
  "diagnostic needed" devlog note. Do not keep patching blind overnight.
- A wire-format change to `shared/protocol/messages.md`: bump
  `PROTOCOL_VERSION` and say so — that is allowed and expected, just never
  silent.
- Adding a dep, touching `assets/final/`, Windows code, or re-deriving a
  known wall (pack-density at the waypoint; barricade placement) is off-limits.
- If you finish the whole queue (unlikely), pick the next greenfield GDD item
  as a new card **only with an explicit scoping write-up first** — a card +
  devlog costs the same as a feature; don't drift.