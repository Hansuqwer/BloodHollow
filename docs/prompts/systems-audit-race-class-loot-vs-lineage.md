# Systems audit — current race/class/loot vs. Lineage 1+2 reference

*Written 2026-09-16 for the next agent session. Read `AGENTS.md` first; this
brief assumes it. This is a READ-ONLY analysis task: produce documents, not
code. Do not bump `kJournalEpoch`, do not touch sim, do not file task cards on
the board — draft card *sketches* only, the director decides what gets filed.*

## Mission

BLOODHOLLOW (2D MMORPG, C++20 raylib client + headless server, ENet, SQLite).
Audit the repo's **current race, class, and loot systems** (plus crafting and
pledge/clan, which the comparison drags in), then compare them feature-by-
feature against the Lineage 1 / Lineage 2 systems reference stored at
`docs/research-notes/lineage-comparison/l1-l2-loot-craft-class-reference.md`
(Part A = research prompt describing L1/L2 mechanics; Part B = a proposed
BloodHollow adaptation written WITHOUT repo access — its assumptions about our
code are placeholders and must be verified, not trusted).

Deliver a gap analysis the director can use to decide which (if any) of the
Part B systems earn task cards post-MVP.

## Ground truth & reading order

1. `AGENTS.md` — hard rules (determinism via `sim/rng.h`, epoch law, no
   whole-file clang-format, docs-with-code).
2. `docs/02-gdd.md` — design intent. **If the reference doc conflicts with the
   GDD, flag it; the GDD wins until the director amends it.**
3. `docs/01-research.md` — era-research matrix (Helbreath/Mir2/Soma/DarkEden/
   Lineage). Note which source games already informed current systems; the
   comparison must say when an L1/L2 proposal *displaces* an existing
   researched choice.
4. `docs/research-notes/lineage1/findings.md` — existing L1 notes (art/siege
   focused; confirms the director already mines L1).
5. `docs/05-mvp.md`, `docs/04-roadmap.md` — MVP scope fence. Proposals that
   breach MVP scope get tagged `post-MVP` automatically.

## Current-state facts (verified pointers — confirm, don't re-derive)

| System | Where | Shape today |
|---|---|---|
| Classes ("kits") | `shared/content/kits.h` | Flat enum `kKitUnsworn/Ravager/Gravecaller/Cultist` ("Pale Choir"); `KitDef` = stat seed (sums to legacy 24) + 10 skill-channel unlock levels. **No race system exists.** |
| Items | `shared/content/items.h` | `ItemDef` slots 0..6 (weapon/armor/helm/amulet/ring/consumable/junk), rarity 0..3 (T-159), affixes 1..20, `kGearDrops` side-table, `kUniqueDrops` boss uniques (fixed item+affix+title, per-row independent rolls), vendor/fence stock lanes. |
| Loot roll | `server/src/world.cpp` (~L3500 killMob; `nightLootPct` +25% night rides, T-062) | **Single** `lootItemId` + `lootChancePct` per `MobDef` + `goldLo/goldHi` (`shared/content/mobs.h:26-28`). No drop groups, no spoil, no party loot modes, no world-drop ownership timers. |
| Crafting | `server/src/world.h:325-329` (`tryAnvil`, `kKarmaAnvilOk=2`, `kKarmaAnvilDestroy=-6`) | Anvil refine/upgrade only; karma-gated tithe/destroy. No recipes, no materials, no crafter class. |
| Karma/moral economy | `server/src/world.h:100`, `world.cpp` (~L3900) | 3 bands (T-057 `karmaBand`); lawful >500 earns +15% XP; chaotic earns +15% gold loot. Drop-on-death rules: verify current behavior and cite it. |
| Pledge/siege | T-151 (wire msgs 117..119, epoch 28), `shared/content/towns.h` (town war: Thornwall/Marrowgate) | Wire + HUD landed; verify what sim state actually exists vs. wire-only. |
| Determinism law | `sim/rng.h`, `server/src/main.cpp` (`kJournalEpoch` 29) | All gameplay RNG through seeded xoshiro256**; any sim-semantics change = epoch bump + fresh gate leg. |
| Content-as-code | `shared/content/*.h` | All content is `constexpr` C++ tables shared by server/client/bots/tests — there is **no JSON data pipeline** (nlohmann/json is vendored but content is code). Part B assumes JSON; this is a real architectural divergence to analyze, not paper over. |

