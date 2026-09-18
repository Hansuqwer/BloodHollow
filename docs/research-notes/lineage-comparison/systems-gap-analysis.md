# Systems gap analysis — current race/class/loot vs. Lineage 1+2 reference

*READ-ONLY analysis. Zero code changes; no `kJournalEpoch`,
wire, or schema bump; no task cards filed (sketches only).
Tree: `task/T-WAVE2-epoch30` @ `cb44964` (epoch 30, wire 241,
schema v15, suite 325/325 headless). Reference:
`docs/research-notes/lineage-comparison/l1-l2-loot-craft-class-reference.md`
(Part A = research prompt, Part B = external proposal WITHOUT
repo access — placeholders verified below, never trusted).
Brief:
`docs/prompts/systems-audit-race-class-loot-vs-lineage.md`.
Prior draft (superseded, kept for history):
`docs/research-notes/lineage-comparison/TECH-DRAFT-systems-vs-lineage.md`.*

## Deviations from the brief

1. Brief §"Current-state facts" says epoch 29 — tree is **epoch
   30** (`server/src/main.cpp:199`). All pricing uses 30.
2. Brief says `KitDef` has "10 skill-channel unlock levels" —
   tree has **12** (`shared/content/kits.h:28`), ch11 spare.
3. Brief cites `server/src/world.h:325-329` for tryAnvil — tree
   has it at `world.h:336-338` (+ pledge block above it).
4. Brief cites `server/src/world.h:100` for karma — Entity karma
   field is `world.h:106`; `karmaBandOf` decl is `world.h:200`,
   body `world.cpp:3670`.
5. Brief cites killMob "~L3500" — body is at
   `server/src/world.cpp:3429`.
6. Reference has no "known exploits" section of its own (only
   Part A §4 asks for one); §5 below applies the listed pain
   points (bot-farming spoil, enchant scams, pledge alt abuse)
   to our tree instead.
7. `docs/01-research.md` is the 4-ancestor matrix (Helbreath /
   L1 / Dark Eden / Mir2) plus Soma field note — §3 cites it
   per mechanic, not per row, to stay under 600 lines.
8. Raw table extracts live in Appendix A (bottom), not a
   second file.

## Reading map

- Ground truth order followed: AGENTS.md law (rng/epoch/wire) >
  GDD BIBLE v2 (`docs/02-gdd.md`) > reference Parts A/B >
  existing code. **GDD wins every conflict.**
- Lineage claims cite the reference doc's section (B2.x etc.),
  never outside knowledge. Part A is baseline only.
- Every current-state claim carries `file:line`. Anything not
  verified from the tree is marked `[unverified]`.

---
## 1. Current-state inventory (every claim cited)

### 1.0 Versioning law — the currency of change

| Law | Value / site |
|---|---|
| RNG | xoshiro256** + splitmix64, `shared/sim/rng.h:1-44`; `range/lo/chance` only; `<random>` forbidden (`AGENTS.md:37-44`) |
| Tick | 20 Hz, integer ticks/tiles (`AGENTS.md:45-48`) |
| Epoch | `kJournalEpoch = 30`, `server/src/main.cpp:199`; sim change = bump + fresh leg (`logs/wave2.bwj`, mm=0); old leg refuses exit 4 (`main.cpp:1838-1842`) |
| Wire | `202 + len(messages)`, `tools/protogen/protogen.py:82` = **241** (42 msgs + base 202; `shared/protocol/gen/messages_gen.h:14`); field-adds bump BASE (T-142 200→201, T-159 201→202); msg-adds move count (T-167 Ch25/Ch120); old client refused reason 4 (`main.cpp:520`) |
| Schema | additive-only `user_version`, `server/src/persist.cpp:68-234`; **v15** (sex, last_death_tick, last_debt_xp, last_res_tick −7000, bounty_mob/cycle, `persist.cpp:225-234`); T-153 takes v16 on its branch |
| Content | `constexpr` C++ in `shared/content/*.h`, shared server+client+bots+tests; no JSON loader / TOML / flags |
| Auth | server never trusts client (`AGENTS.md:34-36`); single-thread tick+net loop |

### 1.1 Kit selection + skill channels

