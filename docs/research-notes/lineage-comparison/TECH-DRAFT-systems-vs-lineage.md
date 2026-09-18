# TECH-DRAFT — BloodHollow systems vs Lineage 1/2: what exists, what the L1/L2 reference proposes, what would change

*Draft 2026-09-16. READ-ONLY analysis — no code, no epoch/wire/schema bump, no board cards filed. Source of truth is the tree on `task/T-WAVE2-epoch30` (epoch 30, wire 241, schema v15, suite 325/325). The L1/L2 reference is `docs/research-notes/lineage-comparison/l1-l2-loot-craft-class-reference.md` (Part A = research prompt, Part B = external proposal written WITHOUT repo access — its `<placeholder>` assumptions are verified below, not trusted). Task brief: `docs/prompts/systems-audit-race-class-loot-vs-lineage.md`. GDD is `docs/02-gdd.md` (BIBLE v2) — on any conflict the GDD wins until the director amends it. MVP fence is `docs/05-mvp.md`.*

## 0. TL;DR (director cut)

1. **Loot (B2): weakest fit-per-cost.** Today is one flat roll per mob (`MobDef.lootItemId + lootChancePct`, `server/src/world.cpp:killMob` ~L3429) + one gear side-table roll (`kGearDrops`, 4 rows) + per-row unique rolls + `goldLo/goldHi`. No drop groups, no weights, no spoil, no ownership timers, no party loot modes, no level-gap penalty. The reference's drop-group/ownership/party-mode layer is *genuinely missing* — but every added RNG draw changes the replay stream → **epoch bump + fresh gate leg**, and ownership/world-items need a **new entity kind + wire fields → wire bump**. Recommend: post-MVP, phased (groups first, ownership/party-modes second, spoil only with a crafter class to consume it).
2. **Crafting/enchant (B3): do NOT import wholesale.** MVP has Anvil refine only (`World::tryAnvil`, `server/src/world.cpp:2248`, tiers in `shared/content/auras.h:20-21`, karma tithe/destroy `world.h:336-338`). The deferred lite path already exists as law: `docs/tasks/T-159b-craft-lite-v0.2.md` (Crystalize-as-junk-sink + 8 fixed-success recipes + spoil-flag, explicitly NOT the full L2 chain). The reference's 5-tier materials / recipe books / Create-Item levels / grade-gated enchant / Bound Souls is a second economy and a second progression spine — it displaces T-159b and fights `05-mvp.md:37` change control. Recommend: keep T-159b as the MVP ceiling; re-price full craft only on retention evidence.
3. **Classes (B4): the tree replaces the kit spine — reject for MVP, keep as v0.2 direction.** Today is a flat enum (`kKitUnsworn/Ravager/Gravecaller/Cultist`, `shared/content/kits.h:11-13`) + `KitDef` stat seeds (sums to legacy 24) + 12 skill-channel unlocks + five-stat assign (T-160/ADR-0015) + creation flow (T-167: `CharCreate 25` / `CharCreatePrompt 120`). No races, no base→1st→2nd changes, no hate/tank framework, no clan-gated targeting. Wave-2 just shipped the support spine start (ch10 Resurrect-as-rebate, rest in `T-161b-kit-spine-part2.md`). A race→base→change tree is a rewrite of progression, starting areas, quest content, and balance evidence (M2/M3) — recommend: post-MVP; the only MVP-adjacent slice is hate/aggro weights IF party meta demands it (own card, epoch-priced).
4. **Pledge/siege (B5): extend, don't replace.** Pledge-lite is shipped and persisted (create/emblem/ranks/chat/vault-tax-only, schema v15 columns incl. `sex/last_death_tick/last_debt_xp/last_res_tick/bounty_mob/bounty_cycle`, wire `SiegeState 117 / PledgeRoster 118 / PledgeMember 119`, epoch-28 wire+HUD in T-151, field-war EK in T-163). The reference's Royal-gated founding / oath levels 0–8 / skills / warehouse / halls / wars / academy / alliance does not exist. Recommend: oath-wars (karma-exempt PvP vs one oath) + warehouse-permissions are the only slices worth sketching pre-v0.2; Royal-as-founder-class conflicts with 1-char-alpha (T-167) and the no-faction MVP law (BIBLE v2 §1/§5/§10).
5. **Architecture divergence (honest price tag):** the reference assumes JSON data + loaders + feature flags (`features.loot_v2`, `config/economy.toml`). The tree is **content-as-`constexpr` C++ headers** (`shared/content/*.h` shared by server+client+bots+tests) with **deterministic seeded RNG** (`sim/rng.h`, xoshiro256**) and **journal epoch law** (`kJournalEpoch = 30`, `server/src/main.cpp:199`) + **count+base wire versioning** (`202 + len(messages)` = 241 today: 42 messages + base 202; field-adds bump the base, T-142/T-159 precedent). There is no config-file pipeline, no feature-flag framework, no JSON content loader. Adopting the reference's shape literally means building all three first. Recommend: keep `constexpr` tables; grow them (drop-group structs, material ids) rather than migrating format.
6. **What I would file (sketches only, director decides):** (a) Loot-groups v1 (per-mob 1–3 weighted groups, deterministic order, epoch-priced); (b) T-159b lite-craft as specced (no new slice); (c) Oath-war declare + Blood-Mark coin (pledge-scoped, no Royal gate); (d) Hate-weight tank lever (only if M3 re-measure with fuller kit still shows solo meta). Everything else → post-MVP backlog with reasons.

