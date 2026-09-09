# Engine extended shift — execution brief (S26–S36)

*Issued 2026-09-09 (day shift, long). Read `AGENTS.md` first, then
`docs/prompts/engine-overnight-brief.md` (the previous shift — its
environment facts and queue discipline still apply), then the morning
handoff `docs/prompts/session-handoff-overnight-2026-09-09.md`. Ground
state: HEAD `ca3575a` (T-074), pushed to `origin/master`. You are the only
engineer awake. Work the queue in order, one card = one sprint = one
devlog = one commit, full Definition of Done on each. Every card below is
fully scoped so you never have to make a game-design decision — pin
numbers, validate, and flag any judgment call in the devlog for the
director's review. This shift is budgeted LONG: prefer finishing S26–S31
solid over touching everything thin.*

## Where things stand (HEAD `ca3575a`, pushed)

- Overnight S20–S25 is **done**: T-033 (M2 de-QA) + T-069 (fence, epoch 8)
  + T-070 (curse, epoch 9) + T-071 (night light, epoch 10) + S23 aura audit
  (no bump) + T-073 (guards, epoch 11) + T-074 (lever measured, rejected,
  reverted — nothing shipped). Cards in `docs/tasks/done/`, devlogs
  `0031–0037`.
- Suite baseline to preserve: **134 tests / 328,092 assertions**, ctest 2/2,
  build warning-free (`-Werror`).
- Journal epoch is **11**. Wire version is **237** (see the count law below).
  `.bhmap` format is **v2**.
- Open on the board: **T-033 human slice** (trade-pass UX, human-only),
  **L8→L9 step-up** (DIRECTOR decision — do NOT touch), **bot levers round
  2** (measured pit in devlog 0037), **karma repentance** (deferred T-070),
  **guard-murder** (M3 exit, deferred T-073).
- Parallel workstreams to coexist with:
  - Art commits to `master` concurrently — **always `git pull --rebase`
    before push**; expect fast-forward races.
  - `docs/prompts/arena-parallel-eval-brief.md` is **untracked and belongs
    to another workstream — do not touch, move, or commit it.**
  - `docs/prompts/art-b1-b8-execution.md` (untracked) + the art stream's
    in-flight `tools/atlaspack/bh_mob_sheet.py` edits +
    `tools/atlaspack/b5_build.sh` (untracked) are likewise **not yours**.
    Never stage them.
- Git identity for this workstream (match the existing author convention):
  `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`.

## Hard-won environment facts (do not re-learn)

- Per-command timeout ~30 s. Any run that lives longer must be detached:
  `(setsid ./build/<thing> ... > logs/<run>.log 2>&1 < /dev/null &)` then
  poll with short greps. A full 12-leg chain takes ~1 h 50 min; leg cadence
  ~9 min. For per-card validation use **short soaks (540–600 s)** instead —
  do NOT spend the shift on chain-length runs unless a card says so.
- Soak shape (all four overnight legs): `bh_server` (fresh DB
  `/tmp/tNN.db`, `--soak-secs <bots+100>`, `--record-world logs/tNN.bwj`)
  + 14-bot grinder mix on one port (`wander ×5 / fighter ×4 / pilgrim ×3 /
  campaign ×2`, `--secs 540–600`). Reference: devlog 0032 table style.
- Resumed-DB validation (val pattern): copy a chain DB, then **set state via
  sqlite3** (`UPDATE characters SET level=?, xp=?, map_id=1, x=?, y=?...`) —
  do NOT resume a DB "as-is": saved positions park characters inside live
  aggro clusters and produce false death-loops (devlogs 0030, 0037).
  **`/tmp/*.db` does not survive a reboot** — if the template is gone,
  regenerate it: fresh pair → climb → sqlite-set (XP bars: `xpNext(L)` in
  `shared/sim/combat.h`; L2 ≈ 360, L3 ≈ 760). Same bot `--prefix` resumes
  the same characters.
- Bot A/B recipe (T-074 discipline, devlog 0037): same start state, same
  regime, one lever, before/after tables (deaths, killerByLvl, lastDeath,
  maxLevel, levelDrops, mend). **Revert-on-red**: a lever that triples
  deaths ships nothing — restore `tools/bots/main.cpp` to v5c, rebuild,
  record the pit. Bot-only work never bumps the epoch.