- Kits: flat enum `kKitUnsworn/Ravager/Gravecaller/Cultist`, `shared/content/kits.h:11-13`; `KitDef` = 5-stat seed (sums to legacy 24; Ravager exact 8/8/8/0/0) + `chUnlock[12]`, `kits.h:19-31`.
- ch1 Power Swing, ch2 Mend, ch3 Bless, ch4 Ironskin, ch5 Firebolt, ch6 Chorus, ch7 Mass Mend, ch8 Haste, ch9 Purify (Cultist 6, T-082), ch10 Resurrect (Cultist 20, T-161); ch11 spare (`kits.h:22-27`).
- `/kit <name>` one-time swear: `main.cpp:1127-1135` → `Command::kKitChoose` → `World::kitChoose`, `world.cpp:185-196` (keeps assigned pts over seed). Journal `k` sidecar, `main.cpp:1966-1969`; pre-v2.1 defaults Ravager, `main.cpp:2034-2038`.
- Creation (T-167, 1-char-alpha): fresh Hello → `LoginResult ok + CharCreatePrompt 120` → client parchment → `CharCreate 25` (class 1..3, sex 1..2) → spawn, `main.cpp:411,620-660`; bots answer name-hash; legacy sex 0 → hero fallback.
- `trySkill` gates on `kitSkillUnlock(classId, ch)` + level, `world.cpp:898-918`; `SkillUse 11`, `messages.md:71`.
- Resurrect = post-mortem debt rebate ≤6000t (NOT corpse-raise: 3 s respawn), 25 MP, 6-tile same-zone, 6000t CD init −7000 (`world.h:90-94`, `done/T-WAVE2.md`). Rest of spine → open `T-161b-kit-spine-part2.md`.
- **No race system exists.** No tree/quests/hate/clan-targeting (see §1.5).

### 1.2 Item model + affix/rarity pipeline (T-159)

| Piece | Site |
|---|---|
| `ItemDef` slots 0 weapon/1 armor/2 helm/3 amulet/4 ring/5 consumable/6 junk | `items.h:19-28` |
| ~37 rows; 12 uniques rarity 3 (fixed item+affix+title) | `items.h:33-84,132-145` |
| Rarity consts 0..3 + count | `items.h:8-12` |
| `InvSlot` iid/qty/equipped/aura/durability/affix/refine/rarity | `world.h:42-51` |
| Blob `iid:qty:equipped:aura:dura:affix:refine:rarity;`, legacy defaults | `world.h:53-62` |
| Affix names 1..20, count | `items.h:99-104` |
| Slot-gated probe `hasAffix(e, affix, slot)` 0 weapon/1 armor/9 any + awake | `world.h:257-259`, `world.cpp:381-392` |
| 20 hooks: 11 +3 flat, 12 +1 OOC, 13 −10% incoming, 14 3% leech, 15 +2acc/+1evd, 16 +5% crit, 17 +4 night, 18 +2 def, 19 +15% gold, 20 +8 sub-20% (v1–v2 in `items.h:88-92`) | `world.cpp:375,862-872,928,2705,2738-2748,3603-3604,4410-4411` |
| Gear side-table 4 rows (1002→2001@4%, 1005→2002@3%, 1006→2101@3%, 1007→2102@2%) | `items.h:107-117` |
| Unique table 12 rows (1012/1013/1014 ×3 @4%, 1009 ×3 @6%) | `items.h:125-150` |
| Grant path (fixed affix, broadcast, inv-full = nothing) | `world.cpp:3408-3427` |
| Sell 40% | `items.h:31` |

### 1.3 killMob loot roll + night + karma (end-to-end)

Order in `World::killMob`, `world.cpp:3429`:

1. Party XP share, `world.cpp:3436-3477`: alive same-zone
   party in `kPartyXpRadius` split `xpValue` + bonus; night
   x110/100 (T-062); night-spawned at night x150/100 (T-162,
   `nightSpawned`, `world.h:175`). **Loot/gold stay killer.**
2. Field-war EK, `world.cpp:3484-3495`: sworn killer vs
   opposite-town patrol (`MobDef.town`, `mobs.h:35-37`; rows
   1024/1025) +1 EK, no stain.
3. Bounty pay, `world.cpp:3515-3533`: held mark + cycle match
   → payout, one-kill drain (`bountyMobId/cycle`,
   `world.h:167-168`).
4. Gear roll, `world.cpp:3535-3572`: `findGearDrop` hit (night
   x125/100) → inv cap 32 → rarity `1-100: ≤78 C / ≤95 M /
   ≤99 R / else U-slot` → M/R roll ONE affix `1..20`; Common
   0; U-slot unused (boss path owns uniques). Direct push.
5. Unique rolls, `world.cpp:3573-3581`: per-row
   `range(1,100) <= chancePct` (night x125/100) →
   `grantUniqueDrop`.