## Deliverables (in this order)

Write one findings doc: `docs/research-notes/lineage-comparison/systems-gap-analysis.md`.

### 1. Current-state inventory (cite file:line for every claim)

End-to-end walk of each subsystem as it exists NOW: kit selection (`/kit`),
skill channels, item model + affix/rarity pipeline (T-159), the killMob loot
roll including night modifier and karma interaction, gold curve per mob level
(extract the actual `goldLo/goldHi` table and plot gold/XP per kill by level),
anvil refine rules, pledge/siege wire vs. sim reality, vendor/fence economy.
Include the "tunable knobs" that exist today (every constant a designer can
touch, with its file:line).

### 2. Gap matrix: current vs. L1/L2 reference

Side-by-side table, one row per subsystem from the reference doc's Part A/B:
drop groups, currency scaling, level-gap penalty, party loot modes, loot
ownership timers, spoil/sweep, death-drop rules, material tiers, recipes +
Create Item, crystallize/grades, enchant (vs. our anvil), soul crystals/SA,
class tree (race→base→changes), support/battery classes, pure tanks + hate,
Royal/pledge-leader class, clan levels/skills/halls, sieges.
Columns: `L1/L2 mechanic` | `Part B proposal` | `BloodHollow today (file:line)`
| `gap: none/partial/absent` | `conflicts with GDD? (cite §)`.

### 3. Fit assessment

For each `absent`/`partial` row, judge against existing direction:
- Does `docs/01-research.md` already source this mechanic from another era
  game (e.g., crafting-lite decision, class design)? Say what it displaces.
- Does it fit the deterministic-sim + content-as-code architecture? Price the
  architectural delta honestly (e.g., Part B's JSON tables vs. our constexpr
  headers — who consumes `shared/content` today: server, client, bots, tests).
- MVP scope: tag each row `MVP-feasible` / `post-MVP` / `rejected-with-reason`.

### 4. Adoption recommendation + phased sketch

Take the reference doc's Part B6 phase list and re-map it onto reality: which
phases survive, in what order, and what each costs in **epoch bumps, protocol
version bumps, and replay-gate legs** (the repo's real currency of change).
For each surviving phase, draft a **task-card sketch** (title, acceptance
criteria bullets, files touched, epoch/wire impact) in an appendix — do NOT
write files into `docs/tasks/`.

### 5. Risks & open questions for the director

Numbered list: design conflicts, economy risks (the reference doc's own
"known exploits" section applied to us), determinism risks (drop-group rolls
and weighted picks change the RNG stream — epoch law applies), and every
place Part B's `<placeholder>` assumptions proved wrong about our code.

## Format rules

- Tables > prose. Every current-state claim gets `file:line`. Every Lineage
  claim cites the reference doc's section, not outside knowledge.
- Mark anything you could not verify from the tree `[unverified]` — do not
  guess numbers.
- The reference doc's Part A is a *research prompt about L1/L2*, not
  BloodHollow law; treat its numbers as the comparison baseline only.
- Keep the findings doc under ~600 lines; move raw table extracts to an
  appendix section at the bottom.

## Done criteria

- `docs/research-notes/lineage-comparison/systems-gap-analysis.md` committed
  with all five sections; gap matrix covers every Part B subsystem.
- Zero code changes (`git status` shows only the new docs).
- Deviations from this brief listed at the top of the findings doc.