- `maxLevel` semantics: level at close (SUMMARY counter), NOT the peak. Peak
  = last `reached L<N>` line. Keep `peak`/`end` distinct in every table.
- Journal epoch: `constexpr int kJournalEpoch` in `server/src/main.cpp`.
  **Content changes that shift the sim under old journals bump the epoch**
  (…T-068 6→7, T-069 7→8, T-070 8→9, T-071 9→10, T-073 10→11; S23 audit did
  NOT bump). Bot-only and render-only changes must NOT bump it. When you
  bump, record a fresh soak journal and paste `[replay] OK …` in the devlog;
  old journals must refuse cleanly (exit 4 — a feature, not a failure).
- Wire version: `kProtocolVersion = 200 + message count` (auto-derived by
  `tools/protogen/protogen.py`; currently **237**). **Trailing field
  appends do NOT bump it** (S16 ItemSlot growth, T-070 OwnStats, T-071
  Spawn/Delta precedents) — the fleet deploys lockstep. Never bump it by
  hand; the generator overwrites. `.bhmap` has its OWN version law
  (`kBhmapVersion`, now **2**) and its loader is strict — that one DOES
  bump on spawner-record changes.
- Maps: the generator (`tools/mapgen/*.py`) is the source of truth — edit
  it, regenerate (`python3 tools/mapgen/make_thornwall.py`), commit the
  regenerated `.tmj` (other four must regenerate byte-identical). The build
  (`bh_maps` target) rebuilds all five `.bhmap` files; `.bhmap` files are
  git-ignored artifacts — never commit them.
- `.bwj` journal files ARE tracked (git add -f; they predate the
  `.gitignore` rule) — commit them as evidence.
- `clang-format` is version 22.1.8 but **HEAD is not clean under it** (comment
  reflow churn). Do NOT run whole-file `clang-format` — it rewrote 600+
  lines last shift and had to be surgically reverted. Match surrounding
  style by hand on touched lines only.
- The `Edit`/`Write` tooling occasionally eats newlines on whitespace-only
  replacements — never issue no-op edits; always change real content, and
  re-read the edited region after (a missing newline once deleted a struct
  member and a function signature in one shift).

## Standing constraints (AGENTS.md + inherited)

- C++20, no exceptions across module boundaries, no RTTI,
  `-Wall -Wextra -Werror`.
- Determinism: all gameplay RNG through `sim/rng.h` (xoshiro256**, seeded);
  no `rand()`/wall-clock in sim or server gameplay code. Night schedule is
  tick-derived. Fixed 20 Hz step, integer ticks/tiles.
- `worldHash` + journal/hash markers are the replay oracle; every new
  gameplay change must keep them deterministic and end-to-end replayable.
  (`worldHash` covers id/zone/pos/hp only — buff stamps, karma, light,
  wanted, protection ride along deterministically via journaled commands.)
- Server never trusts the client; all combat/trade/economy validated
  server-side.
- No new third-party dependency without an ADR. Never touch `assets/final/`.
- GDD conflict rule: **anything that contradicts shipped values keeps the
  SHIPPED values; update the GDD** (`docs/02-gdd.md`), don't renegotiate.
  Known live deviations to record when touched: party cap **8**
  (`world.h:236 kPartyMaxMembers`, GDD §6 says 5); refine 0–3 mercy law
  (GDD §7 says 90/80 + destruction at +2); Blood Curse 75%-heal (GDD §5
  MVP-lite says −10% stats); night base light 0 (GDD §9 says base 6 —
  see S36).
- Numbers pinned (do not silently renegotiate): `kLevelCap = 25`, 3 pts/level,
  XP curve in `shared/sim/combat.h`, karma clamp ±1000 (`world.cpp:2215`),
  whitening +1/3600 ticks (= +20/logged-hour), moral split lawful +15% XP
  (`awardXp`), GDD §5 PK formula −(300 + 20·deficit), aura gates at
  20/50/80/120/150, aura I +3 atk / II regen 2 / III cleave / IV sunder +35 /
  V graft, refine ceiling +3 (part + 50g, full-durability gate, 2→3 60%
  + destruction), Blood Curse 600 ticks / 75% / chapel `/confess` ≤ 3,
  torch 6 tiles / 6000 ticks / 8g, lantern toggle 8 / 150g, wanted 4800
  ticks / 8-tile anchor radius / leash-12 acquire, spawn protection 100
  ticks, guard row 1011 (L15, hp 400, dmg 30, def 18, xp 0, aggro 0, wander
  0, leash 12), night window 21:00–05:00, mask peak 90 < floor 150,
  wireKinds 64 vendor / 65 anvil / 66 bounty / 68 confessor / 69 fence
  (67 reserved twins; mobs < 64), vendor panel F1–F7 / fence F8–F10.