6. Junk roll, `world.cpp:3582-3596`: single `lootItemId +
   lootChancePct` (`mobs.h:26-27`, night x125/100) → `addItem`.
7. Gold, `world.cpp:3597-3633`: uniform `goldLo..goldHi`
   (`mobs.h:28`) → Greed x110 → Tithemaster x115 → chaotic
   x115 (`karma<0`) → −5% castle tithe (holder/vault,
   `world.cpp:3606-3630`).
8. Whitening, `world.cpp:3637-3640`: `mob.level <= killer.level`
   → `bumpKarma(+1)`.
- Draws per kill today: up to 1 (gear gate) +1 (rarity) +1
  (affix) +N (uniques on 4 mobs) +1 (junk) +1 (gold).
- Tunable knobs: per-row `lootItemId/lootChancePct/goldLo/Hi/
  xp` (`mobs.h:14-38`); 4 gear rows; rarity literals
  (`world.cpp:3546-3549`); 20 affix sites; night literals
  x125/x110/x150; Greed/Tithemaster/chaos/tithe multipliers.

### 1.4 Gold/XP curve (literals, day base; night §1.3)

See Appendix A table. Shape: L1 40xp/6-14g → L7 300xp
(Maw 1200xp) → L9 420xp (Red Widow 1680xp) → L12 820xp
(elites 3200-4100xp) → L14 Gravemother 4000xp/400-650g;
L15 Guard 0xp wall (`mobs.h:42-91`).

### 1.5 Anvil refine rules

- `tryAnvil`, `world.cpp:2248`; karma +2 ok / −6 destroy,
  `world.h:336-338`. Tiers `kAuraTiers`, `auras.h:9-30`:
  T1 30×4001+120g … T5 3×4005+8000g; reqSkill; mercy bit;
  fail −1 safe / 0 soft / 1 destroy (slot erased, T-115).
- Shipped GDD §7 table (+0..+7, shatter@+3, −1 @+4..6, reset
  @+7, glow +5): `docs/02-gdd.md:217-231` (ADR-0013). No
  recipes/materials/books/Create-Item/crystallize/grades/
  scrolls/souls. Junk/ore = currency (1:5 ore fallback).
- Lite deferred NOT built: T-159b (Crystalize 10×junk→shard,
  8 fixed recipes, spoil-flag).

### 1.6 Karma / death / PK (verify per brief)

- Bands `karmaBandOf`, `world.cpp:3670-3673`: <0 chaotic red;
  >500 lawful; else neutral. Lawful +15% XP, chaotic +15%
  gold (`world.h:106` + `world.cpp:3605`).
- Unlawful PK vs L15-pinned: −(300+20×deficit) + wanted;
  guard 8-tile → 240 s wanted (`world.cpp:3848-3875`).
  War kills (field-war EK, `world.cpp:3484`) + guard-mobs
  exempt; duels exempt (`duelUntil`, `world.h:148`).
- Chaotic death: equipped 15%/slot + 1–6 bag picks,
  `world.cpp:3891-3935`. All: −5 durability surviving gear
  (T-081), XP debt 10→25% bar L1→L25 + de-level
  (`world.cpp:3936-3970`). Lawful PvE death drops nothing.

### 1.7 Pledge/siege wire vs. sim reality (verify per brief)

- SIM REAL: create/invite/accept/leave/kick/rank/disband/chat/
  who/tithe/vault-readout all live, `world.cpp:3076-3330`;
  registry `pledges_` NOT in worldHash by design (social, not
  sim), `world.h:293-297`. Vault deposit-only
  (tax-drip + tithe), `world.cpp:3253-3280,3606-3630`.
- Persisted: `pledges` table (name/emblem/liege/vault_gold)
  v13 + vault col v14, `persist.cpp:199-218`.
- WIRE (T-151, epoch 28): `SiegeState 117` (holder/window/
  gates/heart/crown/bands/phase/vault/crowns),
  `PledgeRoster 118` + `PledgeMember 119`,
  `messages.md:323-363`. HUD shipped; M4 FAIL-open (0 flips
  m4e29/b/f3a, `done/T-157.md`).
- NOT sim: levels/skills/warehouse/halls/wars/academy/
  alliance (all absent — see §2).

### 1.8 Vendor / fence economy

- Marta `kVendorStock` 14 ids, `items.h:163-164`; Sable
  `kFenceStock` 3 ids + 125% markup + 60% pawn vs 40%,
  `items.h:171-176`. Chaotics refused by Marta
  (`world.cpp:1264,1318,1357`); wanted refused; fence red-only
  crate (`world.cpp:1431`). Repair-all 1g/2pts (all-or-nothing).