## 1. Current-state inventory (tech-first, every claim cited)

### 1.1 Determinism + versioning law (the currency every change pays in)

- RNG: `bh::sim::Rng` xoshiro256** + splitmix64 seed,
  `shared/sim/rng.h:1-60`. No `<random>` distributions (replay
  would break). All gameplay draws use `range()/chance()/unit()`.
- Journal epoch: `kJournalEpoch = 30` in
  `server/src/main.cpp:199`. Any sim-semantics change under old
  journals bumps epoch + ships a fresh gate leg (`logs/wave2.bwj`:
  8 fighters x60s + 8 relogs, replay mm=0). Old leg must refuse
  exit 4.
- Wire: `kProtocolVersion = 202 + len(messages)` in
  `tools/protogen/protogen.py:82` = **241** today (42 messages +
  base 202; `shared/protocol/gen/messages_gen.h:14`).
  Field-adds bump the BASE (T-142 200->201 classId+sex; T-159
  201->202 rarity). Message-adds (T-167 CharCreate 25 +
  CharCreatePrompt 120) move the count. Old client refused
  reason 4.
- Schema: additive-only `user_version` migrations,
  `server/src/persist.cpp:68-234`. Current **v15** (wave-2: sex,
  last_death_tick, last_debt_xp, last_res_tick, bounty_mob,
  bounty_cycle). T-153 branch takes v16 (argon2id PHC col).
- Content-as-code: `shared/content/*.h` constexpr tables shared
  by server + client + bots + tests. No JSON loader, no TOML
  config, no feature-flag framework. Server-auth single-threaded
  20 Hz tick (`AGENTS.md`).

### 1.2 Loot pipeline today (`World::killMob`, `world.cpp:3429+`)

1. Party XP share (L3436+): alive same-zone members within
   `kPartyXpRadius` split `mob.xpValue` + bonus per sharer.
   Night x110/100 (T-062); night-spawned quarry at night
   x150/100 (T-162, `nightSpawned`, `world.h:175`).
   Loot/gold stay with the killer (era rule).
2. Field-war EK (L3462+): sworn killer vs opposite-town patrol
   (`MobDef.town`, rows 1024/1025) mints +1 EK, no karma stain.
3. Gear roll (L~3520+): one lookup `kGearDrops` (`items.h:108`,
   exactly 4 rows @2-4%). Hit -> rarity 78/17/4.6/0.4, Magic/Rare
   roll ONE affix 1..20. Direct inv push — no world item, no
   ownership timer, no party split.
4. Unique rolls (L3575+): per-row `range(1,100) <= chancePct`
   over `kUniqueDrops` (12 rows: 1012/1013/1014 x3 @4%, 1009 x3
   @6%). Night +25% relative. Fixed item + fixed affix + title.