- Max diff < ~400 lines per commit (journals/logs excluded). Split bigger.
- Do NOT work toward the L8→L9 step-up, the siege, the war state, Blood
  Moon, or any other game-design decision: brand it "awaiting director"
  and move on.

## Verification protocol — before ANY commit

1. `cmake --build build -j"$(nproc)"` — warning-free.
2. `ctest --test-dir build` — 2/2.
3. `./build/tests/bh_tests` — 134/134 (328,092 assertions), or the new
   baseline after your added tests (report the new number).
4. Replay **0 mismatches** for every journal/leg touched (fresh soak journal
   if the epoch bumped; bot-only cards record regime journals, unreplayed).
5. For map edits: generator truth (see above). For client-only cards:
   attach the named screenshot or state why headless (no display) blocked
   it — never claim an unseen render.
6. Hand-format touched C++ lines (see clang-format warning above).

---

## Work queue

Do S26 → S36 in order. Each item = full DoD (suites green, replay clean
where applicable, devlog, card in `docs/tasks/done/`, board updated,
commit + pull + push). Longer shift: prefer S26–S31 SOLID over skimming
all eleven. Park honestly with a pit note rather than thin.

### S26 — Client enablement quick wins (T-ART-01 + T-ART-02 + T-ART-08)

Source: `docs/tasks/art-backlog.md` (pre-scoped; the pins below are quoted
from it — verify each line number before editing).

- **T-ART-01 — point filtering in `loadAtlas`.** `engine/assets/atlas.cpp:13`
  loads the texture without a filter while `placeholder.cpp:57` sets
  `TEXTURE_FILTER_POINT`. Add the one line after `LoadTexture`. Tests:
  existing atlas unit tests green; note headless screenshot blockage.
  No sim/wire/epoch impact.
- **T-ART-02 — snap wheel zoom to {1, 1.5, 2}.** `engine/render/camera_rig.h`
  wheel block (~:53–60: `zoom += wheel*0.25`, clamp 2.5, `Z` toggles 1↔2).
  Put an inline `snapZoom` in the header (testable — `bh_tests` already
  carries `${CMAKE_SOURCE_DIR}/engine` on its include path since T-071),
  snap the wheel result, keep `Z`. Tests: boundary pins (1.25→1.5,
  1.75→2, clamp edges). No sim/wire/epoch impact.
- **T-ART-08 — map cases 4/5.** `Game::mapFileFor` (`client/src/game.cpp:830`)
  knows 1–3; add `4: bonehowl_mine`, `5: drowned_crypt` (filenames from the
  server boot lines / `mapFileFor` pattern). Tests: zone-handoff walk
  1→4→5→1 if a display exists, else the case-table unit pin. No sim/wire.
- **Ledger flips (no code):** mark **T-ART-03 done-superseded by T-071**
  (additive mask + `DrawCircleGradient` pools shipped in devlog 0034 —
  quote it); mark **T-ART-06 partial** (68 confessor + 69 fence shipped with
  the generic furniture rect + label; 67/70/71/72/73 remain — future card,
  do NOT implement now). **T-ART-11 stays parked** — it needs refine +5,
  which does not exist until S32 lands; say so in one line.
- Three micro-commits (one per T-ART card) or one batch commit if each is
  collectively trivial — director prefers three; keep each < 30 lines.
  Devlog `0038-client-quick-wins.md`. No epoch bump.

### S27 — Client anim-state hook (T-ART-04, client-only)

The key art-visibility task: the client plays only walk/idle
(`game.cpp` draw paths); attack/cast/hurt/die frames never display.