### 1.9 Oath / town-war / bounty (sim reality)

- Oath L19 one-way, never respec, `world.cpp:3676-3694`
  (`kTownOathLevel 19`, `towns.h:10-13`). EK board top-5 +
  `/ek`, `world.cpp:3696-3730`. Town-affiliated patrols
  1024/1025 (`mobs.h:89-90`).
- Bounty single-mark persisted (v15 cols), one-kill drain,
  pickup-N cut (T-166), `world.cpp:3515-3533`.

## 2. Gap matrix (ref § → proposal → tree → gap → GDD)

### 2.1 Loot + drops (Part A §§1.1/2.1/2.2/3/4 → B2.1-B2.4)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD? |
|---|---|---|---|---|
| Per-monster item list + chance + min/max (A1.1/A2.1) | JSON per mob: currency{chance,min,max} + drop_groups[{group_chance, items[{weight,min,max}]}] (B2.1) | ONE junk roll `lootItemId+lootChancePct` qty 1 (`mobs.h:26-27`, `world.cpp:3582-3596`) | absent | GDD §7 silent on groups; no conflict |
| Group→item two-stage roll (A2.1) | `roll_loot`: group gate then `weighted_pick` + multi-roll (B2.2) | no groups, no weights, no multi-roll | absent | no conflict |
| Adena formula vs level (A2.1) | `base = level² × k` (B2.4) | literal `goldLo/Hi` per row (§1.4/App A) | partial (gold exists, formula absent) | no conflict |
| Level-gap penalty table (A2.1) | 0–8:1.0 … 14+:0 (B2.2/B7) | none | absent | no conflict |
| Champion/raid multipliers (A2.1) | `champion_mult` (B2.2) | elites = separate rows ×4–5 XP (1012-1014/1019-1020); no multiplier knob | partial | GDD §9 named timers; no conflict |
| Party loot modes ×5 (A2.1) | `distribute()` strategy: finders/random/±spoil/by-turn (B2.3) | XP shares only; loot/gold killer-takes-all (`world.cpp:3436-3477`) | absent | partial — GDD §4 party = XP curve + heals-count; loot modes unmentioned |
| Raid loot rights damage+timer (A2.1) | top-damage party 300 s (B2.3) | none (boss = same direct rolls) | absent | no conflict |
| Ownership timers 15 s (A2.1) | world items owned killer/party → FFA (B2.3) | no world items; direct inv push | absent | no conflict |
| Spoil pre-mark + Sweep collect (A2.2) | Boneraker Spoil formula + Sweep/ Festival (B4.3, B2.2-3) | T-159b spoil-flag sketched only | absent | no — T-159b owns lite slice |
| Herbs/bind-on-drop (A2.1) | optional phase 3 (B2.3) | none | absent | OUT — GDD §12 (no pets-as-loot-adjacent bloat) |
| Manor/seeds, Seven Signs/Ancient Adena, weight/inv limits, class-locked items (A2.1/A1.1) | Old Blood Coin merchant (B2.4); grade gear gates (B3) | no manor/seals; inv cap 32 (world.cpp:3539); no weight; no class locks | absent | OUT (GDD §12) except cap-by-design |
| Boss vs normal drops (A1.1/A2.1) | raid flag + separate tables (B2.1) | per-row unique rolls on 4 mobs (§1.2) | partial | GDD §9 named elites; no conflict |
| Drop-on-death PvE/PvP + karma slots (A1.1/A2.1) | threshold 1–N drops (B2.3) | chaotic 15%/slot + 1–6 bag; lawful PvE none (§1.6) | partial | none — matches GDD §5 fantasy |