5. Junk roll (L3583+): single `lootItemId + lootChancePct` per
   MobDef (`mobs.h:26-27`), night +25% relative.
6. Gold (L3595+): uniform `goldLo..goldHi`, then Greed x110,
   Tithemaster x115, chaotic x115, minus 5% castle tithe.
- MISSING: drop groups, weights, multi-item groups, spoil/sweep,
  ownership timers, raid damage-share, party loot modes,
  level-gap penalty, herbs, currency formula.

### 1.3 Items / affixes / rarity (`shared/content/items.h`)

- `ItemDef`: slot 0 weapon / 1 armor / 2 helm / 3 amulet /
  4 ring / 5 consumable / 6 junk. ~37 rows, 12 uniques (rarity
  3). Affixes 1..20 all hooked (11 Hollow +3 flat, 12
  Grave-touched regen, 13 Crypt -10% incoming, 14 Marrow 3%
  leech, 15 Pall +2acc/+1evd, 16 Boneyard +5% crit, 17 Dirge +4
  night, 18 Husk +2 def, 19 Tithemaster +15% gold, 20 Last
  Rites +8 sub-20% HP). One affix per item (one u8).
- Vendor Marta 14 ids / fence Sable 3 ids + 25% markup, 60%
  pawn vs 40% (`items.h:157-176`). Chaotics refused by Marta.
  `ItemSlot` wire carries rarity (T-159); chrome NOT rendered
  (deferred T-ART-15). Blob 8-field, legacy 7-field defaults 0.

### 1.4 Crafting = Anvil refine only (`tryAnvil`, `world.cpp:2248+`)

- Tiers `kAuraTiers` (`auras.h:24-30`): T1 30x4001+120g to T5
  3x4005+8000g; reqSkill gates; first-attempt mercy bit; fail
  -1 safe / 0 soft / 1 destroy (erase slot, no tombstone) +
  karma -6 destroy / +2 success (`world.h:336-338`).
- No recipes, materials table, recipe book, Create-Item,
  crystallize, grades, scrolls, Bound Souls. Junk/ore are
  currency (parts + 1:5 ore fallback). Lite law deferred, NOT
  built: T-159b (10xjunk->shard, 8 fixed recipes, spoil-flag).

### 1.5 Class / kit / progression today

- Flat enum `kKitUnsworn/Ravager/Gravecaller/Cultist`
  (`kits.h:11-13`). `KitDef` = 5-stat seed (sums to legacy 24;
  Ravager exact 8/8/8/0/0) + 12 channel unlocks. ch10 Resurrect
  = REBATE not corpse-raise (Cultist 20, <=6000t window, 25 MP,
  6-tile, 6000t CD). ch11/12 + control sets -> T-161b (open).
- Five stats STR/VIT/DEX/INT/MAG (T-160/ADR-0015). Creation
  T-167: Hello -> CharCreatePrompt 120 -> CharCreate 25
  (class 1..3, sex 1..2) -> spawn; 1-char-alpha; sex 0 legacy
  fallback. No races, no change tree/quests, no hate table
  (aggro radius + leash + guard/wanted only), no clan-gated
  targeting.
- Roster 25 rows (`mobs.h:42-91`): L1 Rat to L15 Guard wall
  (xp 0), 3 named elites, boss 1009, wave-2 rows incl. 2
  night-only (1022/1023, `nightOnly` spawners in 6 mapgens) +
  2 patrols with `town`.

### 1.6 Gold/XP literals by level (day base, `mobs.h:42-91`)

- L1 Rat 40xp 6-14g / L2 Bat 55xp / L3 Ghoul 90xp 14-30g +
  2001@4% / L5 Hound 150xp / L6 Spider 200xp / L7 Gnoll 300xp
  (+Maw 1012 1200xp, 3 uniques @4%) / L8 Wretch 340xp (+50%
  night) / L9 Widow 420xp (+Red Widow 1680xp, 3 @4%), Golem
  480xp / L10 Cultist 520xp, patrols 500-560xp / L11 Celebrant
  560xp / L12 Sexton 820xp, elites 3200-4100xp, Cantor 3 @4% /
  L14 Gravemother 4000xp 400-650g, 3 @6% / L15 Guard 0xp wall.

