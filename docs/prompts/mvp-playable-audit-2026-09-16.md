# MVP-playable audit prompt — BLOODHOLLOW (2026-09-16)

> **Executable agent brief.** Goal: produce the definitive, evidence-backed answer
> to one question — **"What is left before the MVP is done and actually
> *playable* by a human who is not the director?"** — and turn that answer into a
> card-ready backlog with a P0 cut line.
>
> **Mode: AUDIT, not build.** You may run builds, servers, bots, tests, replays,
> sqlite queries and screenshots. You may **not** ship feature code, fix bugs you
> find, bump `kJournalEpoch`, touch the wire, migrate the schema, or merge
> anything. The only files you write are the audit artifacts in §12.
>
> **Branch lock (this Arena session): `arena/01a0a8c9-bloodhollow` only.** Never
> switch to, create, or push any other branch. One docs-only PR at the end.

---

## 0. Ground truth, in priority order

1. This prompt (procedure + output contract).
2. `docs/05-mvp.md` — **the MVP scope law** (§1 IN/OUT, §4 gate metrics, §6
   Friday-Night Test, §7 alpha checklist, §8 risks).
3. `docs/02-gdd.md` — the numbers (stats, kits, combat math, loot/enhance tables,
   party math, day/night, siege, §11 content inventory, §12 not-building list).
4. `docs/03-architecture.md` + `docs/adr/*` + `docs/rfcs/0001` — how it must be
   built (authority, determinism, persistence, protocol).
5. `AGENTS.md` — standing rules (C++20/no-exceptions-across-boundaries, no new
   deps, server never trusts client, 20 Hz integer ticks, RNG via `sim/rng.h`,
   epoch discipline).
6. **The code and the assets.** Everything above it is a *claim*; the tree is the
   *fact*. Where they disagree, the tree wins and the disagreement is a finding.

**Explicitly NOT ground truth:** `README.md §Status`, `docs/tasks/README.md`
board prose, `docs/devlog/*` narratives, `docs/04-roadmap.md` (self-declared
superseded), and any PR description. These are leads. Several are known-stale
(README still says "Next: Phase-4 pledges/siege spine" while T-131..T-140 shipped
pledges + siege). Cite them as *claims to test*, never as evidence.

### 0.1 Tree state at issue time (orient, then re-verify)

| Fact | Value (2026-09-16) | Re-verify with |
|---|---|---|
| HEAD | `2bd471d` "Merge repair: resolve committed conflict markers + epoch 27 + fresh leg" | `git log -1 --format=%H%n%s` |
| History depth | **1 commit** (squashed re-base; no archaeology available) | `git rev-list --count HEAD` |
| Session branch | `arena/01a0a8c9-bloodhollow` == `master` | `git branch -vv` |
| Journal epoch | **27** (`server/src/main.cpp:126`) | `grep -n kJournalEpoch server/src/main.cpp` |
| Gate leg of record | `logs/t146.bwj` (tracked) | `git ls-files 'logs/*.bwj' \| tail` |
| DB schema | **v14** (pledge vault) | `grep -n "user_version=" server/src/persist.cpp \| tail -3` |
| Protocol | generated; `kProtocolVersion = 200 + messageCount` (`tools/protogen/protogen.py:82`) → 237 at 37 messages | `grep -c '^message' shared/protocol/messages.md` |
| Zones loaded | 1..6 (thornwall, fields_overflow, thornwall_crypt, bonehowl_mine, drowned_crypt, weeping_castle) | `grep -n "assets/maps/" server/src/main.cpp` |
| Test files in build | 57 listed in `tests/CMakeLists.txt`; `tests/test_siege_stub.cpp` exists on disk but is **not compiled** | diff `ls tests/*.cpp` vs the CMake list |
| Open PRs | 8: #22, #23, #27, #28, #30, #49, #50, #51 | `gh pr list --state open` |
| Server LOC risk surface | `server/src/world.cpp` ≈176 KB, `server/src/main.cpp` ≈77 KB, `client/src/game.cpp` ≈67 KB | `wc -c server/src/*.cpp client/src/*.cpp` |

### 0.2 What "playable" means here (the acceptance target)

The audit answers three nested questions. Answer all three, separately:

- **P-1 Boots.** A competent stranger on a clean Linux box (and separately a
  clean macOS arm64 box) can go from `git clone` → build → server up → client in
  world, with no console editing and no tribal knowledge (MVP §7 last box).
- **P-2 Loops.** One human can play a 30-minute self-directed session and touch
  every MVP §1 IN row: fight, loot, equip, level, assign stats, pick a class,
  party, vendor, repair, refine at the anvil, mine ore, trade, die and lose XP,
  go red and drop, cross zones, see night fall and need light, join a pledge,
  and take part in a siege — **and understand what is happening without reading
  source** (§7 Phase E legibility).
- **P-3 Holds.** The Friday-Night Test (MVP §6, 4 legs) is runnable end-to-end
  with 10–30 humans + bot backfill, and the M1–M5 metrics (MVP §4) have
  non-stale evidence on *today's* tree.

Verdict vocabulary for the report header: `PLAYABLE`, `PLAYABLE-WITH-CAVEATS`,
`NOT-PLAYABLE`, each plus a P0 count and an estimated number of agent sessions to
clear P0.

### 0.3 Severity rubric (use exactly this)

| Sev | Meaning | Test |
|---|---|---|
| **P0** | Blocks "playable" for a real human. | Friday-Night leg fails, or a stranger cannot boot/play/understand the core loop, or data loss/dupe/crash on the alpha path. |
| **P1** | MVP §1 IN row materially incomplete or illegible; playable but the genre fantasy leaks. | A promised verb exists server-side but a player cannot see/drive/trust it. |
| **P2** | Spec drift, missing polish, missing evidence, ops friction. | Fixable in a session; alpha could ship with it documented. |
| **P3** | Hygiene: docs truth, dead files, repo/process debt. | No player impact. |

### 0.4 Status vocabulary (one per requirement row)