### 2.2 Crafting / crystallize / enchant / souls (A §§1.2/1.3/2.3-2.6 → B3.1-B3.7)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD? |
|---|---|---|---|---|
| L1: no player craft, NPC exchange only (A1.2) | n/a (L2 model chosen) | Anvil refine only (§1.5) — same shape as L1 law | none | none — GDD §7 Anvil only |
| Material taxonomy raw→key parts (A2.3) | T0–T4 + frames, bible metals (B3.1) | junk/ore as currency only | absent | partial — GDD §7 ore mining only; T-159b caps lite |
| Recipe types + acquisition (A2.3) | dwarven/common JSON, drop/shop/quest, book slots (B3.2) | none (T-159b: 8 fixed, no fail) | absent | YES if % fail/shop breadth — GDD §7 gamble spine is refine |
| Create Item 1–10 + MP + success (A2.3) | Forgehand 5+ grade unlocks; Common to Iron (B3.3-4) | none | absent | YES — needs Deepkin race (GDD §3 Human-only MVP) |
| Private craft shops/Manufacture (A2.3) | stationary mode + fee escrow (B3.4) | none | absent | YES — stalls OUT, GDD §12 |
| Crystallize + yields (A2.4) | Deepkin skill, shards ×(1+0.1×ench) (B3.5) | T-159b Crystalize sketched (10×junk→shard) | absent | no — T-159b owns it |
| Grades + penalty (A2.4) | Ash→Heartblood 6 grades (B1/B3.6) | none (all gear equippable) | absent | OUT pre-v0.2 |
| Safe enchant +3/+4, 66%, fail→crystals, blessed/cursed (A1.3/A2.5) | Blood Sigils normal/blessed/cursed + per-grade bonus (B3.6) | Anvil mercy + −1/0/1 (§1.5, GDD §7 table) — RIVAL law | partial (rival) | YES displaces — ADR-0013 + GDD §7 |
| Mammon unseal/services (A2.5) | Blacksmith NPC attach (B3.6-7) | Bonesmith = aura vendor only | absent | OUT |
| Soul crystals/SA stages + absorb (A2.6) | Bound Souls 1–13 + attach B+ (B3.7) | none | absent | OUT — v0.2 earliest (GDD §10/12) |
| Augmentation/attributes (A2.6 app.) | optional (B6.7) | none | absent | OUT |

### 2.3 Class tree + races (A §§1.5/2.7 → B4.1-B4.4/B4.8)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD? |
|---|---|---|---|---|
| Race→base→1st(20)→2nd(40)→[3rd] (A2.7) | 5 races, JSON templates (B4.1-2) | flat 3 kits + Unsworn (§1.1) | absent | YES — GDD §3: 3 classes, cap 25, Human-only |
| Dwarf Scav→Bounty→Fortune / Art→Wars→Maestro (A2.7) | Boneraker→Grave-Reaver / Forgehand→Warsmith (B4.2-3) | none | absent | YES — needs race law |
| Spoil/Sweep/Crystallize/Create/Golems (A2.7) | skill table w/ levels (B4.3) | none (pet arrives via T-161b Raise) | absent | YES — same race gate |
| Class-change quests + coupon (A2.7) | gather/craft/NPC chain + Borrowed Blade (B4.8) | creation + `/kit` window (§1.1) | absent | YES — quests-as-content OUT (GDD §12) |
| Royal: pledge gate + weak-1v1 + auras (A1.4) | Oathsworn→Hollow Lord, Found/Call/Rally/Auras (B4.4) | any L10+10k founds; no Royal | absent | YES — 1-char-alpha (T-167) + no-faction MVP (GDD §1/5/10) |
| L1 class list + roles/stats/sigs (A1.5) | 3-kit MVP + T-161b control sets (no L1 import) | Ravager DPS/off-tank, Gravecaller burst/control, Cultist support (GDD §3, `kits.h`) | partial (cast exists, L1 list not imported) | none — GDD §3 IS the list |
| Research note already sourced: Prince-gate → CHA stat (`01-research.md:40-41`) | Royal class (B4.4) | CHA cut (ADR-0015); gate = L10+10k (GDD §3) | absent by prior decision | YES — re-opens decided mapping |

### 2.4 Support / tanks / hate (A §§2.8/2.9 → B4.5-B4.7)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD? |
|---|---|---|---|---|
| EE vs SE: sustain vs offense batteries (A2.8) | Grove/Hollow Elder split (B4.5) | ONE Cultist spine ch2/3/6/7/8/9/10 (§1.1; Taoist×Enchanter per `01-research.md:66`) | partial | partial — single-support IS the Friday-Night fantasy (`05-mvp.md:111-112`) |
| Recharge/mana battery (A2.8) | EE Recharge, SE Vampiric/Death Whisper (B4.5) | no mana transfer; MP via MAG + vials | absent | no conflict |
| Resurrection (A2.8) | EE/SE rez | Resurrect-as-rebate ch10 (§1.1) | partial (rival shape) | none — GDD §3 Resurrect-20 |
| Party chants vs clan-only seals (A2.8) | Warhowler party / Oathcaller pledge==filter (B4.6) | Chorus party-wide (T-054b); no pledge filter | partial | partial — filter buildable, Royal gate not |
| Pure tanks + Aggression/Hate Aura + block/shield kit (A2.9) | 4 Wardens + `add_aggro` + role flag (B4.7) | Ravager off-tank (armor shred/gap-close, GDD §3); no hate, no block | absent | no — but needs M3 justification |
| Summons/cubics/reflect (A2.9) | Warden signatures (B4.7) | Raise Skeleton deferred (T-161b.3); no cubics | absent | OUT pre-T-161b |