### 1.7 Karma / death / PK (what L1-drops displace)

- Bands `karmaBandOf` (`world.cpp:3670`): <0 chaotic red,
  >500 lawful (+15% XP), else neutral. Chaotic +15% gold at
  roll. Unlawful PK: -(300+20xdeficit) + wanted; guard 8-tile
  adds 240s wanted. War kills exempt.
- Death: chaotic -> equipped 15%/slot + 1-6 bag picks
  (`world.cpp:3891+`); all -> -5 durability, XP debt 10->25%
  bar L1->L25 with de-level. Lawful PvE death drops nothing.

### 1.8 Pledge / siege / town-war / bounty today

- Pledge-lite persisted (name/emblem/liege/vault_gold;
  ops create/ranks/chat/vault-tax-only). Wire SiegeState 117 /
  PledgeRoster 118 / PledgeMember 119 + HUD (T-151, epoch 28).
  Castle tax 5% -> holder/vault. Siege weekly gates->Heartstone
  ->crown; M4 FAIL-open (0 flips m4e29/b/f3a, `done/T-157.md`).
- Town-war: L19 one-way oath + patrol EK + /ek (T-163). No
  second-city sim. Bounty: single-mark persisted, one-kill
  drain, pickup-N cut (T-166, v15 bounty_mob/cycle).

## 2. Gap matrix — reference vs tree

Ref sections = Part B chapters (B1 names, B2 loot, B3 craft,
B4 classes, B5 oath). Gap: none / partial / absent. GDD cites
are BIBLE v2 (`docs/02-gdd.md`).

### B2 loot (ref B2.1-B2.4)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD conflict? |
|---|---|---|---|---|
| Drop groups + weights per mob | per-mob JSON, group_chance + weighted pick, multi-roll | one junk roll + one 4-row gear lookup + per-row uniques (§1.2) | absent | no — GDD §7 wants ~40 affixes/~12 uniques, silent on groups |
| Currency scaled base=level^2*k | currency min/max/chance per mob | literal goldLo/Hi per row (§1.6) | partial | no |
| Level-gap penalty table | 0-8:1.0 down to 14+:0 | none (L6 wall was waypoint/density, not gap) | absent | no |
| Ownership timers (15s / 300s raid) | world items owned by killer/party | no world items; direct inv push | absent | no |
| Party loot modes (5 incl. spoil) | distribute() strategy | XP shares, loot stays killer (§1.2.1) | absent | partial — GDD §4 party = XP bonus curve, silent on loot |
| Spoil / sweep | Boneraker marks pre-kill, Sweep collects | T-159b spoil-flag sketched only, not built | absent | no — T-159b owns the lite slice |
| Death-drop PvE/PK | karma-threshold 1-N drops | chaotic 15%/slot + 1-6 bag (§1.7); lawful PvE none | partial | no — matches GDD §5 red-loses-item fantasy |
| Herbs / bind-on-drop buffs | optional phase 3 | none | absent | OUT per §12 (no pets-as-loot-adjacent bloat) |

### B3 craft / enchant (ref B3.1-B3.7)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD conflict? |
|---|---|---|---|---|
| Material tiers T0-T4 + frames | 5-tier taxonomy, bible metals | junk/ore as refine currency only | absent | partial — GDD §7 Anvil + ore mining only; T-159b caps lite |
| Recipes + book slots | dwarven/common, success %, MP cost | none (T-159b: 8 fixed, no fail) | absent | YES if % fail — GDD §7 refine table is the gamble spine |
| Create Item levels 1-10 | Forgehand 5+, grade unlocks | none | absent | YES — needs Deepkin class (no race law, GDD §3) |
| Common Craft | non-dwarf to Iron, quest-gated | none | absent | same as above |
| Crystallize grades | break gear -> shards x(1+0.1xench) | T-159b Crystalize sketched (10xjunk->shard) | absent | no — T-159b owns it |
| Enchant safe +3, 66%, fail->crystals | Blood Sigils normal/blessed/cursed | Anvil mercy + fail -1/0/1 (§1.4) | partial (rival law) | YES — displaces shipped refine economy (ADR-0013) |
| Bound Souls / SA | stage 1-13 absorb + attach B+ | none | absent | OUT — v0.2 at earliest (GDD §10/12) |
| Private craft shops | Manufacture mode + fee | none (stalls OUT, GDD §12) | absent | YES — explicitly NOT building |