- Per-entity render-side state machine keyed by authoritative events
  already on the wire (swing stamps, `CombatEvent` kinds incl. death,
  respawn/despawn → existing vestiges). Durations from 20 Hz tick stamps.
  Contact frame lands on the swing-resolution tick (bible §5 timing truth
  — the backlog's words; verify against `trySwing` event emission).
- Rendering-only, non-wire, non-sim: server stays authoritative; replay
  must stay bit-exact (mask/decal-class change — prove it with the standard
  replay line on any touched journal, or state that none was touched).
- Tests: duel-harness frame-vs-tick alignment unit where headless-testable;
  screenshots where a display exists (else the honest headless note).
- Devlog `0039-anim-hook.md`. No epoch bump.

### S28 — Client atlas table + party tint + anchorY (T-ART-05 + T-ART-07 + T-ART-10)

- **T-ART-05 — `atlasFor(wireKind)`** keyed by `mobs/<id>_<slug>/`, fallback
  to the placeholder hero atlas, furniture kinds to their sheets, missing
  frames keep the red-circle QA marker. Acceptance: spawn sweep 1001–1011 +
  furniture, no red-diamond fallback on any shipped row. (1011 Guard renders
  the fallback hero until its sheet ships — say so.)
- **T-ART-07 — party overhead tint** by priority: chaotic red > enemy-town >
  party green > neutral (client reads `snap.karmaBand` + party roster it
  already holds — no wire change).
- **T-ART-10 — `anchorY`** in the anim JSON (default 42), loaded by
  `loadAtlas`, used at the draw call (`game.cpp` hard-codes 42).
- All three render-only. Tests: table unit (every shipped kind resolves),
  tint-priority unit, anchor overlay note. Devlog `0040-client-atlas.md`.
  No epoch bump. Out of scope: T-ART-09 decals (parked — needs the
  render-to-texture layer; say so in one line).

### S29 — Karma repentance at the chapel (T-075)

Deferred from T-070 (which scoped it out). Fill the gap with derived pins:

- `/repent` chat verb → journaled command (append at the enum end in
  `server/src/command.h`, after `kConfess` — old journal kind-ints
  untouched), handled in `applyWorldCommand` → `World::repent`.
- Gate: standing within 3 of the confessor (`nearConfessor`, T-070 reuse) +
  **72000-tick (1 logged hour) cooldown** per player (new stamp
  `repentUntil`, buff-stamp precedent). Effect: **+20 karma** — exactly one
  whitening-hour (pinned rate, `world.cpp` whitening block) — clamped by
  `bumpKarma` (±1000, band-crossing events fire normally). Curse untouched.
  Fiction line on success; quiet fail otherwise.
- Amount and cooldown DERIVE from shipped pins (not invented) — flag the
  derivation for review. No karma repentance for the wanted (keep the lanes
  separate: wanted is gate law, repentance is chapel grace — if wanted,
  refuse with the gallows-bound line).
- Tests (`tests/test_repent.cpp`): +20 pin, clamp at ±1000, cooldown edge,
  proximity gate, wanted refusal, curse unmoved, `applyWorldCommand`
  round-trip. No epoch bump expected (journaled command, no tick/entity
  change, karma unhashed) — bump ONLY if a tick-coupled effect is found,
  with the rule cited. Short soak + replay to be safe.
- Devlog `0041-repentance.md`. Card `docs/tasks/done/T-075.md`.

### S30 — Bot levers round 2 (T-077, the 0037 pit — bot-only, no epoch bump)

Execute the measured pit in order, same A/B recipe (L2-parked pair at
(55,14)/(54,13), fresh-DB template + sqlite-set, 540 s, tables with deaths
/ killerByLvl / lastDeath / maxLevel / levelDrops / mend):

1. **Retreat threshold 5→4** (`swarmOnUs >= 5`, `tools/bots/main.cpp:789`) —
   untouched in round 1. One lever, before/after, revert-on-red.
2. **Repeat arms** (round 1 was n=1 pair each — chaos dominates; the
   0025→0029 discipline wants at least a second run before any claim).
3. **L1-naked variant** (strip inv/gold at set-time) to isolate the pack
   gate with the climb confound removed.
- Stop after the FIRST conclusive red or green; park the rest with numbers.
  Never stack levers. Devlog `0042-levers-round2.md`. Card
  `docs/tasks/done/T-077.md` (T-074 stays done-reverted).

### S31 — Guard-murder consequences (T-078, M3 exit)

The M3 exit names guard-murder; T-073 shipped the posts without it. Scope
with derived pins only:

- Killing mob 1011 (any killer kind — player or mob? **players only**,
  mirror the PK law) applies the existing GDD §5 stain via `bumpKarma`
  with deficit computed against the victim's level (15):
  `−(300 + 20·max(0, 15 − killer.level))` — formula + victim level, no new
  numbers — plus `wantedUntil = tick + 4800` (reuse: the victim's own anchor
  is within 0 ≤ 8 tiles by construction; route through the same code path
  as the PK wanted block, not a copy).
- Guards keep refill 1200 (no post-emptying design). No faction flags, no
  permanent marks, no vendor changes beyond the existing wanted refusal.
- Tests (`tests/test_guardmurder.cpp` or extend `test_guards.cpp`): stain
  math pin (L1 killer: −(300+280); L15+: −300), wanted set, fiction lines,
  mob-kills-guard (no stain — players only), replay-shape via command lane.
  No epoch bump expected (existing fields, deterministic) — cite the rule;
  short soak + replay to be safe.
- Devlog `0043-guard-murder.md`. Card `docs/tasks/done/T-078.md`.

### S32 — Refine +4 to +7 (T-079, GDD §7 shape, shipped spirit)

Current law (`tryRefine`, `world.cpp:1312`): ceiling +3, 0→1/1→2 mercy,
2→3 60% + destruction, 1 junk part + 50g, full-durability gate. GDD wants
+0..+7 but contradicts the shipped rows (90/80 vs mercy, −1 vs destruction
at +2) — **shipped rows are frozen**; extend only:

- 3→4: 65% · 4→5: 50% · 5→6: 35% · 6→7: 25% (GDD-literal rates).
- Failure: −1 refine level; +7 failure resets to +0 (GDD-literal).
- Costs frozen (1 part + 50g), durability gate frozen, gear-only frozen.
- Fiction lines per step (mirror the "no hotter coal" voice). The refine
  value already rides `ItemSlot.refine` — **no wire change**; the +5 glow
  render is T-ART-11's job (unblocks it — say so).
- New RNG strata (refine rolls) → **epoch 11 → 12** + fresh soak journal +
  replay line. Tests: each transition pin, failure-law pins, ceiling
  message, fixed-seed determinism, replay parity.
- Devlog `0044-refine-four-seven.md`. Card `docs/tasks/done/T-079.md`.

### S33 — Trade transaction log (T-080, GDD §7 dupe audits)

- On the successful swap in `World::tradeCommit` (`world.cpp:1607`,
  post-validation, post-swap): append ONE line per executed trade to an
  append-only file (`logs/trades-<date>.log` — committed as evidence, same
  class as soak logs): tick, both entity ids + names, item list, gold both
  ways. Cancelled/atomic-failed commits append NOTHING.
- File I/O only on executed trades (rare — no hot-loop concern). No sim
  effect, outside the journal → **no epoch bump, no replay impact** (prove
  the latter with the standard replay line).
- Tests: committed trade appends exactly one parseable line; oversell/cancel
  append nothing; order of fields pinned. Devlog `0045-trade-log.md`. Card
  `docs/tasks/done/T-080.md`. Out of scope: any analytics/dupe-hunting UI
  (the log enables it later).

### S34 — Durability on death −5 (T-081, GDD-literal)

- GDD §7: "items lose durability on death (5)". In `killPlayer`, after the
  chaotic-drop removal: every surviving gear slot (slot ≤ 1) −5 durability,
  floor 0 (dormant per T-058 law, never destroyed). Junk/consumables
  untouched. Order pinned: drops first, durability second (deterministic).
- Tests: −5 pin, floor pin, junk/consumable exclusion, chaotic interplay
  order. No epoch bump expected (deterministic arithmetic, no new strata —
  cite the rule) + short soak + replay to be safe.
- Devlog `0046-death-durability.md`. Card `docs/tasks/done/T-081.md`.

### S35 — Cultist Purify, channel 9 (T-082, GDD kit list)

- GDD §6 lists Cultist **Purify**; channels 1–8 are all taken
  (`trySkill` dispatch: 1 swing, 2 Mend, 3 Bless, 4 Ironskin, 5 Firebolt,
  6–8 choir block). Add channel 9: widen the `skill <= 8` gate, grow
  `chUnlock` to 10 in `shared/content/kits.h` (all three rows — Ravager 0,
  Gravecaller 0, Cultist pinned below), extend `kitSkillUnlock`'s
  `channel >= 9` guard to `>= 10`.
- Cultist unlock **6** (utility-tier parity with Ironskin — flagged
  derivation). Cost/cooldown/range/targeting **mirror Mend exactly**
  (8 MP, 25t CD, 6 tiles, self-or-party via `choirTarget`) — stated, not
  designed. Effect: clears `curseUntil` (−1) on the target + chapel-family
  fiction line. No client key (precedent: 6–8 ship without keys — verify
  how 6–8 are triggered and mirror it; if chat verbs, add `/purify` the
  same way, journaled).
- Tests (`tests/test_purify.cpp`): clear pin, gates (MP/CD/range/kit/level),
  curse-only (does not heal, does not touch bless), round-trip. No epoch
  bump expected (existing field, deterministic) — cite the rule; short
  soak + replay to be safe.
- Devlog `0047-purify.md`. Card `docs/tasks/done/T-082.md`.

### S36 — Night base light: DIRECTOR OPTION (T-076 — price it, do NOT build)

GDD §9 says base light radius 6; the tree ships 0 (T-071). Shipped-wins
says keep 0 + update the GDD — but that guts the torch (radius 6 =
torch). Do NOT implement either. Write devlog `0048-base-light-option.md`
pricing both: **A** (GDD-literal base 6 at night — torch becomes
duration-only, lantern still +2, mask constants unchanged, sim content
change → epoch bump, fresh leg) vs **B** (keep shipped 0, one-line GDD §9
fix, zero code). Code pointers for A
(`Entity.lightRadius` default `world.h`, `useItem` torch branch,
`lightmask.h` falloff). No code change, no bump, no tests. Card
`docs/tasks/done/T-076.md` marked DECISION (awaiting director).

---

## End-of-shift checklist (do this before you go)

1. Every card completed: card file in `docs/tasks/done/` with evidence
   pasted, devlog written (one per sprint — 0038→0048 as used), board
   updated (`docs/tasks/README.md` — extend the "Done — overnight"
   sections in the same style), suite numbers (new totals) in each devlog.
2. All work committed (bloodhollow-dev identity) **and**
   `git pull --rebase` + `git push origin master`. Expect to be outrun by
   the art stream; rebase and re-push as needed. NEVER stage the
   arena-brief, art-brief, `b5_build.sh`, or `bh_mob_sheet.py` changes.
3. Write a fresh handoff following
   `docs/prompts/session-handoff-overnight-2026-09-09.md`, named
   `docs/prompts/session-handoff-extended-<date>.md`: HEAD, landed table,
   epoch now, suite + replay numbers, every judgment call, open director
   decisions (L8→L9 STILL, base-light NEW, plus siege/war/Blood-Moon NEVER).
4. Leave the arena brief untracked/untouched. No generated binaries staged.

## Stop conditions (never violate)

- Determinism break you can't root-cause in ~45 min: **park with artifacts**
  and a "diagnostic needed" devlog note. Never patch blind.
- A wire-format change to `shared/protocol/messages.md`: trailing field
  appends keep version 237 by the count law (S16/T-070/T-071 precedent) —
  just say so. A NEW message would move it (200+count) — also just say so.
  Never hand-edit `shared/protocol/gen/` (unused mirror; the build
  regenerates into `build/generated/`).
- A `.bhmap` loader failure on zones 2–5 means a stale artifact: the build
  re-syncs all five since T-071 — `cmake --build` again before panicking.
- Adding a dep, touching `assets/final/`, Windows code, re-deriving a known
  wall (pack-density at the waypoint; barricade placement; the rejected
  pack-gate drop), or starting the siege/war/Blood-Moon/L8→L9 without the
  director is off-limits.
- If you finish S26–S36 early (unlikely), the next greenfield item needs an
  explicit scoping write-up first — a card + devlog costs the same as a
  feature; don't drift. Suggested spares, scoped only: EK ledger needs
  Marrowgate (blocked); named elites need stat/timer design (blocked);
  T-ART-09 decals and T-ART-11 glow unlock after S28/S32 respectively.