### 2.5 Clan / oath / siege (A §§1.4/2.10 → B5)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD? |
|---|---|---|---|---|
| Founding gate (A1.4/A2.10) | Royal-only + oath_seal (B5) | any L10+10k (T-138) | absent | YES — same Royal gate |
| Ranks + privileges (A2.10) | 4 ranks + bitmask (B5) | Liege/Bloodsworn/Initiate + kick/rank (`world.cpp:3076-3330`) | partial | no conflict |
| Clan levels 0–8 + costs (A2.10) | SP/marks/members (B5) | none (flat) | absent | OUT (GDD §10 v0.2) |
| Clan skills + reputation (A2.10) | rep-bought +HP/PDef (B5) | none | absent | OUT |
| Warehouse + permissions (A2.10) | rank perms (B5) | vault-GOLD only, no items/perms (§1.7) | partial | partial — perms is the sane slice |
| Halls auction (A2.10) | bidding (B5) | none | absent | OUT |
| Wars + Blood Marks, emblem = free PK (A1.4/A2.10) | declare → karmaless (B5) | field-war EK vs patrols only (§1.9); pledge-vs-pledge absent | partial | no — natural extend (GDD §5 lawful PvP incl. pledge wars) |
| Academy/alliance (A2.10) | apprentices, N-oath chants (B5) | none | absent | OUT (GDD §10) |
| Castles 7/siege windows/tax (A1.4) | 3–5 strongholds + golems (B5) | 1 castle, weekly 90-min, 5% tax (§1.7) | partial | none — 1 castle IS MVP (`05-mvp.md:23`) |
| Siege golems (A2.7) | Warsmith summon (B4.3/B5) | none | absent | YES — race gate |

## 3. Fit assessment (per absent/partial row)

Tags: `MVP-feasible` / `post-MVP` / `rejected-with-reason`.
Research-source column = which `01-research.md` ancestor
already decided this (a proposal that displaces one names it).

### Loot & drops

| Row (§2.1) | Researched choice it meets/displaces | Arch fit + honest delta | Tag |
|---|---|---|---|
| Drop groups + weights | Displaces NOTHING (never built); T-159 deviation 2 banked it | Fits constexpr (new structs in `mobs.h`, roll in `killMob`); every new draw = epoch + leg | post-MVP (wave opener) |
| Currency formula | Displaces literal table (M2 evidence built on it) | 10 lines; same epoch cost as any award change | post-MVP (needs economy reason) |
| Level-gap penalty | No prior choice; L6-wall postmortem says density, not gap | 20 lines + table; burns M2 TTK/XP-hr chain | post-MVP (defer to evidence) |
| Party modes / ownership / raid rights | Mir party-scaling (XP curve, `01-research.md:62-63`) covers XP; loot modes are NEW social law | Needs world-item entity + wire + client + bots; largest loot card | post-MVP (after groups prove) |
| Spoil/sweep | T-159b spoil-flag OWNS the lite slice | Rides corpse law (T-161b.4 owns corpse entity) or waits | post-MVP (via T-159b → T-161b.4) |
| Death drops | L1 alignment stewarded WHOLE (`01-research.md:32,38`); current law IS the steal | No change needed | none (keep) |
| Herbs/manor/seals/weight/locks | No source; OUT by §12 | New systems + UI | rejected (GDD §12 OUT) |

### Craft / enchant

| Row (§2.2) | Researched choice | Arch fit | Tag |
|---|---|---|---|
| Materials/recipes/books/Create | Crafting-lite DECIDED (`01-research.md:22`; T-159b caps it) | Needs loader + persist + skill channels; displaces T-159b | rejected-with-reason (re-price on retention only) |
| Crystallize | T-159b OWNS lite form | Verb + shard ids; no new model | MVP-feasible (as T-159b, not B3.5) |
| Grades/penalty | No source; v0.2+ gearing | ItemDef widen + equip gates + UI | post-MVP |
| Sigil enchant vs Anvil | Mir upgrade gamble ADAPTED to Anvil (`01-research.md:62,68`; ADR-0013; GDD §7) | Rival spine; amending ADR-0013 = economy re-proof | rejected-with-reason (Anvil stands) |
| Souls/augments/shops/Mammon | No source; shops OUT (`05-mvp.md:33`) | New machines + escrow + NPC | rejected (OUT) |