### B4 classes (ref B4.1-B4.8)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD conflict? |
|---|---|---|---|---|
| Race -> base -> 1st(20) -> 2nd(40) tree | 5 races, JSON templates | flat 3 kits + Unsworn (§1.5) | absent | YES — GDD §3 = 3 classes, cap 25, no races |
| Deepkin/Boneraker spoil+sweep | Spoil formula, Festival/Spoil Crush | none | absent | YES — needs race + T-159b consumer |
| Forgehand/Warsmith + golems | Create Item, siege/mechanic golems | none (Raise Skeleton is T-161b pet) | absent | YES — same race gate |
| Royal/Oathsworn founder | only-Royal founds, weak 1v1 + auras | any L10+10k founds (T-138); no Royal | absent | YES — 1-char-alpha (T-167) + no-faction MVP (GDD §1/5/10) |
| Grove/Hollow Elders (EE/SE split) | sustain vs offense buffers | one Cultist support spine (ch2/3/6/7/8/9/10) | partial | partial — single-support is the Friday-Night fantasy (§6.1) |
| Oathcaller clan-chants / Warhowler party | pledge==target filter | party frames + Chorus party-wide (T-054b) | partial | partial — filter is buildable, Royal gate is not |
| Pure tanks + hate table | add_aggro, Aggression pulls 100% | no hate; aggro radius + leash | absent | no — but M3 must justify it |
| Class-change quests + Borrowed Blade | level/items/NPC chain + coupon | kit via creation (/kit window fallback) | absent | YES — scope (quests-as-content OUT, GDD §12) |

### B5 oath / pledge (ref B5)

| L1/L2 mechanic | Part B proposal | BloodHollow today | Gap | GDD conflict? |
|---|---|---|---|---|
| Royal-gated founding + oath_seal | lvl10 Royal + shrine item | any sworn L10+10k (T-138) | absent | YES (same Royal gate) |
| Oath levels 0-8 + costs | SP/marks/members | none (flat pledge) | absent | OUT pre-alpha (GDD §10 pledge wars v0.2) |
| Oath skills / warehouse / halls | rep-bought buffs, rank perms, bidding | vault-tax-only, no perms/levels | partial | partial — warehouse-perms is the sane slice |
| Oath wars + Blood Marks | declare -> karmaless PvP | field-war EK vs PATROLS only (T-163) | partial | no — oath-vs-oath is the natural extend |
| Academy / alliance | apprentices, N-oath chants | none | absent | OUT (GDD §10) |
| Sieges 3-5 strongholds + golems | registration + tax + chamber | 1 castle, weekly 90-min, tax shipped | partial | no — 1 castle IS the MVP (05-mvp §1) |

## 3. What changes per proposal (files + version price)

Conventions: E=epoch bump (fresh leg + guard), W=wire bump
(base for fields / count for messages), S=schema migration
(additive + pin). Suite today 325/325 headless.

### L1. Loot groups v1 (the only loot slice worth pricing)

- ADD `DropGroupDef { groupChancePct; entries[] {itemId, weight, min, max} }` + per-mob table (start: 1-3 groups on farmed rows only — Ghoul/Gnoll/Widow/Sexton). Roll in `killMob` AFTER junk roll, deterministic order, `rng_.range` weights.
- TOUCH: `shared/content/mobs.h` (structs + table), `server/src/world.cpp` (roll), `server/src/main.cpp` (epoch), `tests/test_*loot*` (distribution pins: reuse T-159 100k fixed-seed pattern), `tools/wave2_leg.sh` or new leg, `docs/02-gdd.md` §7.
- PRICE: **E+1 + fresh leg** (new RNG draws). No W (no wire), no S (no persist). Keep drops direct-to-inv — world items are a separate, larger card.
- PRO: real per-band tables (T-159 deviation 2 closed), tunable without code shape change, deterministic-testable. CON: every tuning pass re-rolls the stream (epoch discipline); does not fix "who gets it" (still killer-takes-all).