`SHIPPED` (code + test + legible) · `SHIPPED-INVISIBLE` (server truth, client
can't see/drive it) · `PARTIAL` (some sub-requirements) · `MISSING` · `STALE-DOC`
(docs claim it, code doesn't) · `DEFERRED-BY-DECISION` (named ADR/card/GDD cut —
not a gap) · `UNVERIFIED` (could not build/run; say why) · `SCOPE-CREEP`
(shipped but on the OUT list).

Every `SHIPPED` needs a probe. Every `PARTIAL`/`MISSING` needs a `file:line` or
an empty-grep result. **No row may be filled from memory or from a devlog.**

---

## 1. Evidence law

1. **Probe discipline.** Each requirement row carries: `probe` (exact command),
   `observed` (paste the decisive line(s), trimmed), `verdict`, `sev`, `owner`
   (agent / director / both), `effort` (S ≤ 1 session, M 2–4, L 5+), `card`
   (existing `T-*` if one covers it, else proposed id).
2. **Three probe classes**, in preference order:
   - **Runnable** — ctest case, bot leg, replay, live server + client, sqlite
     query. Strongest.
   - **Structural** — `grep -rn` / `wc` / file existence / CMake target list.
     Proves presence, never proves behaviour.
   - **Documentary** — a card or devlog. **Never sufficient alone.**
3. **Empty results are evidence.** `grep -rn "rarity" server shared client` →
   no hits is a `MISSING` finding, pasted as such.
4. **Do not fix.** Findings become cards. If a one-line fix is *needed to
   continue the audit* (e.g. a build blocker), record it as `AUDIT-UNBLOCK-n`,
   keep it in a separate scratch commit clearly labelled, and list it in the
   report's "audit deviations" section. Nothing else gets touched.
5. **Never invent.** No hypothetical slash verbs, flags, tables or file paths.
   If you're unsure whether something exists, grep it and paste the result.
6. **Timestamp evidence.** Every runnable probe records date + HEAD + build dir +
   the command line, so a later session can tell stale evidence from fresh.
7. **Determinism discipline while probing.** Recording/replaying is allowed;
   changing sim semantics is not. If a replay mismatches, that is a P0 finding —
   report it, do not "fix" it by bumping the epoch.

---

## 2. Environment bring-up (do this first, report what happened)

The audit sandbox is **not** the director's machine. Known realities:

- `cmake` / `ninja` are typically **absent**; `tools/bootstrap.sh` installs cmake
  via `pip` and pulls X11/GL dev headers with `apt-get download` + `dpkg -x` into
  `/tmp/x11prefix`, then configures **`build/`** (Release) and runs protogen.
- Network may be blocked (raylib is `FetchContent` by URL+hash; sqlite has a
  vendored fallback per `T-104`'s `third_party/CMakeLists.txt` note).
- There is usually **no display**: `bh_client` calls `InitWindow`, and no
  `xvfb` path exists anywhere in the tree. Graphical probes may be impossible.
- `assets/maps/*.bhmap` are gitignored and generated by the `bh_maps` ALL target
  (`tools/mapconv/CMakeLists.txt`) — a build creates them; a bare clone has none.
- `logs/*.bwj` is in `.gitignore`, yet 118 legs are tracked; **new** legs need
  `git add -f`. Note this as friction (P3), don't "fix" it.

### 2.1 Bring-up battery (run in order, paste outcomes)

```bash
cd /home/user/BloodHollow
git log -1 --format='%H %s' && git status --short | head
command -v cmake ninja g++ clang++ python3 pip || true
cat tools/bootstrap.sh                      # read before running
bash tools/bootstrap.sh 2>&1 | tail -30     # or the steps below, manually
```

Manual equivalent (use when bootstrap's assumptions don't hold):

```bash
pip install --quiet cmake ninja             # if absent
python3 tools/protogen/protogen.py shared/protocol/messages.md build/generated/protocol
cmake -S . -B build/headless -G Ninja -DBH_BUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/headless -j"$(nproc)"
ctest --test-dir build/headless --output-on-failure
ls -la assets/maps/                          # bh_maps must have produced 6 .bhmap
```

Then, **only if** X11/GL + a display (or `xvfb-run`, which you may install for
the audit but must not commit as a dependency) are available:

```bash
cmake --preset linux-gcc && cmake --build --preset linux-gcc -j"$(nproc)"
ctest --preset linux-gcc --output-on-failure
```

### 2.2 Degraded mode (mandatory fallback, not an excuse)

If you cannot build (no network, no toolchain, no display):

1. Say so loudly in §1 of the report, with the exact failing command + error.
2. Complete **every** structural probe (§3–§10 are ~70 % structural).
3. Mark all runnable-only rows `UNVERIFIED (no build: <reason>)` — never guess.
4. Still deliver the full ledger, the P0 list and the cards. An unverified audit
   with honest gaps beats a confident one with invented results.
5. Add a `RUN-FIRST` appendix: the exact battery the director should run on a
   real machine to convert your `UNVERIFIED` rows into verdicts, in order, with
   expected outputs.

### 2.3 Live-run hygiene

- Scratch DBs only: `--db /tmp/audit_<name>.db`. **Never** point a probe at a
  real/alpha DB. Siege rehearsals write holder state (`docs/ops/gm-runbook.md §4`).
- Unique ports per probe (7831+ are used by leg scripts); `pkill -x bh_server`
  and `pkill -x bh_bots` before and after each run.
- `BH_NO_AUDIO=1` for any client run in a sandbox.
- Capture server stdout to `logs/audit_<probe>_server.log` (scratch, untracked)
  and quote the decisive lines in the report. Do not commit new logs unless the
  report needs a leg of record — and then only with the director's OK.

---

## 3. Phase A — Build / boot / clean-machine battery

Objective: P-1 (§0.2). Each row: probe → observed → verdict.

| # | Check | Probe | Pass condition |
|---|---|---|---|
| A1 | Headless configure+build | §2.1 headless cmake | exit 0, zero warnings (`-Wall -Wextra -Werror`) |
| A2 | Graphical build (Linux) | `cmake --preset linux-gcc && cmake --build --preset linux-gcc` | exit 0 |
| A3 | macOS build | CI run on `macos-latest` for HEAD, or `macos-arm64` preset locally | green; paste the run URL |
| A4 | Test suite | `ctest --test-dir build/headless --output-on-failure` | 100 %; record test count + assertion count; compare to the last claimed number (298 at `2bd471d`) |
| A5 | Map artifacts | `ls assets/maps/*.bhmap` | **6** files, all newer than their `.tmj` |
| A6 | Map determinism (CI's own gate) | `python3 tools/mapgen/make_thornwall.py --out /tmp/t.tmj && diff -q /tmp/t.tmj data/maps-src/thornwall.tmj` | identical — **then repeat for all 6 generators** (CI only checks thornwall; that asymmetry is itself a finding) |
| A7 | Map validation | `./build/.../bh_mapconv data/maps-src/<each>.tmj /tmp/<each>.bhmap --map-id N --validate` | 6/6 validate clean |
| A8 | Portal/link integrity | `python3 tools/mapgen/validate_links.py` | exit 0; note which maps it actually covers |
| A9 | Server boots | `bh_server --db /tmp/audit_boot.db --port 7841 --soak-secs 20` | boot line lists 6 zones, epoch 27, registration posture; clean exit |
| A10 | Bots connect | `bh_bots --port 7841 --count 5 --secs 10 --profile wander --prefix aud_` | 5/5 online, 0 error lines |
| A11 | Journal round-trip | `bh_server --record-world /tmp/aud.bwj` + a bot wave, then `bh_server --replay-world /tmp/aud.bwj` | `mismatches=0` |
| A12 | Epoch guard | `bh_server --replay-world logs/t140.bwj` | refuses, **exit 4** (epoch 26 leg under epoch 27) |
| A13 | Leg of record | `bh_server --replay-world logs/t146.bwj` | `mm=0` on a fresh checkout — proves the committed leg matches the committed tree |
| A14 | Offline client mode | `bh_client --map assets/maps/thornwall.bhmap --max-ticks 60 --shot /tmp/aud_off.png` (needs display) | window opens, screenshot non-empty |
| A15 | Online client | `bh_client --server 127.0.0.1 --port 7841 --name auditor` | in-world, HUD drawn, chat works |
| A16 | **Clean-machine script** | Write (in the report, not the repo) the exact command list a stranger needs from `git clone` to in-world, then **execute it verbatim in a fresh temp clone** | zero undocumented steps; every missing step is a P0/P1 finding |
| A17 | Bootstrap vs presets vs leg scripts | `grep -n "build/" tools/bootstrap.sh tools/t146_merge_repair_leg.sh README.md` | reconcile: bootstrap builds `build/` (Release), README documents `build/linux-gcc`, leg scripts hard-code `build/linux-gcc`. Decide which is law; the mismatch is a finding |
| A18 | CI coverage vs reality | read `.github/workflows/ci.yml` | list what CI does **not** gate: replay legs, soak, headless preset, client run/xvfb screenshots, 5 of 6 mapgens, macOS map validation, warnings on vendored deps. Each omission → verdict |

---

## 4. Phase B — MVP §1 IN ledger (the scope law, row by row)

Decompose **every** row of `docs/05-mvp.md §1 IN` into atomic requirements with
stable ids (`B<row>.<n>`) so cards can cite them. Below is the decomposition to
start from — **extend it where the GDD implies more, and verify each line.**

### B1 World — "4 outdoor/dungeon maps + castle map"

- B1.1 Six zones exist as `.tmj` + generated `.bhmap` (ids 1–6) and load in one
  process (`grep -n "assets/maps/" server/src/main.cpp`).
- B1.2 **The client can render all six.** `client/src/game.cpp` `mapFileFor()`
  has cases 2–5 and `default: thornwall` — **verify whether case 6
  (weeping_castle) exists.** If not, a human walking into the siege stage sees
  Thornwall geometry: P0 for Friday-Night leg 3.
- B1.3 Portals work both ways for a *human* client (not just bots): walk each
  link, log the zone handoff (`Welcome` reuse, T-037).
- B1.4 Map names/level bands match GDD §1 + the MVP row (Thornwall, Churchyard &
  Bleak Fields, Bonehowl Mine, Drowned Crypt, Weeping Castle). Note any GDD map
  that is really two shipped zones (fields_overflow, thornwall_crypt) and whether
  that satisfies or deviates.
- B1.5 Marrowgate: OUT as a city sim, IN as red-named EK targets in the field
  war. Verify what actually exists (`kTownMarrowgate`, guard_synod, `/oath`) and
  whether a player can *experience* the town war at all.

### B2 Classes — "Ravager, Gravecaller, Cultist — kits per GDD §3, cap 25, 3 pts/level"

- B2.1 Three kits exist (`shared/content/kits.h`) and are selectable **in-game
  by a human**: how? (`/kit <name>` chat verb — verify; is there any client UI?).
- B2.2 Class choice at character creation: does it exist? (Probe
  `loginOrCreate`: one character per account, `classId` default 1.) If class is
  only a post-hoc `/kit` oath, decide whether that satisfies "kits per GDD §3"
  and record the deviation.
- B2.3 Skill channels shipped vs GDD §3 lists. Shipped: ch1 Power Swing, ch2
  Mend, ch3 Bless, ch4 Ironskin, ch5 Firebolt, ch6 Chorus, ch7 Mass Mend, ch8
  Haste, ch9 Purify. GDD lists ~6 Ravager + ~7 Gravecaller + ~10 Cultist skills.
  Produce the **missing-skill table** (Sunder, Bull Rush, War Stomp, Executioner,
  Second Wind, Frost Spike, Wither, Corpse Explosion, Terror, Blood Bolt as a
  *player* skill, Mana Shield, Curse of Weakness, Raise Skeleton, Sanctuary,
  Resurrect) with, per row: exists? (grep) / wired to a hotkey? / legible? /
  severity. **Resurrect at 20 is called "the reason every party wants one" in the
  GDD — its absence is a pillar-level finding.**
- B2.4 Level cap 25 (`shared/sim/combat.h:21 kLevelCap`) and the XP curve
  `100·L^1.85` — verify formula in code and spot-check L1→2, L10→11, L24→25.
- B2.5 3 stat points/level, never auto-assigned (`kStatPointsPerLevel`), and
  **which stats can be assigned**: `World::assignStat` (`server/src/world.cpp:338`)
  accepts 0/1/2 = STR/VIT/DEX only; `messages.md` says "INT/MAG land with casters
  in a later phase"; GDD §3 demands six stats (STR VIT DEX INT MAG **CHA**) with
  CHA driving aura radius, pet slots, pledge creation and vendor prices. Findings:
  CHA absent, INT/MAG unassignable, kit seeds are the only source. Verify each
  with greps (`grep -rn "\bcha\b" server/src shared` etc.) and grade.
- B2.6 No respec in MVP — confirm nothing shipped one (SCOPE-CREEP check).
- B2.7 Weapon mastery + auras (GDD §3 "Soma adoption"): `shared/content/auras.h`
  5 tiers, sword family only. Verify: skill gain by use (`sword_skill`,
  `swing_lands`), aura claim path (Bonesmith? which NPC/verb?), tier effects
  live (I–V), other weapon families (are there any?).

### B3 Combat — "server-auth 20 Hz, hit/crit/stagger, potions, gore decals"

- B3.1 All combat resolved server-side; client only renders (probe
  `trySwing`, `applyWorldCommand`, and the client's `sendAttack`).
- B3.2 Hit/crit/miss/kill formula matches GDD §4 (`hitChance` clamp, crit,
  stagger). Verify numbers, not vibes; paste `shared/sim/combat.h`.
- B3.3 Swing/cast cadence deliberate (tick constants), chase is server-auth.
- B3.4 Potions: `useItem` by id, stack caps, chug timing; vial economy reachable
  from a vendor.
- B3.5 Gore decals: `engine/render/decals.h` + T-ART-09 — do blood decals
  **persist ~10 min** as GDD §9/bible claim, or is it a 0.6 s vestige? Verify in
  code and grade the delta.
- B3.6 Telegraphs readable by a human: slam wind-up, bolt cast, boss phases
  (T-091). Client-side evidence required (callouts/decals/anim), not just sim.

### B4 Progression — "XP debt + de-level, mob/elite/named/boss tiers, bounty board"

- B4.1 Death → XP debt 10→25 % of bar, de-level at 0 (T-025): verify curve in
  code + a live probe (die, relog, confirm persisted).
- B4.2 Tier multipliers: elites ×8, nameds ×20, boss ×100 vs `kMobs` xp values —
  recompute from the table and flag drift (e.g. 1010 xp 3200 at L12 vs 1008 820).
- B4.3 Bounty board: `kWireKindBounty` 66, T-065 session-scoped quarry, **no
  persistence by design**. GDD §12 allows "quests = bounty board only:
  kill-N/pickup-N". Verify: kill-N yes; **pickup-N**? persisted across restart?
  legible in client (panel? prompt? nothing?)? Grade.
- B4.4 Named-elite timers 15–60 min rotating (Old Maw/Red Widow/Cantor Vex) +
  world-announced first kill. Verify timers, announce path, and that a *client*
  sees the announcement.

### B5 Party — "5-man, XP bonus curve, contribution share (heals count), party frames"

- B5.1 Cap 5, invite/accept/leave/kick, despawn sweep (T-050).
- B5.2 XP pool `mobXP·(1+0.12(n−1))`, 50 % even / 50 % contribution with heals at
  weight 1.0 and damage-taken 0.5 (GDD §6). Verify the split code + radius
  (12 tiles).
- B5.3 M3 metric "party ≥1.4× solo total XP/hr" — find the evidence, its date,
  and whether it still holds on epoch 27 (likely stale → re-run or mark).
- B5.4 Party frame UI: roster panel, leader star, live HP bars, off-zone greying
  (T-052) — screenshot or `UNVERIFIED`.
- B5.5 8-man parties must NOT exist (OUT list).

### B6 The Anvil — "Refine +0..+7 with table GDD §7; ore mining; durability/repair"

- B6.1 Refine cap +7 (`world.cpp:2114`) and the chance table
  `kRefineChance[7] = {100,100,60,65,50,35,25}` (`world.cpp:2153`) vs GDD §7
  `{+1 100, +2 90, +3 80, +4 65, +5 50, +6 35, +7 25}` → **+2 and +3 drift**.
- B6.2 Failure law: `refine==2` fail → **SHATTER** (`world.cpp:2162`);
  `refine==6` fail → reset to 0 (`2167`); else −1. GDD says +4/+5/+6 fail = −1
  level, +7 fail = reset to +0, and **no shatter anywhere in §7**. Grade the
  deviation: is it a deliberate tuning call with an ADR/card, or silent law
  drift? (AGENTS.md: "If the task conflicts with the GDD … stop and flag it.")
- B6.3 Toll: monster part + 50 g + durability per attempt (GDD: **Blackiron Ore
  with purity 1–10 + a fodder accessory + gold**; ids 5001 ore, 5101–5103 fodder
  reserved in `docs/art/40-items-icons.md`). Verify: does refine consume *ore*
  (5001) or any junk part? Does **purity** exist? Do **fodder tiers** exist
  (±4 %/+5 % modifiers)? Each absence is a row.
- B6.4 Ore mining: `spawnOreNodes()` (zone 4) + `tryMine` + `/mine` +
  `kWireKindOreNode` 74 + equipped pick (2003). Verify a human can find a node,
  mine it, and see the ore land in the bag; check yield vs the refine demand
  (M3 metric: "+5 ≈ 45–60 min of farming").
- B6.5 Durability: burn on swing/hit, −5 on death, −10 on refine, dormant at 0,
  `/repair` at Marta (T-058). Verify the numbers against GDD §7 and that the
  client shows durability (`ItemSlot.durability` exists — is it drawn?).
- B6.6 Glow from +5 (GDD) — `glowTier` on `EntitySpawn`/`EntityDelta` +
  `engine/render/refine_glow.h` (T-ART-11). Verify tiers (1 = +5..9, 2 = +10+)
  against the +7 cap: **is tier 2 reachable at all?**
- B6.7 Anvil UI: client panel exists (T-048). Screenshot or UNVERIFIED; check the
  panel states the real odds (does the player know +3 is 60 % and can shatter?).

### B7 Loot — "4 rarity tiers, ~40 affixes, ~12 boss/named uniques, player trade"

- B7.1 **Rarity tiers: none.** `grep -rn "rarity\|Rarity" server shared client tests`
  → verify empty. GDD §7 wants Common 78 % / Magic 17 % / Rare 4.6 % / Unique
  0.4 % with 1–3 affixes by tier; shipped law is "one affix, rolled at gear-drop
  time". Grade (likely P1: the loot fantasy is a pillar) and cost the gap.
- B7.2 Affix count: `kAffixCount = 10` (`shared/content/items.h`) vs MVP "~40".
  List the 10, verify each has a **live effect** (not flavour text) and that
  wrong-slot affixes are mute by design (v1 precedent).
- B7.3 Uniques: `kUniqueDrops` = 12 rows over 4 bosses (1009/1012/1013/1014) →
  matches "~12". Verify drop chances, titles broadcast, slot-law pins
  (`tests/test_uniques.cpp`), and that a human sees the epithet.
- B7.4 Gear slots: `ItemDef.slot` = 0 weapon / 1 armor / 2 consumable / 3 junk.
  GDD §7 wants weapon, shield, helm, armor, gloves, boots, belt, amulet, 2× ring,
  cape (11). Verify and grade — this constrains affix/rarity depth too.
- B7.5 Item count vs a 25-level career: count `kItems` rows; decide whether the
  ladder has enough steps (L1→L25 with 5 weapons?). Include vendor stock
  (`kVendorStock`) and fence stock (`kFenceStock`).
- B7.6 Player trade: window, commit-time validation, autosafe rollback
  (ADR-0011), audit log (`logs/trades.log`, T-080). Verify a **human** can drive
  it with the documented keys (T/P/X/H) and that the audit line is written.
- B7.7 Dupe surface: re-check the T-104/T-105/T-049x classes (trade offer
  replace-not-append, one shared inv-blob grammar, equipped/dormant/affix/refine
  survive relog). Run `tests/test_inv_blob.cpp` + `test_trade_equipped.cpp` and a
  live relog probe with a refined, affixed, equipped, near-broken item.

### B8 PK — "karma/alignment, chaotic drops, guards, duels, town-of-19 war + EK board"

- B8.1 Karma law: −(300+20d) on unlawful kill, bands (`karmaBandOf`: <0 chaotic,
  >500 lawful), whitening, gallows bind, refusals (T-056/T-057).
- B8.2 Chaotic item-drop risk on death — verify the drop rule and that it is
  *legible* to the player before they go red (nameplate colour, sheet stakes).
- B8.3 Guards: `kWireKindGuardAshen/Synod` 70/71, `guard` flag on mob 1011,
  wanted-only aggro, 240 s wanted mark (T-073/T-078). Verify a human can be
  caught and what happens (gallows? death? fence?).
- B8.4 Duels: `/duel`, `/forfeit`, `bh_duel` harness (T-034). Verify in-world
  offer/accept/refuse and that duel damage does not trigger karma.
- B8.5 Chapel: `/confess`, `/repent`, one whitening-hour per logged hour
  (T-070/T-075) + confessor NPC 68.
- B8.6 Town war: `/oath` at L19 (`kTownOathLevel`), EK fame on enemy-town kills
  (T-130), persisted (`town_id`, `ek`). Verify the whole chain **and** whether a
  player can ever meet an enemy-town player (is Marrowgate populated by anything
  but NPCs/guards? which mobs/players carry `kTownMarrowgate`?). If the war is
  unreachable in practice → P1.
- B8.7 EK board: MVP wants "EK leaderboards" and §7 wants an EK board *page*.
  Verify what exists server-side (`ek` column, read path) and whether any
  in-game or web surface shows it. No surface = SHIPPED-INVISIBLE.
- B8.8 Blood Curse (T-070) + Blood Moon (T-129): both live? Blood Moon is GDD
  §10 **[v0.2]** and not in the MVP IN table → check change control
  (SCOPE-CREEP row, or find the ADR/card that justified it).

### B9 Day/night — "darkness + light radius, night spawns, Blood Curse, Blood Moon"

- B9.1 4 h real = 24 h game; Day 120 / Dusk 30 / Night 90 / Dawn 30; HUD clock
  (`Welcome.hourCenti`, client clock draw). Verify phase lengths in code.
- B9.2 Darkness overlay: peak alpha 150 (≈59 %) at 04:00, floor documented; HUD
  stays untinted, world floaters are tinted (`docs/art/00-VERIFY.md` #7).
  Verify + decide whether the tinted-floaters issue is acceptable at alpha.
- B9.3 Light radius: GDD wants base 0, torch 6, lantern 8, Vigil extend — but
  GDD §9 also records "**shipped 0** per T-076/B". Establish the *actual* current
  law (`grep -n "light" server/src/world.h`, `engine/render/lightmask.h`,
  item 3003/3004 use paths, affix 8 Vigil) and whether night is playable or
  pitch black for a human. Torch burn-out + lantern toggle + a **light mask or
  painted pools** (T-ART-03) — which exists?
- B9.4 Night-only spawns: `SpawnDef.nightOnly` (bhmap v2) + aggro suppression by
  day (`world.cpp:3878`). Verify which spawners actually set it in the six
  `.tmj` files (`grep -c nightOnly data/maps-src/*.tmj` or the generator
  sources) and that the promised night mobs (Wraiths, Bloodfiends, +50 % XP)
  exist or are substituted (GDD names mobs that are not in `kMobs`).
- B9.5 Night economy: +10 % XP / +25 % relative drops after dark (T-062),
  nightcreep ×1.15 dmg / +1 aggro (T-061). Verify constants.

### B10 Siege — "weekly 90-min Weeping Castle siege (gates → Heartstone → crown channel), taxes, holder buff; bot-filled rehearsals"

- B10.1 Scheduler: Saturday 20:00, 90 min, registration closes 24 h prior
  (T-131). Verify the window law + `gm siege-start` + `--siege-rehearsal`.
- B10.2 Gates: 2 destructible gates, 100k HP, siege-damage skills ×3 (GDD §8) vs
  shipped `/breach` ram work on `kWireKindSiegeGate` 75 (T-132). Verify HP,
  damage law, and whether "siege-damage-type skills ×3" exists at all.
- B10.3 Heartstone → 60 s crown channel, interrupt = restart, complete = flip
  (T-133). Verify channel length, interrupt law, re-kneel guard, flip.
- B10.4 Taxes 0–15 % set by holder, collected hourly to pledge vault, Castle's
  Favor (+10 % XP & drops in territory), spawn shortcut (T-134 + T-140).
  Verify each of the four; note T-140 is **deposit-only** ("spending = Phase P")
  and that vault spend is therefore missing.
- B10.5 Bots fill sieges: `--profile siege` (T-136), M4 gate evidence flips 3/3,
  p99 6.6 ms @40 bots (devlog 0098). Verify the leg scripts still run on
  epoch 27 (`tools/t137_m4_leg.sh`) — **stale-gate risk is high after the merge
  repair**.
- B10.6 **Client legibility of the whole siege:** `grep -rn "siege\|pledge"
  client/src/` returned **zero hits** at issue time. If still true, a human at
  Friday-Night leg 3 sees: a castle map they may not even render (B1.2), gates as
  generic furniture, no HP bars, no holder, no countdown, no band roster, no
  crown progress — only chat lines. Grade this (expected P0/P1) and cost the
  minimum viable siege HUD (wire message + panels).
- B10.7 Castle map content: `data/maps-src/weeping_castle.tmj` is ~9 KB vs
  thornwall ~24 KB — verify it has a keep, gates, heartstone, throne, defensible
  geometry, and that 30 players fit (M4 used bots; check occupancy).

### B11 Pledges — "Lite: create/emblem/ranks/chat/vault(tax only)"

- B11.1 Create at registrar (kind 72, town square ≤3 tiles), gate **L≥10 +
  10 000 g** — recorded as a **GDD §8 deviation** (GDD wants CHA ≥20 + 100k).
  Verify the shipped gate and keep the deviation flagged (no CHA stat exists).
- B11.2 Emblem 0–9 placeholder: is it **visible** anywhere (over-head, roster,
  client)? `grep -rn emblem client/src` — expected empty → SHIPPED-INVISIBLE.
- B11.3 Ranks Liege/Bloodsworn/Initiate + `/pledge` verbs (create/promote/kick/
  leave/disband/who/vault) + pledge chat (`/p `). Verify each verb exists in
  `server/src/main.cpp` and is documented in the GM runbook / a player-facing help.
- B11.4 Vault: tax-only deposit (T-140), tithe kind 41, disband burns the pool,
  sworn-holder drip routing (name-keyed, offline-safe). Verify + note the
  deferred spend.
- B11.5 Bands: one pledge = one siege band, sworn callers muster members (T-139).
- B11.6 Persistence: `pledges` table + `pledge_id`/`pledge_rank` (schema v13/v14),
  membership restored on relog (T-122 leg).
- B11.7 OUT check: pledge XP/wars/storage must not exist.

### B12 Account — "4 chars/account, login, offline char slots"

- B12.1 **Character slots:** `Db::loginOrCreate` selects `... FROM characters
  WHERE account_id=? LIMIT 1` and creates one character named after the account
  (`server/src/persist.cpp:332,370`). Verify → expected finding: **1 char per
  account, no char-select, no offline slots**. MVP asks for 4. Grade (P1; blocks
  the "two parties of 5 from 10 humans" leg only if humans need alts — check
  leg-1 maths: 10 humans × 1 char = fine, so grade on player expectation, and
  note the alpha-invite implication).
- B12.2 Login: `Hello{protoVersion, username, password}` → `LoginResult{ok,
  reason}`; wrong version refused reason=4 (`main.cpp:329`).
- B12.3 **Password hashing is a stub:** per-account salted iterative FNV-1a,
  `stubPasswordHash`, with ADR-0009 + `persist.h` saying "replaced by argon2id
  before any public alpha wave". Verify nothing changed → **P0 for a public
  alpha** (and check the dependency rule: argon2 needs an ADR).
- B12.4 Rate limits (T-109): 30 new accounts/60 s/IP, 60 logins/60 s/IP, 10 bad
  passwords → 60 s lockout; `--no-register` gate. Verify constants + that the
  Friday-Night leg-1 brief (staggered logins) is still accurate.
- B12.5 Name law: uniqueness, charset, length, impersonation ("invalid name"
  reason 2). Probe with hostile names.
- B12.6 Session/reconnect: spawn-diff reconnect view (T-017), logout save
  canonical path (T-049x), crash-while-carrying-gold behaviour (probe: `kill -9`
  the server mid-session, restart, verify no dupe/loss).

### B13 Ops — "launcher+patcher, GM CLI, backups, crash reporting, bot soak suite"

- B13.1 **Launcher + patcher: does not exist** (`docs/ops/gm-runbook.md §6` says
  so; MVP §7 box open; director-owned). Verify, then state the minimum viable
  substitute for wave 1 (version manifest + download instructions + reason=4
  messaging) and who owns it.
- B13.2 GM surface: there is **no GM flag on accounts**; GM verbs are chat lines
  typed by the operator's character. Enumerate what exists (`gm siege`,
  `gm siege-start`, `gm siege-now`?, `gm blood-moon`) vs what MVP §7 requires
  (kick/ban/rollback/siege/announce + audit log). Known gaps: **no `/ban`, no
  free-text `gm announce`**. Verify both by grep and grade for alpha.
- B13.3 Backups: `tools/ops/bh_backup.sh` (backup/restore/drill/rotation/
  manifest) + a **drilled** restore (T-143 evidence: accounts=5 chars=5
  pledges=1 uv=14, integrity ok). Re-run `drill` on a scratch copy; paste output.
- B13.4 systemd: `tools/ops/bh-server.service` — `systemd-analyze verify` clean;
  auto-restart; journal capture; resource guards. Verify (in a container you may
  not be able to install the unit — then verify the file statically and say so).
- B13.5 Crash reporting: journald + `coredumpctl` posture, no in-process handler
  (documented as deliberate). Verify the runbook's weekly audit is actionable;
  check there is no crash-upload path (MVP §7 "crash reporting" — decide if
  journald-only satisfies it for a *remote* alpha where players are not on the
  host).
- B13.6 Bot soak suite: `tools/bots` profiles (wander, fighter, pilgrim,
  campaign, crypt, raider, siege, pledge) + leg scripts. Inventory them, mark
  which run on epoch 27, which are stale, and whether any is wired into CI
  (expected: none).
- B13.7 Observability: tick p99 export (`ServerStats`), `--soak-secs`,
  `--p99-budget-ms`, balancer TTK feed (T-031), `BH_DUMP_ENTS` probes. Are they
  enough to run an alpha night and notice a problem? List what's missing
  (metrics over time, alerting, disk-space cron per MVP §7).

### B14 OUT-list compliance (scope creep sweep)

For each OUT item, grep for evidence it shipped: vampire race, pledge XP/wars/
storage, +8..+10 scrolls, Blood Bibles, Crusade, second-town full sim, stalls/
auction, usage-based weapon %-skills (note: auras + sword_skill *are* shipped —
decide whether that contradicts "usage-based weapon %-skills" OUT row or is the
GDD §3 mastery spine), crafting beyond mining/refine, quests-as-content, 8-man
parties, Windows, localization. Verdict per row: `absent` / `present (creep)` /
`present (justified — cite card/ADR)`.

---

## 5. Phase C — GDD numeric delta table

Build one table: `requirement | GDD/MVP value | shipped value | file:line | verdict | sev`.
Minimum rows (add any you find):

1. Six stats + CHA effects (§3) → B2.5.
2. +3 pts/level, no auto-assign, no respec.
3. Skill counts per class (§3) → B2.3.
4. Combat math: hit clamp 0.55±, crit, stagger, damage terms (§4).
5. XP curve `100·L^1.85`, cap 25, ~60–90 h to cap (§6) — sanity-check with the
   measured campaign legs (T-090/T-100: plateau L6? L9 pace?) and say whether
   alpha pacing is achievable.
6. Mob XP tiers ×8/×20/×100 (§6) → B4.2.
7. Party pool + 12 %/sharer, 50/50 split, 12-tile aura radius, leash 18 tiles
   (§6) — verify leash constant (`kMobs` rows show 10–16; GDD says 18).
8. Rarity distribution 78/17/4.6/0.4 (§7) → B7.1.
9. Enhance table + odds + failure law (§7) → B6.1/B6.2.
10. Durability −5 death / −10 refine / unusable at 0 (§7) → B6.5.
11. Economy sinks: potions, repair, refine, **teleport scrolls**, guild upkeep,
    castle tax (§7). Verify each; `grep -rn teleport server shared` → expected
    empty. Missing sinks = gold-inflation finding (tie to the vial-flow/30-shop-
    trip pacing note in T-113/T-114).
12. Siege specifics: Saturday 20:00 UTC, 90 min, 2 gates @100k HP, ×3 siege
    skills, 60 s crown, tax 0–15 % hourly, Castle's Favor +10 % (§8) → B10.
13. Pledge creation CHA ≥20 + 100k (§8) vs shipped L≥10 + 10k → B11.1.
14. Day/night phase minutes + light radii (§9) → B9.
15. Named elite timers 15–60 min + the three nameds' bases/levels (§9) → B4.4.
16. §11 content inventory: tilesets 4+castle kit; player sprites 3 classes ×
    (m/f × 3 skin tones) = 18 sheets (D7 bake ×3); monsters 12 common + 2 elites
    + 3 named + 1 boss; NPCs 8; UI ~60 widgets/icons; VFX ~25; SFX ~40.
    **Count what exists on disk** (`find assets/aigen -name '*.png' | wc -l` per
    area: at issue time mobs 30, npcs 21, players 43, terrain 709, **vfx 0,
    icons 0**, palettes 18) and what the client actually loads.
17. Mob roster delta (§11): shipped `kMobs` = 14 rows (1001–1014). GDD names
    Marsh Rat, Plague Bat, Feral Ghoul, Bonepicker Gnoll, **Pale Cultist**,
    Hollow Hound, **Mine Wretch**, **Lantern Spider**, **Mud Golem**, **Crypt
    Revenant**, **Grave Banshee (elite)**, **Bell Ringer (summoner elite)**,
    Gravemother. Produce the missing/renamed table (incl. the D9 "Gravecaller →
    Waxen Celebrant" rename that `mobs.h` has not applied) and grade: does the
    alpha have enough variety for 3 zones + L1→25?
18. §12 not-building list → B14.

---

## 6. Phase D — The playable-loop walkthrough (P-2)

Run this as a **scripted human session**, in order, on a live server + real
client (or, in degraded mode, with bots + code reading, marking each step
`UNVERIFIED`). Record per step: `pass/fail`, what the player *sees*, what they
must *know* to do it, and any console/log help required. Screenshot or
`--shot` capture where a display exists.

**Setup:** fresh scratch DB, one server, 5 wander bots for ambience, one human
client (plus a second client/bot-as-partner where the step needs two players).

1. Launch → in world at Thornwall spawn. HUD legible at 1024×768? What is
   explained to a first-timer (no tutorial exists — verify)?
2. Movement: click-to-move (mouse) + WASD; camera follow/free-pan/zoom
   {1,1.5,2}; edge scroll. Feel = era-authentic?
3. Find a vendor (Marta, kind 64) by sight. Is she labelled? Is the shop panel
   discoverable without reading source (keys `F1..F12` to buy, `G` to sell junk)?
4. Buy a weapon + vials; open inventory (`I`); equip; confirm damage changed.
5. Walk to the fields; kill 5 mobs; watch callouts, blood, loot drop, XP floater.
6. Level up; assign a stat point (`F5/F6/F7`) — is the *meaning* of STR/VIT/DEX
   communicated anywhere in-client?
7. Pick a class: `/kit <name>`. Discoverable? Reversible? Any UI? What happens if
   a player types `/kit` with no argument or a wrong name?
8. Cast each unlocked kit skill from the hotbar (1–5, Q). Which channels are
   dead keys for which kit? (A dead key with no feedback is a P1 legibility bug —
   verify what the client shows.)
9. Party with a second player: `/invite`, `/accept`; see the party frame; kill
   something; verify XP share lines and that heals count (Cultist).
10. Die on purpose: XP debt message, respawn location (town bind vs gallows for
    chaotics), corpse/loot behaviour, durability hit.
11. Repair at Marta (`/repair`); read the cost before committing?
12. Anvil (kind 65): open the panel (`F`), refine +0→+1→+2→+3. Does the UI state
    odds, toll, shatter risk? React to a shatter/reset as a player would.
13. Mine: travel to Bonehowl Mine (portal), equip pick, `/mine` a node (kind 74).
    Is the node visible/labelled? Yield?
14. Trade with the second player: `T` open, offer item + gold, `P` commit,
    `X` cancel path, then verify `logs/trades.log` has the audit line.
15. Go red: kill the second player (or a lawful, then a guard-adjacent kill);
    watch karma, nameplate colour, wanted mark, guard response, gallows,
    chaotic drop on death. Then `/confess` + `/repent` at the chapel (kind 68).
16. Fence (Sable, kind 69) at the Cove: sell junk at 60 %, buy contraband —
    verify chaotic-only gating and that a lawful player is refused legibly.
17. Bounty board (kind 66): take a mark, complete it, get paid. Session-scoped —
    verify what a player is told about persistence.
18. Zone tour: walk all six zones through their portals as a human. **Record what
    the client renders per zone** (this is where B1.2 bites) and whether portal
    labels/level bands are communicated.
19. Night: force/observe the transition (debug keys `H`/`N` shift the hour —
    note that these are *debug* keys a player would not know; is there a legal
    way to see night?). Verify darkness, light items (torch burn-out, lantern
    toggle), night-only mobs appearing, night economy lines.
20. Pledge: at L≥10 with 10 000 g, found one at the registrar (kind 72);
    promote/kick; `/p ` chat; `/pledge vault`; disband. Emblem visible anywhere?
21. Siege: on a **scratch DB**, `--siege-rehearsal`; `/siege-reg`, `/breach`,
    `/crown`; observe gates (kind 75), heartstone (kind 76), flip, taxes, holder
    buff. Record exactly what a human can see and do without a GM telling them
    (this is leg 3's pass criterion: "screenshot-worthy").
22. Blood Moon: `gm blood-moon`; verify spawn-rate/drop changes are *observable*
    and announced.
23. Relog: quit and rejoin; verify level, XP, stats, gold, inventory (equipped,
    affixed, refined, durability), karma, class, sword skill, town oath, EK,
    pledge id/rank, map + position all survived. Paste the sqlite row before/after.
24. Two-client stress: 2 humans + 20 bots for 10 minutes; watch tick p99 via
    `ServerStats`, interpolation smoothness, chat, AoI pop-in at 26 tiles.

Output: a **walkthrough table** (`step | pass/fail | what the player sees |
knowledge required | friction | sev | card`) plus a short narrative "first 30
minutes" write-up. This narrative is the single most useful artifact for the
director — write it plainly, no jargon.

---

## 7. Phase E — Client legibility audit (art / UI / audio / input)

The server is far ahead of the client. This phase measures the gap that decides
whether the MVP *feels* like the genre.

### E1 Art actually on screen

- E1.1 What does the client draw for a **player**? (`atlasFor(kind 0)` →
  hero placeholder; `mobSheetPaths(0)` pinned FALSE in `tests/test_clientlaw.cpp`;
  `NetEntSnapshot` has no class/sex — T-142 open, PR #49/#50/#51 pending.)
  Consequence: **all three classes look identical, and male/female does not
  exist.** Grade against MVP §1 "Classes" + GDD §11 player sprites.
- E1.2 Mob sheets: 10 packs under `assets/aigen/mobs/` (1001–1010) vs 14
  `kMobs` rows → 1011 Gate Guard, 1012 Old Maw, 1013 Red Widow, 1014 Cantor Vex
  have **no sheets** (fall back to hero placeholder?). Verify `atlasFor` per
  wireKind and list every entity a player sees as a red-circle placeholder.
- E1.3 NPC/furniture sheets: kinds 64–66 have art (marta/anvil/bounty_board);
  67–73 (bonesmith, confessor, fence, guards, registrar, steward) have
  `atlas.draft.json` and some have `sheet.png` — verify which are **packed and
  loaded** vs draft-only. Note the T-ART-B5 procedural-proxy path.
- E1.4 Terrain: 709 PNGs under `assets/aigen/terrain/` (B1 town/fields + B2
  mine/crypts: plates, D12 edge sets, prism skins, `terrain.json` manifests) —
  verify what the client actually draws. At issue time it is flat
  `terrainColor(t)` diamonds (`client/src/game.cpp:728,749`) + untextured
  `iso::drawPrism`, and the batch docs themselves say every asset is
  "**UNVALIDATED in engine (T-ART-12)**". Confirm **T-ART-12** (textured-ground
  renderer, ruled at D6) has no card in `docs/tasks/` → the entire terrain art
  lane is produced-but-unshippable. Grade against design pillar 1
  ("era-authentic feel … not retro-styled 2024"): if the ground is coloured
  diamonds, that pillar is unmet no matter how complete the systems are.
- E1.5 Player sheets state: `assets/aigen/players/` has ravager m/f packed
  (T-141) + gravecaller/cultist **plates only** (raw PNGs, no `sheet.png`?) —
  count what is packed vs raw, and check T-141's recorded **R-LUMA contrast
  FAIL** (day Δ10.7 / night Δ4.3) status (PR #50 proposes provisional accept).
- E1.6 VFX: **0 PNGs** under `assets/aigen/vfx/` vs GDD §11 "~25". What does the
  client show for swing/cast/buff/gore today (callout text + decals + glow)?
- E1.7 Icons: **0 PNGs** under `assets/aigen/icons/` (items/skills/ui have
  BRIEF+prompt only) vs GDD §11 "~60 widgets/icons". Inventory/skill panels are
  text-only — grade.
- E1.8 Font: client uses raylib default `DrawText` (bible §12 wants a callout
  font; `tools/atlaspack/bhfont.py` exists). Verify whether any custom font is
  loaded → era-feel finding.

### E2 UI surface inventory

Enumerate every panel/HUD element in `client/src/game.cpp` (`DrawText`/`DrawRectangle`
sites) and grade each: exists · legible · era-styled · discoverable. Expected
minimum set to check: HP/MP bars, level/XP, stat rows + point-spend affordance,
kit name + buff countdowns, karma readout, aura rows, gold, clock/day-phase,
zone name + level band, chat log + input, inventory grid w/ equip + durability +
affix + refine, vendor panel, fence panel, anvil panel, trade window, party
frame, nameplates (karma-tinted, red for chaotic), floaters/callouts, target
readout, death/respawn messaging, siege state (**expected absent**), pledge
roster/emblem/vault (**expected absent**), EK board (**expected absent**),
bounty readout, minimap (**check**), keybind help (**check** — is there any
in-client list of keys? if not, P1 for a stranger), settings/video/audio options
(**check**), disconnect/kick messaging.

Also verify: no mouse-driven UI (everything is hotkeys + click-to-move) — decide
whether that is era-authentic or an alpha blocker for non-discord players.

### E3 Audio

- E3.1 `client/src/synthkit.h` procedural kit (~78 KB, 7 voices) with
  `BH_NO_AUDIO` skip; wired to ~10 callout kinds (`game.cpp:653–669`).
- E3.2 GDD §11 wants ~40 SFX + ambience. Inventory what exists: hits, crits,
  deaths, swings, anvil toll, choir, bolt. Missing: footsteps, ambient dread
  (GDD §9 "ambient dread audio"), night ambience, music, UI clicks, siege horns
  (server broadcasts a war-horn *line* — is there a sound?), boss telegraphs.
- E3.3 Verify audio device init failure is non-fatal (headless/CI safety).

### E4 Input & accessibility

- E4.1 Full keybind map (produce it from code, then judge it): `WASD`, mouse
  click-to-move, `Enter` chat, `Esc`, `Backspace`, `I` inv, `T` trade, `F` anvil,
  `G` sell junk, `1..5` skills, `Q` ?, `P`/`X`/`H` trade, `F1..F12` buy,
  `F5/F6/F7` stats, `F8+` ?, `F3` grid, `F4` path, `H`/`N` debug hour, `Z` zoom.
  Flag collisions (note `H` is both *debug hour* and *trade gold offer* — verify
  context gating), undocumented bindings, and debug keys reachable by players
  (`H`/`N` shift the local hour offset: does that desync the client clock from
  the server? grade as a trust/legibility issue).
- E4.2 No rebinding, no window resize/scale options? Verify `InitWindow(1024,768)`
  and whether the window is resizable; MVP §4 M5 wants 60 fps on a 2018 MBA —
  check for a fullscreen/scaling path for small screens.

### E5 Produced-but-unrenderable assets (the art↔engine contract)

Build one table: `asset lane | on-disk volume | engine path that would draw it |
exists? | gating card | card filed? | sev`. Known lanes to cover (verify each,
add any you find):

| Lane | Volume at issue time | Gating engine card | Filed? |
|---|---|---|---|
| Terrain plates / D12 edges / prism skins (B1+B2) | 709 PNGs + `terrain.json` manifests | **T-ART-12** textured-ground renderer (D6) | verify |
| Callout + nameplate bitmap font (D4, 7×11 red-caps) | designed; `tools/atlaspack/bhfont.py`; `icons/ui/callout_font.png/.fnt` not shipped | **T-ART-13** bitmap-font path | verify |
| VFX strips (swings/casts/buffs/gore/boss) | **0 PNGs**; `docs/art/50-vfx.md` spec'd ~25 | **T-ART-14** `dirs:1` strip loader (`loadAtlas` rejects `dirs:1` at `engine/assets/atlas.cpp:29`) | verify |
| Item / skill / UI icons | **0 PNGs**; BRIEF+prompt only | **T-ART-15** icon + hotbar draw path | verify |
| Player sheets (18 per D7 bake ×3) | 2 packed (ravager m/f) + 40 raw plates | T-142 wire class/sex → `playerSheetPaths` | open card + PRs #49/#50/#51 |
| Mob sheets for 1011–1014 (guard, Old Maw, Red Widow, Cantor Vex) | none | content/art lane (no engine gate) | verify |
| NPC/furniture kinds 67–73 | `atlas.draft.json` (+ some `sheet.png`/`portrait.png`) | T-ART-06 shipped procedural proxies — verify what actually draws | verify |
| Name-tag pile-up rule **R-TEXT-2** (crowd-test approved: >3 overlapping tags → karma-badge glyph) | n/a (render rule) | client render change, no card | verify |

The point of this table: **art production is not the bottleneck — the renderer
is.** Quantify how many shipped PNGs a player can currently see, and say the
number plainly in the report's verdict block.

---

## 8. Phase F — Server, persistence, security, determinism, ops

### F1 Authority & trust boundary (AGENTS.md law)

- F1.1 Every client→server message re-validated: path/step speed, attack range +
  cooldown, skill channel unlock + CD + MP, use-item slot ownership, buy/sell
  price + stock + proximity (≤3 tiles), trade commit symmetry, anvil tier +
  proximity + toll, refine slot index, mine proximity + tool, party/pledge
  permissions, duel consent, stat point availability. Probe by reading the
  handler for each `Command::Kind` in `server/src/command.h` and produce a
  **coverage table** (kind → validated? where? test?).
- F1.2 Throw-free parsing of every client-controlled number (T-104/T-111 class):
  `grep -rn "std::sto" server/src shared client/src tools/bots` → must be empty
  on client-input paths.
- F1.3 Reader bounds: `shared/protocol/bytestream.h` string/length caps; fuzz a
  malformed packet at the live server (craft with a small python ENet client or
  a truncated bot payload) and confirm no crash/terminate. **Note: no fuzz
  harness exists (AGENTS.md truth-up) — grade the absence for alpha (§7 "rate
  limits + fuzz pass").**
- F1.4 Gold/qty overflow: u64 widen paths (T-104 #3) — re-verify with a probe.
- F1.5 Interest management: AoI ≤26 tiles; verify no full-world broadcast leaks
  (chat ch1 global is by design; check pledge chat + siege broadcasts scope).

### F2 Determinism & replay

- F2.1 Single mutation path: `applyWorldCommand` used by live **and** replay
  (T-049); any direct world mutation outside it is a finding — grep for
  suspicious call sites.
- F2.2 RNG: `grep -rn "rand()\|random_device\|srand\|std::mt19937" server shared
  client tools` → gameplay paths must be `sim/rng.h` only.
- F2.3 No wall-clock in sim: `grep -rn "steady_clock\|system_clock\|time(nullptr)\|gettimeofday"
  server/src shared/sim` → allowed only in shell/telemetry, never in tick logic.
- F2.4 `worldHash` coverage: which fields are hashed (id/zone/pos/hp + economy/
  progression per T-107)? What is **deliberately outside** (pledge state per
  T-122, siege session state?) — list the blind spots and judge whether a
  launderer class (T-049x) could hide there today.
- F2.5 Epoch discipline: `kJournalEpoch = 27` + history comment; committed legs
  (118 `.bwj`); verify **at least** `logs/t146.bwj` replays `mm=0` and one older
  leg refuses with exit 4. Then pick 3 random historical legs and replay them:
  every refusal must be exit 4 (not a mismatch, not a crash).
- F2.6 deque-lifetime discipline (T-106/T-111): grep erase sites
  (`inv.erase`, `ents_.erase`, `despawn`, `killMob`) and check each snapshots
  identity first; run `tests/test_deque_dangling.cpp`.
- F2.7 Fixed 20 Hz: `SIM_TICK_HZ`/`sim::kTickHz` usage; no frame-delta in
  `shared/sim` or `server/`.

### F3 Persistence

- F3.1 Schema ladder: read `server/src/persist.cpp` migrations v1→v14; verify a
  **v1-era DB** (or an empty file) upgrades cleanly to v14 and that every
  `ALTER TABLE` is `user_version`-guarded and additive.
- F3.2 Round-trip fidelity: every `CharacterRow` field survives logout/login
  (level, xp, str/vit/dex, statPoints, gold, anvilMercy, karma, classId,
  swordSkill, swingLands, townId, ek, pledgeId, pledgeRank, invBlob with
  equipped/durability/affix/refine, map/x/y). One live probe + one sqlite diff.
- F3.3 Inv-blob grammar: single shared `parseInvBlob` for live + replay
  (T-049x); verify no second parser exists.
- F3.4 WAL posture + `PRAGMA`s; concurrent-write behaviour with 60 bots;
  fsync/latency; DB size growth per hour (estimate for a 12 h soak).
- F3.5 Siege/pledge persistence: `siege_state` (holder/vault/crowns) +
  `pledges`; verify what survives a restart mid-siege and whether session-scoped
  state (bands, registration, quarry cycle, wanted marks) is *intended* to be
  lost — list player-visible consequences ("the castle remembers, the war doesn't").
- F3.6 Audit trails: `logs/trades.log` (dupe trail), item audit queries (MVP §8
  R6 wants them) — do they exist? `grep -rn "audit" server/src tools/ops`.

### F4 Netcode & protocol

- F4.1 `shared/protocol/messages.md` vs reality: 37 messages, last id 116.
  Everything shipped after parties (karma band rides `EntitySpawn`, glowTier,
  EK/oath, siege, pledge, mine, blood moon) — determine **which have wire
  representation and which are chat-only**. Produce the drift table and grade:
  a docs-only protocol spec that no longer describes the wire is a P2/P3, but
  *systems with no wire presence at all* (siege state, pledge roster, emblem,
  vault) are the P1 legibility findings from B10.6/B11.2.
- F4.2 `kProtocolVersion = 200 + messageCount` (protogen.py:82): note the
  hazard — the version changes implicitly when a message is added, and two
  different trees with the same count collide. Grade (P2) and recommend an
  explicit constant.
- F4.3 `shared/protocol/gen/messages_gen.{h,cpp}` are **committed** while the
  build generates into `${CMAKE_BINARY_DIR}/generated` — dead/drift-prone
  duplicates. Verify nothing includes the source-dir copy; grade P3 with a
  delete-or-generate-in-place recommendation.
- F4.4 ENet channel usage: reliable vs unreliable split, packet size caps,
  reconnect/spawn-diff view (T-017), interpolation 120 ms — verify constants and
  behaviour under 200 ms artificial lag if you can (`tc` may be unavailable;
  then reason from code and mark UNVERIFIED).
- F4.5 AoI churn at zone borders + cross-zone handoff (T-037 `Welcome` reuse):
  probe with two clients on either side of a portal.

### F5 Stability & performance debt

- F5.1 Per-tick allocations in `tickServer`/`distributeEvents`/`queryAoi` and the
  AoS `std::deque<Entity>` store (AGENTS.md/T-110 truth-up: the SoA
  `EntityStore` in `docs/03-architecture.md` is a **deferred card, not law**).
  Quantify: measure RSS over a 10-min 60-bot soak (T-144 saw 8.2 MB flat) and
  decide whether the debt blocks alpha (probably not) — but record it honestly.
- F5.2 Single-file risk: `world.cpp` ~176 KB. Recommend (do not perform) a
  split plan; note it as a velocity risk for the remaining P0 work.
- F5.3 Tick budget: p99 targets (M1 <10 ms, M4 <25 ms); re-measure on today's
  tree with `--soak-secs 600 --p99-budget-ms 10` (or 25) and paste.
- F5.4 Long-run leaks: the M5 200-bot × 12 h soak is **director-scheduled**;
  T-144 banked 60×10 min. State exactly what remains and what it needs (host,
  wall time, monitoring).

### F6 Security posture for a *public* alpha

- F6.1 Password stub (FNV-1a) → argon2id (+ADR for the dep) — P0 for wave 1.
- F6.2 No GM flag / no ban / no announce (B13.2) → moderation gap; grade against
  "invite wave 1" risk (friends-of-friends) and propose the minimum (a
  `gm_accounts` table or env-var operator list + `/ban` + `gm announce`).
- F6.3 Registration open by default (`--no-register` to gate) + limiter — decide
  the alpha posture and document it.
- F6.4 Secrets/hostnames: `grep -rn "password\|token\|secret" --include=*` for
  committed credentials (AGENTS.md prohibition); check `tools/ops/*` for hardcoded
  paths/hosts.
- F6.5 Asset licensing: `assets/LICENSES.md` completeness for every shipped
  PNG (MVP §7 legal-lite box) — count assets vs licence rows; flag gaps.
- F6.6 Name/trademark sanity on "BLOODHOLLOW" (director box) — just record status.

---

## 9. Phase G — Gate metrics re-verification (MVP §4 + §6)

For each gate: `metric | target | claimed evidence (date, card/devlog) | still
valid on epoch 27? | re-run command | fresh result | verdict`.

| Gate | Metric | Target | Notes for the auditor |
|---|---|---|---|
| M1 | 20-bot 30-min soak tick p99 / desyncs | <10 ms / 0 | Historical p99 0.30 ms (2026-09-04). Re-run short form; state staleness. |
| M2 | Solo TTK vs equal mob; XP/hr at L6–8 | 6–10 s; ~1.2 levels/h | Use `tools/bh_duel` + balancer feed; compare to T-113/T-114 findings (vial flow + 30 shop trips/leg is the binding constraint). |
| M3 | 5-man vs solo XP/hr party advantage | ≥1.4× total | Evidence is old (S13/S21). Re-run `tools/m3_*` legs or mark stale. |
| M3 | Refine EV: ore+gold cost of a +5 | ≈45–60 min farming | Needs B6.3/B6.4 truth first (ore/purity/fodder may not exist) — compute with shipped law and state the deviation. |
| M4 | Siege sim 40 bots: tick p99, capture flips | <25 ms, 3/3 | `tools/t137_m4_leg.sh`; devlog 0098 claims PASS at epoch 25. **Re-run at 27** — the merge repair changed world composition (ore + steward). |
| M5 | 200-bot 12 h soak; client min-spec fps | no leaks/crashes; 60 fps 2018 MBA | Not run (T-144 = 60×10 min pre-soak). State exactly what is missing + who owns it. |
| Alpha | D1/D7 retention; median session | ≥60 %/≥30 %; ≥45 min | Needs humans; director-owned. |
| §6 legs 1–4 | Friday-Night Test | see `docs/ops/friday-night-readiness.md` | Verify every tool/flag/verb that page cites **still exists and works** (it was written at epoch 26, pre-merge-repair). Legs 1–3 runnable; leg 4 survey. |

Also re-verify the go/no-go line on that page: "Stack merged through T-144 (or
rehearsal pins: epoch 26, schema v14)" — **the tree is now epoch 27**; the page
is stale. That's a finding (P2) with a one-line fix owner.

---

## 10. Phase H — Repo / process hygiene

| # | Check | Probe | Why it matters |
|---|---|---|---|
| H1 | **Open-PR triage** | `gh pr list --state open` + `gh pr view N --json title,headRefName,baseRefName,mergeable,files` for #22, #23, #27, #28, #30, #49, #50, #51 | 8 open PRs, several stacked/superseded by the merge repair. For each: `MERGE / REBASE / CLOSE-SUPERSEDED / NEEDS-EVIDENCE` + one-line reason + whether master already contains its content (`git log`/grep the feature). **Do not merge any.** |
| H2 | Master vs PR content drift | e.g. is T-126 affix v2 (#30) already in master? (`kAffixCount == 10` + `tests/test_affix_v2.cpp` compiled → yes) | Prevents re-doing shipped work; the board says "open" while the tree says "landed". |
| H3 | Single-commit history | `git rev-list --count HEAD` = 1 | No blame/bisect. Record the consequence for future debugging and recommend (don't perform) a history policy. |
| H4 | Board truth | `docs/tasks/README.md` vs `docs/tasks/done/` vs tree | Rows claim things ("Next: Phase-4 pledge/siege spine" in README) that already shipped; T-104/T-112/T-138–T-145 live in `docs/tasks/` **root** (not `done/`) though the board says "Done … (→ done on merge)". List every misfiled card. |
| H5 | Uncompiled test | `tests/test_siege_stub.cpp` on disk, absent from `tests/CMakeLists.txt` | Dead code kept "as evidence" — decide: delete, or compile + skip. P3. |
| H6 | Committed generated/vendored blobs | `git ls-files \| grep -E '\.(png\|bwj\|log)$' \| wc -l`; `.git` = 101 MB | Repo weight vs the 128 MB patchset cap in agent sandboxes; recommend an LFS/asset-split **decision**, not an action. |
| H7 | `.gitignore` vs tracked legs | `.gitignore` has `logs/*.bwj`, 118 legs tracked | New legs need `git add -f`; a future agent will silently lose a gate leg. P3 with a one-line doc fix. |
| H8 | Docs sprawl | `ls docs/prompts \| wc -l` (27), `docs/devlog` (107), `docs/handover` (3) | Single-use prompts accumulating as ground truth. Recommend an index/archive rule. |
| H9 | AGENTS.md aspirations vs practice | fuzz (absent), zero-alloc SoA (absent), branch+PR flow (8 stale PRs), macOS build (CI only) | The rules file itself flags these; verify each is still accurately flagged, and add any new drift you find. |
| H10 | ADR coverage for shipped deviations | `ls docs/adr` = 11 ADRs | Deviations needing an ADR: refine shatter law (B6.2), pledge gate L10+10k (B11.1), Blood Moon in MVP (B8.8), 1 char/account (B12.1), CHA-less stat model (B2.5), argon2id dep (F6.1). List them as "ADR owed". |

---

## 11. Phase I — Synthesis: the cut line

This is the deliverable that matters. Produce, in this order:

1. **Verdict block** (top of the report, ≤10 lines): `PLAYABLE / PLAYABLE-WITH-
   CAVEATS / NOT-PLAYABLE`, P0/P1/P2/P3 counts, the 3 worst findings in plain
   English, and estimated agent-sessions to clear P0.
2. **P0 list — "the minimum playable set"**: every finding that blocks P-1/P-2/
   P-3, each with: requirement id, one-line problem, evidence, proposed fix
   shape (not code), effort S/M/L, owner (agent / director / both), dependencies,
   and whether it needs a wire/schema/epoch change (flag those loudly — they are
   the expensive ones and must be sequenced first).
3. **P1 list** — the genre-fantasy leaks (expected candidates: siege/pledge
   client legibility, class/sex sprites, rarity tiers, gear slots, missing skills
   incl. Resurrect, CHA stat, town-war reachability, night light law, icons/VFX,
   keybind help). Same fields.
4. **Effort & sequencing table**: dependency-ordered waves. Rule: **wire/schema/
   epoch changes first** (they invalidate everyone else's journals), then
   server-only, then client-only, then content/art (parallelizable), then
   ops/director boxes last-but-scheduled.
5. **Proposed sprint plan to playable alpha**: 2-week sprints with a *playable
   demo at the end of each* (roadmap law), each sprint naming the cards, the
   gate it re-proves, and the human playtest commitment (~12 h/wk director).
   Give the calendar in weeks-from-approval, not dates.
6. **Defer / cut recommendations**: what to *remove* from MVP IN to hit P-2
   faster, with the change-control sentence required by MVP §1 ("adding anything
   to IN requires cutting an IN item of equal cost + an ADR"). Be specific and
   brave — e.g. candidate cuts to evaluate: 4 chars/account → 1 (already the
   reality), ~40 affixes → 20, 11 gear slots → 5, rarity tiers → 3, pickup-N
   bounties, launcher → manual manifest for wave 1, EK web page → in-game
   readout, six-stat model → keep 5 and defer CHA. For each: what breaks in the
   fantasy, what it saves, and your recommendation.
7. **Risk-register delta** against MVP §8 (R1–R8): which risks fired (R2 art
   volume, R3 scope creep — Blood Moon/sword auras, R6 cheaters — no ban, R7
   macOS — CI-only), which are newly visible (client-legibility debt, single-file
   `world.cpp`, stale gate evidence across an epoch bump, 8-PR merge queue,
   single-commit history), each with a mitigation owner.
8. **Card drafts** for every P0 and P1 (§12.2 template), numbered from the next
   free id (**T-147** onward — verify with `ls docs/tasks docs/tasks/done`), each
   ≤1 session, each with acceptance criteria that a *reviewer can execute*.
   Art-only cards use the `T-ART-*` series; audit-follow-up docs use
   `T-AUD-*` if you need a non-code lane.
9. **Docs truth-up list**: every file whose text you proved wrong (README
   §Status "Next:", `docs/ops/friday-night-readiness.md` epoch line,
   `messages.md` missing messages, `docs/art/00-VERIFY.md` rows now shipped,
   GDD numbers changed by tuning, `AGENTS.md` aspirations), with the exact
   sentence to replace it. **Do not edit them** — list them for a docs card.

---

## 12. Deliverables (exact paths + shapes)

Write these files, and nothing else:

1. `docs/audits/2026-09-16-mvp-playable-audit.md` — **the report** (§12.1 shape).
   New directory `docs/audits/` is intentional: audits are not devlogs.
2. `docs/audits/2026-09-16-mvp-requirement-ledger.md` — the machine-readable
   ledger: one row per requirement id (A1…H10, B1.1…B14.n, C1…C18, D1…D24,
   E1…E5, F1…F6, G-gates) with `probe | observed | verdict | sev | owner |
   effort | card`. Keep it flat and greppable; it is the input to future cards.
3. `docs/audits/2026-09-16-card-drafts/T-147-*.md …` — one file per P0/P1 card
   draft (§13.4 template). Do **not** touch `docs/tasks/` (that's the board's
   lane; the director files cards from your drafts).
4. One devlog stub: `docs/devlog/0107-mvp-playable-audit.md` (≤15 lines, house
   style: what was audited, verdict, top 3 findings, artifact paths).
5. A single docs-only commit on `arena/01a0a8c9-bloodhollow` + one PR to
   `master` titled `Audit: MVP-playable gap report (epoch 27) — verdict + P0 cut
   line + card drafts`, body containing: verdict block, P0 table, evidence
   highlights, deviations (incl. any `AUDIT-UNBLOCK-n`), and "no code, no wire,
   no schema, no epoch change". **Do not merge it.**

### 12.1 Report shape

```markdown
# MVP-playable audit — 2026-09-16 (HEAD <sha>, epoch <n>, schema v<n>)

## 1. Verdict
PLAYABLE | PLAYABLE-WITH-CAVEATS | NOT-PLAYABLE
P0 <n> · P1 <n> · P2 <n> · P3 <n> · UNVERIFIED <n>
Build/run capability this session: <full | headless-only | static-only> — why.
Three worst findings, in plain English:
1. …  2. …  3. …
Estimated agent-sessions to clear P0: <n> (of which <m> need wire/schema/epoch).

## 2. What is already true (do not re-audit)
<short list of solid SHIPPED areas with their probes>

## 3. Phase results
### A Build/boot … ### B MVP scope … ### C GDD delta … ### D Walkthrough …
### E Client legibility … ### F Server/persistence/security … ### G Gates …
### H Hygiene …
<each: table + narrative, every row with probe/observed/verdict/sev>

## 4. The P0 cut line (minimum playable set)
<table: id | problem | evidence | fix shape | effort | owner | deps | wire/schema/epoch?>

## 5. P1 genre-fantasy leaks
## 6. Sequencing & sprint plan to playable alpha
## 7. Defer/cut recommendations (with the change-control sentence)
## 8. Risk-register delta
## 9. Docs truth-up list
## 10. Card drafts index (T-147…)
## 11. Audit deviations & unverified rows (RUN-FIRST battery for the director)
```

### 12.2 Card-draft template

```markdown
# T-<nnn> — <title> (<phase>, <n>/<m>)

## Context
<2–5 lines: the audit finding id, the requirement it closes, why now.>

## Scope
<bullets: files/systems touched; wire? schema? epoch? art? none?>

## Acceptance criteria
<executable by a reviewer: commands + expected output + what a human sees.>

## Tests required
<doctest cases, leg script, replay mm=0, screenshot.>

## Out of scope
<explicit non-goals; the follow-up card ids.>

## Evidence owed at merge
<suite count, ctest, leg of record path + replay line, devlog id, board row.>
```

---

## 13. Definition of done (the audit itself)

- [ ] Every MVP §1 IN row has a decomposed ledger entry with a probe and a
      verdict; nothing is answered from a devlog or from memory.
- [ ] Every GDD number the MVP depends on is in the §5 delta table.
- [ ] The §6 walkthrough was run (or each step honestly marked `UNVERIFIED` with
      the reason and a `RUN-FIRST` command for the director).
- [ ] Client legibility (§7) is measured per asset area with on-disk counts, not
      impressions.
- [ ] All 8 open PRs triaged with a verdict; none merged.
- [ ] Gate metrics table states, per gate, whether the evidence is fresh on
      epoch 27 or stale — and re-runs at least M4's siege leg and one replay
      round-trip if a build was possible.
- [ ] P0 list is small enough to be believable (if it exceeds ~12 items, you have
      not prioritised — re-cut and say what you demoted and why).
- [ ] Card drafts exist for every P0 and P1, numbered from the next free id,
      each ≤1 session, each with executable acceptance criteria.
- [ ] Defer/cut section makes at least 5 concrete cut proposals with the
      change-control sentence.
- [ ] No code, wire, schema, epoch, board, or GDD edits. No merges. One docs-only
      PR from `arena/01a0a8c9-bloodhollow`.
- [ ] The report's verdict block fits in 10 lines and a tired human can act on it
      without reading the rest.

### 13.1 Prohibited (hard)

- Implementing fixes, "tiny" refactors, or formatting sweeps.
- Bumping `kJournalEpoch`, `kProtocolVersion`, or the DB schema.
- Editing `docs/tasks/**`, `docs/02-gdd.md`, `docs/05-mvp.md`, `README.md`,
  `AGENTS.md` (list the edits you *want* in §9 of the report instead).
- Merging, closing, or rebasing anyone's PR; pushing to any branch but
  `arena/01a0a8c9-bloodhollow`.
- Touching `assets/final/`; adding third-party deps; adding Windows code.
- Running a siege rehearsal or any write-heavy probe against a non-scratch DB.
- Inventing slash verbs, flags, file paths, metrics, or test results.
- Citing this prompt's Appendix A as a finding without re-verifying it.

---

## Appendix A — Recon leads (2026-09-16, HEAD `2bd471d`)

**These are leads, not conclusions.** Each was observed during prompt authoring
and each must be independently re-verified (they may be fixed by an open PR you
triage, or wrong). Verify → then cite with your own probe.

| # | Lead | Where | Likely sev |
|---|---|---|---|
| L1 | Client `mapFileFor()` has cases 2–5 + `default: thornwall`; **no case 6** → the Weeping Castle (siege stage, zone 6, loaded server-side) renders as Thornwall for a human. | `client/src/game.cpp:971-978` vs `server/src/main.cpp:1278,1628` | **P0** (kills Friday-Night leg 3) |
| L2 | Client has **zero** siege/pledge awareness: `grep -rn "siege\|pledge" client/src/` → no hits. No holder, countdown, gate/heartstone HP, band roster, crown progress, pledge roster/emblem/vault UI. | `client/src/*` | **P0/P1** |
| L3 | No wire messages for siege/pledge/EK/mine: `messages.md` ends at `PartyMember = 116`; those systems are chat-verb + `OwnStats`-field only. | `shared/protocol/messages.md` | **P1** (blocks L2's fix) |
| L4 | **1 character per account**, no char-select screen, no offline slots; MVP asks for 4. Class is chosen post-hoc by `/kit`; and `grep -rni "\bsex\b" server/src shared client/src` → **no hits**: no sex field exists in Entity, `CharacterRow`, or the wire, so any sprite work needs creation-flow + schema + wire changes (T-142's own scope alert). | `server/src/persist.cpp:332,370`; `server/src/persist.h` `CharacterRow`; `docs/tasks/T-142.md` | **P1** |
| L5 | Password hashing is the ADR-0009 **stub** (salted iterative FNV-1a) with "argon2id before any public alpha" owed. | `server/src/persist.h` (comment + `stubPasswordHash`) | **P0** for a public wave |
| L6 | Stats: only STR/VIT/DEX assignable; INT/MAG come from kit seeds; **CHA does not exist** though GDD §3/§8 hang aura radius, pet slots, pledge creation and vendor prices on it (and the pledge gate was already re-cut to L≥10 + 10k because of it). | `server/src/world.cpp:338-354`; `shared/content/kits.h`; `docs/tasks/done/T-122.md` | **P1** |
| L7 | Skill channels: 9 shipped vs ~23 in GDD §3. **Resurrect (L20), Sanctuary, Raise Skeleton, Corpse Explosion, Terror, Mana Shield, Frost Spike, Wither, Curse of Weakness, Sunder, Bull Rush, War Stomp, Executioner, Second Wind** unverified/absent. | `shared/content/kits.h` `chUnlock[10]`; `server/src/world.h:251-258` | **P1** (pillar: support class as social glue) |
| L8 | **Rarity tiers absent** (`grep -rn rarity` → empty) vs MVP "4 rarity tiers"; affixes 10 vs "~40"; gear slots 2 vs GDD's 11. | `shared/content/items.h` | **P1** |
| L9 | Refine law drift: chances `{100,100,60,65,50,35,25}` vs GDD `{100,90,80,…}`; **SHATTER at +2→+3 failure** is not in GDD §7 at all; no ore **purity**, no **fodder tiers** (ids 5101–5103 reserved, unused). | `server/src/world.cpp:2153,2162,2167`; `docs/art/00-VERIFY.md` row 16 | **P1/P2** + ADR owed |
| L10 | Mob roster 14 rows vs GDD's 12 common + 2 elites + 3 named + 1 boss; missing Pale Cultist, Mine Wretch, Lantern Spider, Mud Golem, Crypt Revenant, Grave Banshee, Bell Ringer; mobs 1011–1014 have **no art packs**; D9 rename (1007 → Waxen Celebrant) not applied to `mobs.h`. | `shared/content/mobs.h`; `assets/aigen/mobs/` | **P1** (art R2 + variety) |
| L11 | **VFX 0 PNGs, icons 0 PNGs** vs GDD §11 "~25 VFX / ~60 UI widgets+icons"; client uses raylib default font; panels are text-only. | `assets/aigen/vfx`, `assets/aigen/icons`, `client/src/game.cpp` `DrawText` | **P1/P2** |
| L12 | Player sprites: exactly **2** packed sheets exist (`players/ravager/{m,f}/sheet.png`, T-141) with a recorded **R-LUMA contrast FAIL**; 40 more PNGs are unpacked `plates/` (Gravecaller/Cultist, both sexes); D7 ruled **18 baked sheets** for MVP (3 classes × 2 sexes × 3 tones). `mobSheetPaths(0)` is pinned FALSE → every player draws as the hero placeholder today. T-142 open; PRs #49/#50/#51 pending. | `find assets/aigen/players -name sheet.png`; `docs/art/B0-GATE-DECISION.md` D7; `tests/test_clientlaw.cpp` | **P1** |
| L13 | **Terrain art is invisible.** 709 tile/edge/prism PNGs exist under `assets/aigen/terrain/`, but the client draws the ground as flat `terrainColor(t)` diamonds and walls as untextured `iso::drawPrism`; the batch docs say nothing is visible until **T-ART-12** (textured ground + prism skins + D12 edge lookup) exists — and no `T-ART-12` card is filed (`done/` has only T-ART-06/09/11). Design pillar 1 ("era-authentic feel") is unmet regardless of systems. | `client/src/game.cpp:728,749`; `engine/render/iso.cpp`; `docs/art/batches/B1-terrain-town-fields.md` (head) | **P0/P1** |
| L13b | **Four engine cards were ruled but never filed**, and between them they gate *all* remaining art visibility: **T-ART-12** textured-ground renderer (D6 — unblocks the 709 B1/B2 terrain PNGs), **T-ART-13** bitmap-font path for callouts/names (D4 — `callout_font.png/.fnt` designed, `DrawText` still default), **T-ART-14** VFX `dirs:1` strip loader (D8 — `loadAtlas` rejects `dirs:1`, so no VFX can load), **T-ART-15** icon/hotbar draw path (D8 — unblocks item/skill/UI icons). Plus **R-TEXT-2** (crowd-test rule: >3 overlapping name tags degrade to karma-badge glyph) is an approved client render change with no card. `ls docs/tasks/done | grep ART` → only 06/09/11. | `docs/art/B0-GATE-DECISION.md` D6/D8/Part 4; `engine/assets/atlas.cpp:29`; `docs/art/50-vfx.md:77` | **P0/P1** (the art pipeline's output is currently unshippable) |
| L14 | No launcher/patcher, **no `/ban`**, **no `gm announce`**, no GM account flag, no crash-upload path — all MVP §7 boxes, all documented as gaps/deferred. | `docs/ops/gm-runbook.md §2,§5,§6` | **P1** (alpha ops) |
| L15 | CI gates only: thornwall mapgen diff, build, ctest, thornwall mapconv validate. **No replay leg, no soak, no headless-preset leg, no client/xvfb visual check, 5 of 6 mapgens unchecked.** All gate evidence is manual leg scripts. | `.github/workflows/ci.yml`; `tools/*_leg.sh` | **P2** |
| L16 | Build-dir law is inconsistent: `tools/bootstrap.sh` → `build/` (Release), README → `build/linux-gcc`, leg scripts → `build/linux-gcc` hard-coded. A stranger cannot follow one path. | `tools/bootstrap.sh`; `tools/t146_merge_repair_leg.sh`; `README.md:95-105` | **P1** (blocks P-1 clean boot) |
| L17 | `docs/ops/friday-night-readiness.md` go/no-go pins "epoch 26, schema v14" — tree is **epoch 27**; M4 siege evidence (devlog 0098) predates the merge repair that changed world composition (ore + steward). Re-run required. | `docs/ops/friday-night-readiness.md`; `tools/t137_m4_leg.sh` | **P1** (stale gate) |
| L18 | README §Status still says "Next: Phase-4 pledges/siege spine" though T-131..T-140 shipped siege + pledges; board rows for T-138..T-145 say "→ done on merge" while the cards sit in `docs/tasks/` root. | `README.md:120-130`; `docs/tasks/README.md` | **P3** |
| L19 | `shared/protocol/gen/messages_gen.*` committed but unused (build generates into `${CMAKE_BINARY_DIR}/generated`); `tests/test_siege_stub.cpp` on disk but not compiled. | `shared/protocol/CMakeLists.txt:5`; `tests/CMakeLists.txt` | **P3** |
| L20 | `kProtocolVersion = 200 + messageCount` — implicit versioning; two trees with equal message counts but different layouts share a version. | `tools/protogen/protogen.py:82` | **P2** |
| L21 | No teleport scrolls (an MVP §7/GDD economy sink), no persistence for the bounty board (session-scoped by design), no EK board surface (in-game or web) despite `ek` being persisted. | `grep -rn teleport server shared` (empty); `world.h:438-440`; `persist.h` `ek` | **P2** |
| L22 | Night light law is contradictory across docs (GDD §9: base 0 / torch 6 / lantern 8, but "shipped 0 per T-076/B"); no additive light mask (T-ART-03) — establish what a human actually sees at 04:00 with and without a torch. | `docs/02-gdd.md §9`; `engine/render/lightmask.h`; `docs/art/00-VERIFY.md #7` | **P1** (verify) |
| L23 | 8 open PRs including stacked/superseded lanes (#22/#23 predate the Phase-S stack; #30's content appears already in master) — merge-queue risk before any further work. | `gh pr list --state open` | **P1** (process) |
| L24 | `.gitignore` lists `logs/*.bwj` while 118 legs are tracked → a new gate leg silently won't `git add`. | `.gitignore:9`; `git ls-files 'logs/*.bwj' \| wc -l` | **P3** |
| L25 | `.git` is 101 MB with 2298 tracked files (PNG-heavy) — agent patchset caps and clone time; no LFS/asset-split decision on record. | `du -sh .git`; `git ls-files \| wc -l` | **P3** |
| L26 | No in-client keybind help / no tutorial / debug keys (`H`,`N` hour offset; `F3`,`F4`) reachable by players — a stranger's first 5 minutes are undocumented. | `client/src/game.cpp:118-121,153-158` | **P1** |
| L27 | No fuzz harness and no property-style combat/enhance math test (AGENTS.md truth-up confirms the aspiration); MVP §5 S15 wanted "rate limits + fuzz pass". | `AGENTS.md`; `tests/` | **P2** |
| L28 | AoS `std::deque<Entity>` + per-tick allocations remain (SoA `EntityStore` deferred); `world.cpp` ≈176 KB single file — velocity + stability risk for the remaining P0 work. | `AGENTS.md` DoD note; `wc -c server/src/world.cpp` | **P2** |

---

## Appendix B — Command cookbook

```bash
# --- orientation -----------------------------------------------------------
git log -1 --format='%H %s'; git rev-list --count HEAD; git status --short
grep -n "kJournalEpoch" server/src/main.cpp | head -3
grep -n "user_version=" server/src/persist.cpp | tail -3
grep -c '^message' shared/protocol/messages.md
gh pr list --state open; gh pr view 49 --json title,mergeable,files

# --- structural probes (safe anywhere) -------------------------------------
grep -rn "rarity\|Rarity" server shared client tests        # expect: empty
grep -rn "siege\|pledge" client/src                          # expect: empty
grep -rn "teleport" server shared                            # expect: empty
grep -rn "\bcha\b" server/src shared                         # CHA stat?
grep -rn "std::sto" server/src shared client/src tools/bots  # throw-free law
grep -rn "rand()\|random_device\|srand" server shared client # RNG law
grep -rn "steady_clock\|system_clock\|time(nullptr)" server/src shared/sim
grep -n "case [0-9]:" client/src/game.cpp | sed -n '/mapFileFor/,$p'
sed -n '/const char\* Game::mapFileFor/,/^}/p' client/src/game.cpp
grep -n "kRefineChance" -A3 server/src/world.cpp
grep -n "assignStat" -A16 server/src/world.cpp | head -20
grep -n "FROM characters" -B4 -A6 server/src/persist.cpp | head -40
ls tests/*.cpp | wc -l; grep -c "test_.*\.cpp" tests/CMakeLists.txt
for d in mobs npcs players terrain vfx icons palettes; do \
  echo "$d: $(find assets/aigen/$d -name '*.png' | wc -l) png"; done
grep -rn "nightOnly" data/maps-src tools/mapgen | head
wc -c server/src/world.cpp server/src/main.cpp client/src/game.cpp

# --- live probes (scratch DB only) -----------------------------------------
PORT=7841; DB=/tmp/audit_$PORT.db; rm -f $DB $DB-wal $DB-shm
./build/headless/server/bh_server --db $DB --port $PORT --soak-secs 120 \
  --record-world /tmp/audit_$PORT.bwj > logs/audit_srv.log 2>&1 &
sleep 2
./build/headless/tools/bots/bh_bots --port $PORT --count 20 --secs 60 \
  --profile fighter --prefix aud_ > logs/audit_bots.log 2>&1
wait; ./build/headless/server/bh_server --replay-world /tmp/audit_$PORT.bwj | tail -3
./build/headless/server/bh_server --replay-world logs/t146.bwj | tail -3   # mm=0
./build/headless/server/bh_server --replay-world logs/t140.bwj; echo "exit=$?"  # 4

# --- persistence probe -----------------------------------------------------
sqlite3 /tmp/audit_7841.db "PRAGMA user_version; SELECT count(*) FROM accounts;
  SELECT name,level,xp,gold,karma,class_id,sword_skill,town_id,ek,pledge_id,
  pledge_rank,map_id,inv FROM characters LIMIT 5;"

# --- soak / perf -----------------------------------------------------------
./build/headless/server/bh_server --db /tmp/audit_soak.db --port 7842 \
  --soak-secs 600 --p99-budget-ms 10 ; echo "soak exit=$?"
bash tools/t137_m4_leg.sh        # M4 siege re-proof at current epoch (scratch DB)

# --- maps ------------------------------------------------------------------
for m in thornwall:1 fields_overflow:2 thornwall_crypt:3 bonehowl_mine:4 \
         drowned_crypt:5 weeping_castle:6; do \
  n=${m%:*}; i=${m#*:}; \
  ./build/headless/tools/mapconv/bh_mapconv data/maps-src/$n.tmj /tmp/$n.bhmap \
    --map-id $i --validate && echo "$n OK"; done
python3 tools/mapgen/validate_links.py
```

---

## Appendix C — Turn budget (suggested, adapt freely)

| Turns | Work |
|---|---|
| 1–2 | §2 bring-up; record build capability; orientation facts table |
| 3–5 | Phase A battery + Phase F2/F3 (determinism, persistence, replays) |
| 6–9 | Phase B ledger (the big one) + Phase C delta table |
| 10–11 | Phase D walkthrough (or degraded-mode equivalent) |
| 12–13 | Phase E legibility + Phase G gate re-runs |
| 14 | Phase H hygiene + PR triage |
| 15–16 | Phase I synthesis, card drafts, report assembly, devlog stub, PR |

If you run out of budget: **ship the report with honest `UNVERIFIED` rows and a
`RUN-FIRST` appendix.** A partial audit that is true beats a complete one that
guesses.

---

*Filed 2026-09-16 under `docs/prompts/`. Single-use brief for one audit session;
not precedent for code changes, merges, or scope edits. The MVP scope law stays
`docs/05-mvp.md` and the numbers stay `docs/02-gdd.md` — this prompt only
measures the distance between them and the tree.*