### Classes / support / tanks

| Row (§2.3–2.4) | Researched choice | Arch fit | Tag |
|---|---|---|---|
| Race tree + Dwarf kits + change quests | Human-only DECIDED (GDD §3; Vampire v0.2, `01-research.md:48-49` Dark Eden asymmetry) | Migrates wire+DB+journal+art+balance | rejected (v0.2 host = Vampire, not 5-race bang) |
| Royal founder | Prince-gate → CHA DECIDED (`01-research.md:40-41`), then CHA cut (ADR-0015) | Re-opens two decisions | rejected-with-reason |
| EE/SE split, Recharge | Cultist = Taoist×Enchanter DECIDED (`01-research.md:51,66`); single-support is Friday-Night fantasy | Splits kit + balance + M3 re-proof | post-MVP (only if support meta fails) |
| Oathcaller filter (no Royal) | L1 war-declare chaos-immunity (`01-research.md:33`) + field-war EK (T-163) | Predicate on pledgeId in buff path; small | post-MVP (sketch P1-adjacent) |
| Hate/tanks | No prior tank law; Ravager off-tank per GDD §3 | hate map + targeting; epoch-priced | post-MVP conditional (M3 re-measure first) |
| Pets/cubics | T-161b.3 OWNS pet | Entity + leash + despawn law | post-MVP (via T-161b) |

### Clan / siege

| Row (§2.5) | Researched choice | Arch fit | Tag |
|---|---|---|---|
| Oath-war declare + marks | L1 emblem-war (`01-research.md:33`); GDD §5 lists pledge wars as lawful PvP | PK-path check + coin; E-priced | post-MVP (only new pledge slice) |
| Warehouse-perms | No source; guild-hosting | S + UI + perm tests | post-MVP (first hosting slice) |
| Levels/skills/halls/academy/alliance | v0.2 map (GDD §10) | Each S + UI | rejected (v0.2+) |
| Extra castles/golems | 1-castle MVP (`05-mvp.md:23`; `01-research.md:34` simplify) | Maps + scheduler + balance | rejected (OUT) |

## 4. Adoption recommendation + phased sketch

B6 proposed 0→7 behind flags (`features.loot_v2` …). NO flag
framework exists (nearest precedent: content-only widening
`chUnlock` 10→12, no wire/DB). Re-map onto constexpr +
epoch/wire/schema currency (E/W/S). Change control applies:
`05-mvp.md:37` (IN +ADR to add).

| Order | Phase (verdict) | E/W/S | Why this order |
|---|---|---|---|
| 0 | JSON externalization (REJECT as stated) | — | keep constexpr; grow structs. JSON-loader would be its own ADR + card first |
| 1 | Loot groups v1 (SURVIVES, wave opener) | E+1 | closes T-159 deviation 2 with data; batch K2/P1 under same E if ready |
| 2 | T-159b lite-craft (SURVIVES, already specced) | E+0..1, W+0, S+0 | no new slice; spoil-flag rides T-161b.4 corpse law |
| 3 | Oath-war declare + Blood Marks (SURVIVES, only new pledge slice) | E+1, S+0..1 | mutual-declare + cooldown; siege blackout ruled |
| 4 | Hate-weight tank lever (CONDITIONAL) | E+1 | only if M3 re-measure (fuller T-161b kit) still shows solo meta |
| 5 | Ownership + one party mode (SURVIVES, after L1) | E+1, W+1 | finders-keepers first; client marker + bot pickup |
| 6 | Warehouse-perms (SURVIVES, first hosting slice) | S+1 | post-alpha guild work |
| 7 | Full craft / tree / souls / shops / levels / academy / castles (REJECT for MVP) | E+N, W+N, S+N | retention-gated (D1/D7); v0.2+; each own ADR + §37 cut |

Recommendation: NOTHING above lands pre-MVP except T-159b
(already the ceiling) — the tree is in Friday-Night/M5
evidence mode, not new-spine mode.

### Appendix B — task-card sketches (NOT filed; director decides)

- **SK-1 LOOT-GROUPS-V1.** Per-mob 1–3 weighted groups on 4
  farmed rows (Ghoul/Gnoll/Widow/Sexton), deterministic
  `rng_.range` order, 100k fixed-seed distribution pins (T-159
  pattern), epoch + fresh leg, GDD §7 amend. Files: `mobs.h`,
  `world.cpp`, `main.cpp`, loot test, leg. **E+1.**