### L2. Ownership + party modes (do NOT bundle with L1)

- ADD world-item entity (owner id + expiry tick), `EntitySpawn/Delta` fields or new `WorldDrop` messages, `distribute()` per party mode, pickup rules, 15s/300s timers on tick.
- PRICE: **E+1, W+1 (base), S+0** (transient, not persisted — or S if raid rights persist), client pickup render, bot support. Largest loot card.
- PRO: solves kill-steal/KS rage, enables raid contention. CON: new entity kind in AoI/sim/persist/replay; needs client + bot + HUD work; M1/M4 re-proof. Post-MVP.

### L3. Level-gap penalty (cheap, but needs a reason)

- ADD `level_gap_multiplier(killerLv, mobLv)` + table (ref default 0-8:1.0 to 14+:0) applied to XP/gold/groups in `killMob`.
- PRICE: **E+1** (scales awards). No W/S.
- PRO: kills grey-farming, 20 lines. CON: invalidates M2 TTK/XP-hr evidence (re-run chain); GDD silent — director must want it. Defer until economy evidence demands.

### C1. T-159b lite (the MVP ceiling — no new slice)

- As specced: Bonesmith Crystalize verb + 8 fixed recipes + spoil-flag +1 mat. Reuses corpse path (T-126 Embers precedent), `recipes.h` constexpr, gold fee -> tax sink.
- PRICE: small E if spoil changes killMob stream (coordinate with L-waves); W+0 if verbs reuse AnvilOp channels; S+0 (shards = junk ids).
- PRO/CON: already decided; see `T-159b-craft-lite-v0.2.md`. Do not expand without ADR + `05-mvp.md:37` cut.

### C2. Full craft/enchant/grades/souls (re-price only on retention)

- ADD material taxonomy + recipe book persist (S), Create-Item skill channels (W if client casts new ids), grade + crystal_count on ItemDef, enchant path rival to Anvil (ADR-0013 amend), Bound-Soul stage machine + raid-absorb skill, private-shop stationary mode + fee escrow.
- PRICE: **E+N, W+1..2, S+1..2**, new NPC verbs, new UI, new tests per subsystem, economy Monte Carlo re-run. Effectively a second progression spine.
- PRO: L2-faithful depth, crafter identity, sink richness. CON: displaces shipped Anvil economy + T-159b; needs Deepkin race (see K-class); JSON/config migration first (see §5); 2-3 sessions minimum; fights horror law (dwarven-industrial vs mud/rust/dread, T-159b rationale).

### K1. Class tree + races (reject for MVP, keep direction)

- ADD race enum + base->change tables + change quests/NPC chains + starting-area content + per-branch skill tables + stat growth + equip mastery + role flags; migrate `KitDef`/creation/persist/journal (`class_id` meaning changes).
- PRICE: **E+1, W+1 (creation fields), S+1 (race/change cols)**, M2/M3 re-proof, bot profiles per branch, art sheets per race/class (the R2 schedule bomb: ~80 frames per common mob already governs; player sheets x races multiply it).
- PRO: Lineage-faithful identity, tank/healer/nuker meta, content depth. CON: rewrites progression + balance evidence + art budget; cap 25 -> 40/76 breaks GDD §3; change quests violate quests-OUT (§12). v0.2 with Vampire race (GDD §10) is the natural host.

### K2. Hate-weight tank lever (only MVP-adjacent slice)

- ADD `hate` map on mob + `add_aggro` on damage/heal + Aggression multiplier skill; targeting honors hate within leash.
- PRICE: **E+1** (targeting changes sim). No W/S (server-side).
- PRO: enables real tanking, small. CON: needs M3 re-measure to justify; T-161b kit spine first (fuller kit may fix party meta without hate).

### P1. Oath-war declare + Blood Marks (the sane pledge extend)

- ADD `declareWar(oathA, oathB)` + karmaless-PvP check in PK path + Blood-Mark coin on war kills + `/ek`-style readout. No Royal gate, no levels/skills/halls.
- PRICE: **E+1** (PK law), W+0 (reuse chat/panel), S+1 if marks persist (or purse-only).
- PRO: pledge-vs-pledge stakes without faction sim; feeds Friday-Night story (§6.2 red-loses-item). CON: needs abuse pins (mutual-declare only? cooldown?); M4-adjacent risk (war during siege?).

### P2. Warehouse-perms + oath skills/levels/halls/academy (defer)

- PRICE each: S+1, W+0..1, E+0..1 + UI + persist + permission tests. Real guild-hosting work. Post-MVP (GDD §10).

## 4. Pros / cons (stacked, director-readable)

### Adopt loot groups v1 (L1)

- PRO: closes the last T-159 deviation with data not code-shape; designers tune rows without touching roll logic; deterministic pins already proven (100k fixed-seed pattern); no wire/schema/client cost.
- CON: another epoch + leg on the busiest stream in the sim; tuning churn = epoch churn (batch rows, don't dribble); still killer-takes-all (KS rage untouched — that is L2's price).
- VERDICT: post-MVP wave opener (batch with K2/P1 under ONE epoch if possible).

### Adopt ownership + party modes (L2)

- PRO: the only fix for KS contention + raid rights; L2-faithful social layer.
- CON: biggest loot card by far (entity + wire + client + bots + HUD + M1/M4 re-proof); transient-state replay surface; pickup UX needs art/HUD passes (T-ART-15 queue).
- VERDICT: post-MVP, after L1 proves the tables.

### Adopt level-gap penalty (L3)

- PRO: 20 lines, kills grey-farming forever.
- CON: burns M2 evidence (TTK/XP-hr chain re-run); GDD never asked; punishes help-a-friend power-leveling (genre-social cost).
- VERDICT: defer to economy evidence.

### Adopt full craft/enchant/grades/souls/shops (C2)

- PRO: crafter identity, deep sinks, L2-faithful chase.
- CON: second economy + second spine; displaces ADR-0013 refine + T-159b; needs Deepkin race + JSON migration + 2-3 sessions; horror-law fight; stalls explicitly OUT.
- VERDICT: reject for MVP; re-price on retention (D1/D7) evidence only.

### Adopt class tree + races (K1)

- PRO: the Lineage fantasy proper; tank/healer/nuker meta; content depth for years.
- CON: rewrites kits/creation/persist/journal/balance/art; cap-25 law breaks; quests-OUT breaks; R2 art bomb multiplies.
- VERDICT: reject for MVP; v0.2 host is Vampire race (GDD §10), not a 5-race big bang.

### Adopt Royal-gated oath + levels/skills/halls (P2-shape)

- PRO: L1-faithful leadership fantasy; Oathcaller chants get their lore filter.
- CON: Royal gate conflicts 1-char-alpha + no-faction MVP; levels/skills/halls are guild-hosting work, not alpha work; pledge-lite just shipped and is tested.
- VERDICT: reject gate + levels; keep P1 (oath-war) + warehouse-perms as the only sketches.

## 5. Architecture divergence (the reference's wrong assumptions)

1. `<items/loot/craft/class/clan file>` placeholders: real paths are `shared/content/items.h` / `mobs.h` (+`kGearDrops`/`kUniqueDrops`) / `auras.h` + `world.cpp:tryAnvil` / `kits.h` + `world.cpp` skill channels / pledge blocks in `world.cpp` + `persist.cpp` + `towns.h`. No JSON to point at.
2. "Data is (or can be) JSON/YAML/TOML": it is `constexpr` C++ by law (shared by 4 consumers). Externalizing = building a loader + validator + hot-reload + keeping determinism — a card before any B-phase.
3. "Classes are a flat enum": true (`kits.h:11-13`) but the enum rides wire + DB + journal (`class_id` + `kKitChoose.a`), so a tree migrates all three, not one file.
4. "Clan exists minimally or not at all": pledge-lite is SHIPPED + persisted + wired (T-138..T-140/T-151/T-163/T-166). B5 is extend, not greenfield.
5. Feature flags (`features.loot_v2`) + `config/economy.toml`: neither exists. Nearest precedent is content-only widening without wire/DB (T-161 `chUnlock` 10->12). Flags would need their own ADR + journal interaction law (flag state in replay?).
6. `lore_names.json`: no string-table pipeline; names are `const char*` in tables, clients print them. Neutral-ID + UI-text split is new infra.
7. RNG: reference pseudo uses `rand()`/`randint`; tree law forbids it (`sim/rng.h` only). Every weighted pick must be `rng_.range`-ordered and pinned, or replay breaks.
8. Spoil timing (pre-kill mark + post-death sweep bag) needs corpse law: today corpses despawn via `killMob` + 3 s respawn (the reason Resurrect became a rebate, `done/T-WAVE2.md` deviation 1). Corpse Explosion (T-161b item 4) owns the corpse-entity question — spoil rides it or waits.