- **SK-2 OATH-WAR.** Mutual declare, karmaless-vs-target-oath
  PK check, Blood-Mark purse coin + readout, abuse pins
  (cooldown, siege blackout). Files: `world.cpp/h`, karma/PK
  tests, GDD §5/§8. **E+1, S+0..1.**
- **SK-3 HATE-LEVER (conditional).** hate map + add_aggro +
  Aggression skill, targeting honors hate in leash, M3
  re-measure attached (fuller kit first). **E+1.**
- **SK-4 OWNERSHIP-V1.** World-item entity + owner/expiry +
  pickup + 15 s timer (raid 300 s later), finders-keepers
  only, client marker + bot support. **E+1, W+1 (base).**

## 5. Risks & open questions for the director

1. Epoch economy: SK-1/2/3 each want E+1 — one shared
   post-MVP wave bump (single leg + guard) or apart?
2. Transferred exploits (from Part A §4's ask — bot-farming
   spoil funnels, enchant-insurance via blessed, Royal-alt
   oath spam, KS-script pickup races): which survive contact
   with our direct-to-inv + limiter (T-109) + allowlist
   (T-152) posture? Each adopted slice needs its abuse pin.
3. Determinism: EVERY added draw (groups, weights, gap
   scaling, spoil bags) re-rolls the stream — epoch law
   applies, tuning churn = epoch churn. Batch, don't dribble.
4. Grey-farming (L3): is the anti-social cost (no PL help)
   acceptable for a friends-invite alpha? No evidence yet.
5. Royal fantasy vs 1-char-alpha: does leadership need a
   class, or do oath-war + banner-aura items cover it?
6. Anvil vs Sigils: room for TWO gamble spines, or does
   enchant talk end with ADR-0013? (Staff: ends.)
7. Corpse law: does T-161b.4 create the entity spoil needs,
   or stay spell-only? (Spoil waits for the answer.)
8. Placeholders proven wrong about our code: `<items file>`
   = `items.h:19-28` constexpr (not JSON); `<loot code>` =
   `killMob` 6-stage direct-push (`world.cpp:3429-3640`); `<craft
   code>` = `tryAnvil` + `kAuraTiers` (no recipes); `<class
   defs>` = `KitDef` 12-channel (rides wire+DB+journal);
   `<skill defs>` = channel unlocks + `trySkill` gates (no
   trees); `<clan code>` = pledge-lite SHIPPED
   (`world.cpp:3076-3330` + wire 117-119); JSON/flags/TOML =
   none exist; `rand()` = forbidden (`rng.h` only).
9. `[unverified]`: live 200-kill all-tiers drop log (T-159f1,
   needs instrumentation); 30-bot argon2id p99 (T-153 soak);
   wave-2 graphical client compile (CI matrix flagged);
   M4 flip cause beyond F3 (b-vs-c call); EE/SE-era buff
   VALUES (reference Part B gives effects, not numbers —
   treat as direction, not tuning).

## Appendix A — raw extracts (tables, day base)

Gold/XP per row (`shared/content/mobs.h:42-91`): Rat L1
40xp 6-14g 4001@40% · Bat L2 55xp 4-12g 4001@25% · Ghoul L3
90xp 14-30g 4002@50% +2001@4% · Hound L5 150xp 26-48g
4003@60% · Spider L6 200xp 30-60g 4002@40% · Gnoll L7 300xp
60-110g 4003@55% +2002@3% · Wretch L8 340xp 70-130g
4001@40% · Widow L9 420xp 90-160g 4004@35% +2101@3% ·
Golem L9 480xp 100-180g · Cultist L10 520xp 120-200g ·
Patrols L9-10 500-560xp · Celebrant L11 560xp 130-220g ·
Sexton L12 820xp 160-260g 4005@100% · elites L12 3200-4100xp
200-320g · Gravemother L14 4000xp 400-650g 4005@100% +3
uniques @6% · Guard L15 0xp wall.
Rarity literals (`world.cpp:3546-3549`): ≤78 C / ≤95 M /
≤99 R / else U-slot; M/R one affix 1..20.
Night multipliers: junk/gear/unique chance x125/100; XP
x110/100; night-spawned XP x150/100 (`world.cpp:3444-3452,
3535-3596`). Gold chain: Greed x110 → Tithemaster x115 →
chaotic x115 → −5% tithe (`world.cpp:3603-3630`).
Anvil tiers (`auras.h:24-30`): T1 30×4001+120g … T5
3×4005+8000g; GDD §7 success table
(`02-gdd.md:217-231`).