## 6. Phased sketch (re-mapped B6 onto reality)

| Order | Phase (survives?) | Epoch/Wire/Schema | Notes |
|---|---|---|---|
| 0 | JSON externalization (REJECT as stated) | — | keep constexpr; grow structs instead. If JSON ever wanted, own ADR + loader card first |
| 1 | Loot groups v1 (L1) | E+1 | wave opener post-MVP; batch K2/P1 under the same E if ready |
| 2 | T-159b lite-craft (ALREADY SPECCED) | E+0..1, W+0, S+0 | no new slice; spoil-flag rides corpse law (T-161b.4) |
| 3 | Oath-war + Blood Marks (P1) | E+1, S+0..1 | mutual-declare + cooldown pins; siege-interaction ruled |
| 4 | Hate-weight tank lever (K2, CONDITIONAL) | E+1 | only if M3 re-measure (fuller kit) still shows solo meta |
| 5 | Ownership + party modes (L2) | E+1, W+1 | after L1 tables prove; needs client/bot/HUD passes |
| 6 | Warehouse-perms | S+1 | first guild-hosting slice post-alpha |
| 7 | Full craft / tree / souls / shops / academy | E+N, W+N, S+N | retention-gated (D1/D7); v0.2+; each own ADR + `05-mvp.md:37` cut |

### Card sketches (director decides — NOT filed)

- **LOOT-GROUPS-V1:** per-mob 1-3 weighted groups on 4 farmed rows, deterministic order, 100k-seed distribution pins, epoch + leg. Files: `mobs.h`, `world.cpp`, `main.cpp`, loot test, leg, GDD §7. E+1.
- **OATH-WAR:** mutual declare, karmaless-vs-target-oath check in PK path, Blood-Mark purse coin + readout, abuse pins (cooldown, siege blackout). Files: `world.cpp/h`, karma/PK tests, GDD §5/§8. E+1, S+0..1.
- **HATE-LEVER (conditional):** hate map + add_aggro + Aggression skill, targeting honors hate in leash, M3 re-measure attached. E+1.
- **OWNERSHIP-V1:** world-item entity + owner/expiry + pickup + 15s timer (raid 300s later), one party mode first (finders-keepers), client marker. E+1, W+1.

## 7. Risks + open questions for the director

1. Epoch economy: L1+K2+P1 each want E+1 — can they share ONE post-MVP wave bump (single leg, single guard) or do they land apart?
2. Reference's own known-exploits (multi-client spoil funnel, shop-fee evasion, enchant-insurance via blessed, Royal-alt oath spam): which transfer to us if we adopt L1/P1/C2?
3. Grey-farming: is L3's anti-social cost (no PL help) acceptable for a friends-invite alpha?
4. Royal fantasy vs 1-char-alpha: does leadership identity need a class, or do oath-war + banner-aura items cover it?
5. Anvil vs Sigils: is there room for TWO gamble spines, or does enchant talk end permanently with ADR-0013?
6. Corpse law: does T-161b.4 (Corpse Explosion) create the entity spoil needs, or stay spell-only?
7. `[unverified]` from tree: live 200-kill all-tiers drop log (T-159f1.5, needs instrumentation); 30-bot argon2id login wave p99 (T-153 deferred to soak); graphical client compile on wave-2 (CI matrix flagged); M4 flip cause beyond F3 (b-vs-c call).






