# Lineage 1 / Lineage 2 loot, crafting, class & clan — reference for comparison

*Pasted by the director 2026-09-16 (external author, no repo access). Part A is
a research prompt describing L1/L2 mechanics; Part B is a BloodHollow design
proposal written against `<placeholder>` assumptions about our code. Treat as
the comparison baseline for
`docs/prompts/systems-audit-race-class-loot-vs-lineage.md` — not as law.*

---

# PART A — Research Prompt

> ## Role
> You are a systems designer + reverse-engineer documenting the **loot, crafting, enchanting, class, and clan systems of Lineage 1 (Lineage: The Blood Pledge) and Lineage 2 (Prelude → Interlude, with notes on later chronicles)** for the purpose of re-implementing a faithful-but-adapted version in an indie dark-fantasy game. Precision matters more than breadth: prefer exact numbers, formulas, tables, and state machines over prose.
>
> ## Sources to prioritize (cite everything)
> - **Open-source emulators** (treat as primary technical sources): L1J / L1J-TW / L1J-JP (Lineage 1), L2J / L2JServer / L2J-Mobius / aCis / L2JFrozen (Lineage 2). Pull actual data from `droplist`, `spoil`, `recipes.xml`, `skills/*.xml`, `enchant` config, `player class templates`, `clan` config, `castle` config.
> - Official/semi-official docs: Lineage 2 Interlude/C4 official "Game Guide", PMfun / L2Wiki (Classic + Chronicle), Lineage 1 official (lineage.plaync) class/item pages, LinDB, LinCraft-era fan DBs.
> - Community mechanic write-ups (enchant rate tests, drop formula analyses, party-loot behavior), flagged as secondary.
>
> ## Deliverables (in this order)
>
> ### 1. Lineage 1 (Blood Pledge)
> 1.1 **Loot**: how drop tables are structured (per-monster item list, chance, min/max count), adena/gold drops, boss vs. normal drops, drop-on-death rules for players (PK/karma, lawful points, which slots, chance), item weight and inventory limits, "class-locked" items (Royal/Knight/Elf/Wizard/Dark Elf), quest-gated items.
> 1.2 **Crafting/creation**: confirm that L1 has no player craft skill; document all NPC-based item creation/exchange (material-hunting NPCs, Elven quest items, Dwarf NPC roles as warehouse keepers, polymorph scroll economy, elixirs). Document any later-added crafting (Evolution/Episode era) as an appendix.
> 1.3 **Enchanting**: safe enchant caps (weapon vs. armor), success behavior, failure behavior (item destruction), Blessed/Cursed scrolls, stacking of enchant with damage/AC, and any level-gated enchant differences.
> 1.4 **Royal (Prince/Princess) class and the Blood Pledge**: creation requirements, pledge ranks, pledge warehouse, castle sieges (which castles, siege windows, tax), Royal-only skills (Call Clan, Run Clan, Brave Aura, Shining/Glowing Aura, etc.) with numbers, and how the Royal's weaker personal combat was balanced by leadership utility.
> 1.5 Lineage 1 **class list** with role summary, primary stats, and signature skills.

> ### 2. Lineage 2 (focus C4/Interlude; note deltas for Classic and post-Gracia)
> 2.1 **Drops**: structure of drop groups (category/group chance → item chance), adena formula vs. monster level, herbs (Gracia+), champion/raid multipliers, level-gap drop penalty table, party loot modes (Finders Keepers, Random, Random incl. Spoil, By Turn, By Turn incl. Spoil) and their exact behavior, raid/grand boss loot rights (damage-based, timer, "looting rights" to party), item drop on death (PvP vs. PvE, karma), Manor (seeds/crops), Seven Signs seal stones→Ancient Adena, Luxury shop crystal trade.
> 2.2 **Spoil/Sweep**: Scavenger mechanics (Spoil chance vs. level difference, Sweep timing, spoil tables vs. drop tables, Festival Sweep), who can pick up spoil, party loot interaction.
> 2.3 **Crafting**: recipe types (Dwarven vs. Common), Create Item skill levels per class level, MP cost, success rates (100% vs. 60% recipes), material taxonomy (raw → intermediate → key parts → finished), full material tree examples for at least one D, C, B, A, S grade weapon and armor set, recipe acquisition (drop, shop, quest), Warsmith-only recipes, private craft shops, crafting fees and Manufacture window.
> 2.4 **Crystallization & grades**: grade list (NG/D/C/B/A/S/S80/S84), grade penalty rules, Crystallize skill levels, crystal yield per item and enchant level.
> 2.5 **Enchant**: safe enchant (+3, +4 for one-piece body armor), base success rates by chronicle, failure result (crystals), Blessed scroll behavior, Crystal scrolls, enchant bonus formulas (P.Atk/M.Atk per +1, extra past +3, armor P.Def), Blacksmith of Mammon unseal/enchant services.
> 2.6 **Soul Crystals / SA**: crystal stages, leveling via raid bosses, SA effects (Focus, Haste, Critical Damage, Health, Anger, etc.), how SA is attached (gemstones + blacksmith). Also summarize **Augmentation** (Life Stones) and **Attribute** systems as optional appendices.
> 2.7 **Dwarf race**: Scavenger→Bounty Hunter→Fortune Seeker, Artisan→Warsmith→Maestro; every signature skill with level/values (Spoil, Sweep, Crystallize, Create Item, Summon Siege Golem, Big Boom, Summon Mechanic Golem/Wild Hog Cannon, Hammer Crush, Stun Shot, Fake Death, etc.).
> 2.8 **Support/"battery" classes**: Elven Elder (EE) and Shillien Elder (SE) full buff/heal kits with values by level (Recharge, Bless the Body/Soul, Empower, Vampiric Rage, Wild Magic, Death Whisper, Clarity, Mana Regeneration, Resist Shock, Bless Shield, Cure Poison/Bleed, Resurrection, Battle Heal, etc.); Prophet for comparison; Warcryer chants (party) vs. **Overlord seals/blessings (clan-only)** with exact targeting rules and the Dominator 3rd-class evolution.
> 2.9 **Tanks**: Paladin, Dark Avenger, Temple Knight, Shillien Knight — aggro mechanics (Aggression, Hate Aura, aggro points formula), Shield mechanics (Shield Def, block rate, Shield Stun, Shield Fortress, Ultimate Defense), summons/cubics, damage-reflect.
> 2.10 **Clan system**: clan levels 0–8 (later 11), level-up costs (SP/adena/reputation/Blood Marks), clan skills, clan halls (auction), castle & fortress sieges, academy, alliances, clan wars, clan warehouse, clan reputation sources/sinks.
>
> ### 3. Comparison matrix
> Side-by-side table: L1 vs L2 for each subsystem (loot, drop-on-death, crafting, enchant, pledge/clan, siege, support classes, tanks). Note which L1 elements survive in L2 and which were replaced.
>
> ### 4. Extraction for re-implementation
> - JSON schemas you'd propose for: `DropTable`, `SpoilTable`, `Recipe`, `MaterialTree`, `EnchantRule`, `ClassTemplate`, `SkillDefinition`, `ClanLevel`.
> - Pseudo-code for: drop roll, spoil/sweep, party loot distribution, craft attempt, enchant attempt, aggro accumulation.
> - A "tunable knobs" table (every rate/cap that a designer should be able to change).
> - Known exploits/pain points of each system (e.g., bot-farming spoil, enchant scams, pledge alt abuse) and how later chronicles patched them.
>
> ## Format rules
> Tables > prose. Every number gets a source. Mark uncertain values `[unverified]`. Keep chronicle deltas in separate columns. Deliver as Markdown with an appendix of raw table excerpts from the emulator data.

---

# PART B — Patch / Rewrite Proposal for BloodHollow

## B0. Assumptions & what I need from the repo

| Assumption | If wrong, adjust |
|---|---|
| Repo has some flat "kill → drop item" loot and a simple "combine X+Y" craft | The proposal replaces both with data-driven tables; if you already have tables, keep the schema names and only add the new fields |
| Data is (or can be) JSON/YAML/TOML with code loading it | If data is hard-coded, step 1 of the migration is externalizing it |
| Classes are a flat enum or list | Replace with a race→base→1st→2nd (→3rd) tree |
| Clan/guild exists minimally or not at all | Pledge system below is self-contained |

**Please paste (or point me to):** `<items file>`, `<loot/drop code>`, `<craft code>`, `<class definitions>`, `<skill definitions>`, `<clan/guild code>`, and the lore bible sections on **races, factions, metals/materials, magic sources, and oaths/blood**. Then I'll convert Part B into actual diffs.

---

## B1. Lore-fit naming layer (swap with bible canon)

Everything below uses these working names. Keep a single `lore_names.json` mapping so the code uses neutral IDs and the UI pulls canon text.

| Lineage concept | Working BloodHollow name | ID | Lore hook to write into the bible |
|---|---|---|---|
| Blood Pledge (L1) | **Blood Oath** | `pledge` | An Oath is sworn in blood at a Hollow shrine; the leader's blood binds the members (justifies clan-only buffs) |
| Prince/Princess (L1 Royal) | **Oathsworn / Hollow Lord (♂/♀ Hollow Lady)** | `royal` | Born of a bloodline old enough to bind others; weak in arms, strong in command |
| Dwarf (L2) | **Deepkin** (Stonewright / Scavenger branches) | `race_deepkin` | Sole race that understands "cold metal" and can read the veins of the Hollow |
| Scavenger → Bounty Hunter | **Boneraker → Grave-Reaver** | `scav`, `bounty` | Strip corpses of what others can't see |
| Artisan → Warsmith | **Forgehand → Warsmith** | `artisan`, `warsmith` | Only ones who can shape Hollowsteel |
| Elven Elder (EE) | **Grove Elder** | `ee` | Life-magic of the old wood; mana as sap |
| Shillien Elder (SE) | **Hollow Elder** | `se` | Death/blood-magic; mana as drawn blood |
| Overlord (Orc, clan-only) | **Oathcaller** (2nd) → **Dominator**-equiv **Oathbreaker's Bane / Warlord of the Oath** | `overlord` | Chants only reach those who share the leader's blood-bond → clan-only targeting is literally lore |
| Warcryer (party chants) | **Warhowler** | `warcryer` | Party-scope chants |
| Paladin / Dark Avenger / Temple Knight / Shillien Knight | **Bulwark (holy), Grave Warden (dark), Grove Warden, Hollow Warden** | `tank_*` | Pure tanks; each faction has one |
| Adena | **Hollowmarks** (or bible's coin) | `currency_main` | |
| Ancient Adena | **Old Blood Coin** | `currency_ancient` | Boss/event currency |
| Crystals (D/C/B/A/S) | **Bloodshards** by grade | `crystal_<grade>` | Items "bleed" into shards when broken |
| Soul Crystal / SA | **Bound Soul** | `soul_crystal` | Fed on raid-boss deaths |
| Enchant scrolls | **Blood Sigils** (Weapon/Armor) | `scroll_enchant_*` | |
| Blessed scrolls | **Consecrated Sigils** | `scroll_enchant_*_blessed` | |
| Grades NG/D/C/B/A/S | **Ash / Bone / Iron / Hollowsteel / Veinsilver / Heartblood** | `grade_0..5` | Tie each grade to a material named in the bible |

---

## B2. Loot system rewrite (L2-style)

### B2.1 Data schema

```jsonc
// data/loot/<monster_id>.json
{
  "monster_id": "hollow_ghoul",
  "level": 24,
  "currency": { "min": 120, "max": 240, "chance": 0.70 },
  "drop_groups": [
    {
      "group_chance": 0.15,
      "items": [
        { "item": "iron_dagger", "weight": 40, "min": 1, "max": 1 },
        { "item": "recipe_iron_dagger", "weight": 15, "min": 1, "max": 1 },
        { "item": "iron_dagger_blade", "weight": 45, "min": 1, "max": 2 }
      ]
    },
    {
      "group_chance": 0.60,
      "items": [
        { "item": "animal_bone", "weight": 50, "min": 1, "max": 3 },
        { "item": "coal", "weight": 50, "min": 1, "max": 2 }
      ]
    }
  ],
  "spoil": [
    { "item": "iron_ore", "chance": 0.55, "min": 1, "max": 3 },
    { "item": "suede", "chance": 0.30, "min": 1, "max": 2 }
  ],
  "flags": { "raid": false, "champion_eligible": true }
}
```

### B2.2 Roll algorithm (pseudo)

```python
def roll_loot(monster, killer_party, config):
    ctx = LootContext(monster, killer_party)
    lvl_mult = level_gap_multiplier(monster.level, killer_party.avg_level, config.level_gap_table)
    rate = config.rate_drop * lvl_mult * (config.champion_mult if monster.is_champion else 1)

    out = []
    if rand() < monster.currency.chance * min(1, rate):
        amt = randint(monster.currency.min, monster.currency.max) * config.rate_currency * lvl_mult
        out.append(Stack(CURRENCY, int(amt)))

    for g in monster.drop_groups:
        chance = g.group_chance * rate
        rolls = int(chance) + (1 if rand() < chance % 1 else 0)
        for _ in range(rolls):
            entry = weighted_pick(g.items)
            out.append(Stack(entry.item, randint(entry.min, entry.max)))

    if monster.spoiled_by:
        monster.spoil_bag = [Stack(e.item, randint(e.min,e.max)) for e in monster.spoil if rand() < e.chance * config.rate_spoil]

    return distribute(out, killer_party, monster)
```

`level_gap_table` (L2-faithful default): gap ≤ 8 → 1.0; 9 → 0.5? No — use: 0–8: 1.0, 9: 0.83, 10: 0.66, 11: 0.5, 12: 0.33, 13: 0.16, 14+: 0.0 (tunable).

### B2.3 Distribution / ownership rules

| Rule | Implementation |
|---|---|
| Loot rights | On monster death, drops spawn as world items owned by the killer (or party) for `T_owner = 15s`, then free-for-all |
| Raid loot rights | Track `damage_by_party`; party with highest damage gets `T_owner = 300s` |
| Party loot modes | `finders_keepers`, `random`, `random_incl_spoil`, `by_turn`, `by_turn_incl_spoil` — implement as strategy pattern in `distribute()` |
| Spoil bag | Only pickable by the Boneraker who spoiled it via `Sweep`, within `T_sweep = 20s` after death (party modes with `_incl_spoil` reroute to party) |
| Death drop (PvE) | L1-style: on death, `chance_pve_drop` (default 0 for lawful, rising with karma); L2-style default: 0 for lawful, karma players roll each equipped/inventory slot |
| Death drop (PK) | Karma (blood-taint in lore: **"Stained"**) ≥ threshold → drop 1–N random items; bosses immune |
| Herbs (optional, phase 3) | Separate `herb_groups`, bind on drop, 15s life, instant-use buffs |

### B2.4 Currency & sinks (L2 economy pillars)

- **Hollowmarks** drop scaled: `base = level² * k` (L2 uses roughly linear-quadratic scaling; tune `k`).
- Sinks: shop buy, teleport, craft fees (0 by default like L2, or small fee), enchant scrolls in shops (D/C only), clan level-ups, siege registration.
- **Old Blood Coin**: from boss "vein stones" (Seven-Signs analogue) → traded at a **Blood Merchant** for otherwise unobtainable A-grade recipes/pieces.

---

## B3. Crafting rewrite (L2 Dwarven crafting)

### B3.1 Material taxonomy (5 tiers, map names to bible)

| Tier | Purpose | Examples (L2 → BloodHollow working name) |
|---|---|---|
| T0 Raw | drop/spoil everywhere | Animal Bone→**Bone**, Coal→**Coal**, Iron Ore→**Iron Ore**, Suede→**Hide**, Thread→**Sinew**, Varnish→**Pitch**, Charcoal, Stem→**Root** |
| T1 Refined | crafted by anyone with recipe (common) | Steel, Cokes→**Slag**, Silver Nugget→**Veinsilver Nugget**, Mithril Ore→**Hollowsteel Ore**, Oriharukon Ore→**Heartblood Ore** |
| T2 Compound | Deepkin-only, cheap | Varnish of Purity, Synthetic Cokes, Mold Glue→**Bone Glue**, Mold Hardener, Mold Lubricant→**Marrow Oil**, Braided Hemp |
| T3 Advanced | Deepkin-only, rarer drops | Enria→**Ash Salt**, Asofe→**Grave Salt**, Thons→**Blood Salt**, Durable Metal Plate, Mithril Alloy→**Hollowsteel Alloy**, Oriharukon→**Heartblood Ingot**, Crafted Leather, Metallic Fiber |
| T4 Key parts | item-specific, 60–100% recipe | "\<Item> Blade / Head / Edge / Fabric / Piece / Fragment" |
| Frames/Molds | skill-tier gates | Blacksmith's Frame→**Forgehand's Frame**, Artisan's Frame, Warsmith's Mold/Holder → **Warsmith's Mold / Holder** |

### B3.2 Recipe schema

```jsonc
// data/recipes/iron_sword.json
{
  "recipe_id": "rcp_iron_sword",
  "product": { "item": "iron_sword", "count": 1 },
  "grade": "iron",
  "type": "dwarven",
  "craft_level": 3,
  "success": 1.00,
  "mp_cost": 96,
  "materials": [
    { "item": "iron_sword_blade", "count": 12 },
    { "item": "steel",            "count": 30 },
    { "item": "crystal_iron",     "count": 24 },
    { "item": "gemstone_iron",    "count": 6 }
  ],
  "learn": { "consumes_recipe_item": "recipe_iron_sword", "book_slots": 1 },
  "lore_tag": "hollowsteel_forging"
}
```

Recipe book capacity: `dwarven_book = 50 + 2*craft_level` (Deepkin), `common_book = 50`.

### B3.3 Create Item skill (Forgehand/Warsmith)

| Class level | Create Item lvl | Unlocks grade |
|---|---|---|
| 5 | 1 | Ash (NG) |
| 20 | 2 | Bone (D) low |
| 28 | 3 | Bone (D) |
| 36 | 4 | Iron (C) low |
| 43 | 5 | Iron (C) |
| 49 | 6 | Hollowsteel (B) low |
| 55 | 7 | Hollowsteel (B) |
| 62 | 8 | Veinsilver (A) |
| 70 | 9 | Heartblood (S) |
| 76+ | 10 | Heartblood+ |

Any non-Deepkin class gets **Common Craft** (levels 1–5, common recipes only, max Iron grade, 60–80% success) unlocked at level 20 quest **"The First Forging"**.

### B3.4 Craft attempt (pseudo)

```python
def craft(player, recipe, count=1):
    if recipe.type == "dwarven" and not player.has_skill("create_item", recipe.craft_level): fail("skill")
    if recipe.type == "common"  and not player.has_skill("common_craft", recipe.craft_level): fail("skill")
    if recipe.recipe_id not in player.recipe_book: fail("unknown")
    for m in recipe.materials:
        if player.inv.count(m.item) < m.count*count: fail("materials")
    if player.mp < recipe.mp_cost: fail("mp")
    player.mp -= recipe.mp_cost
    player.inv.remove_all(recipe.materials, count)
    if rand() < recipe.success * (1 + player.stat.craft_luck):
        player.inv.add(recipe.product)
        emit("craft_success")
    else:
        emit("craft_fail")
```

Optional **private craft shop**: player enters "Manufacture" mode, lists recipes + fee; customers supply materials, crafter gains fee. Needs a stationary-mode flag in the player state machine.

### B3.5 Crystallize (Deepkin skill)

| Crystallize lvl | Class lvl | Can break grade |
|---|---|---|
| 1 | 20 | Bone (D) |
| 2 | 40 | Iron (C) |
| 3 | 52 | Hollowsteel (B) |
| 4 | 61 | Veinsilver (A) |
| 5 | 76 | Heartblood (S) |

`shards = item.crystal_count * (1 + 0.1 * enchant_level)` (tunable). Items carry `crystal_count` and `crystal_grade`.

### B3.6 Enchant (Blood Sigils)

```jsonc
"enchant": {
  "safe_weapon": 3, "safe_armor": 3, "safe_fullbody": 4,
  "success_weapon": 0.66, "success_armor": 0.66,
  "fail_normal": "destroy_to_crystals",
  "fail_blessed": "reset_to_zero",
  "fail_cursed": "decrement",
  "bonus": {
    "weapon_patk_per_level": {"bone":2,"iron":3,"hollowsteel":3,"veinsilver":4,"heartblood":5},
    "extra_after": 3, "extra_mult": 2,
    "armor_pdef_per_level": 1, "armor_extra_after": 3
  }
}
```

Enchant in L1 flavor for lore: sigils are inked in blood; failure "drinks" the item into shards (crystals) — consistent with Crystallize.

### B3.7 Bound Souls (Soul Crystal / SA)

- Stages 1–13 (`soul_crystal_<color>_<stage>`). Stage-up: party kills eligible raid; if the crystal holder "absorbs" (skill `absorb_soul`, cast during last 10% HP), roll `p_absorb`; fail may shatter (L2: stage 11+ may break).
- Attach at **Blacksmith** NPC: weapon (B+) + stage‑10+ crystal + gemstones → adds `sa_effect` (Focus/Haste/Health/Anger/Critical Damage/Empower/Acumen...).
- Lore: a Bound Soul is a boss's soul chained to steel; give each raid boss a named soul in the bible.

---

## B4. Class tree rewrite

### B4.1 Structure

```
race → base (lvl 1) → 1st change (lvl 20 quest) → 2nd change (lvl 40 quest) → [3rd change lvl 76, phase 4]
```

Data-driven: `data/classes/<class_id>.json` with `parent`, `race`, `change_level`, `base_stats`, `stat_growth`, `skills[]`, `equip_mastery`, `role`.

### B4.2 Proposed tree (working names; adapt races to bible)

**Humans (or bible's "Kindred")**
- Fighter → Warrior → *Gladiator / Warlord* | Knight → **Bulwark** (Paladin-type) / **Grave Warden** (Dark Avenger-type) | Rogue → *Treasure Hunter / Hawkeye*
- Mystic → Wizard → *Sorcerer / Necromancer / Warlock* | Cleric → *Bishop / Prophet*
- **Oathsworn / Hollow Lord (Royal, from L1)** — see B4.4

**Grove Elves (light)**
- Fighter → **Grove Warden** (Temple Knight) | Scout → *Plainswalker / Silver Ranger*
- Mystic → Wizard → *Spellsinger / Elemental Summoner* | Oracle → **Grove Elder (EE)**

**Hollow Elves (dark)**
- Fighter → **Hollow Warden** (Shillien Knight) | Assassin → *Abyss Walker / Phantom Ranger*
- Mystic → Wizard → *Spellhowler / Phantom Summoner* | Oracle → **Hollow Elder (SE)**

**Orc-equivalent (bible's brute race)**
- Fighter → Raider → *Destroyer* | Monk → *Tyrant*
- Mystic → Shaman → **Oathcaller (Overlord, clan-only)** / **Warhowler (Warcryer, party)**

**Deepkin (Dwarf)**
- Fighter → **Boneraker** (Scavenger) → *Grave-Reaver* (Bounty Hunter)
- Fighter → **Forgehand** (Artisan) → *Warsmith*

### B4.3 Deepkin kits

| Skill | Class | Level | Effect |
|---|---|---|---|
| Spoil | Boneraker | 20 | Marks monster; chance = `base 0.9 - 0.1*max(0, mob_lvl - player_lvl - 3)`; only on unspoiled targets |
| Sweep | Boneraker | 20 | Collect spoil bag from corpse (owner only) |
| Festival Sweep (2nd) | Grave-Reaver | 48 | AoE sweep |
| Spoil Crush (opt.) | Grave-Reaver | 55 | Spoil + damage |
| Crystallize | both | 20+ | see B3.5 |
| Create Item | Forgehand | 5+ | see B3.3 |
| Summon Siege Golem | Warsmith | 40 | siege-only summon vs. gates (needs Old Blood Coin / D-crystals) |
| Big Boom | Warsmith | 40 | summon self-destructing golem |
| Summon Mechanic Golem | Warsmith | 43 | combat pet fed with crystals |
| Hammer Crush / Stun Shot / Fake Death | both | var. | standard L2 |
| Deepkin passive | race | 1 | +inventory slots, +weight limit, +mining/pickup speed |

### B4.4 Oathsworn / Hollow Lord (L1 Royal → clan-leader class)

Design intent from L1: the Royal is the **only class that can found a Blood Oath**, is weaker 1v1, and pays it back with leadership utility. Fit into L2's tree as a **Human (or any race, bible-dependent) base class that has no 1st change branch choice** — it "changes" into **Oathsworn (20)** → **Hollow Lord/Lady (40)**.

| Skill | Lvl | L1 origin | Effect (tunable) |
|---|---|---|---|
| Found Oath | 10 | Pledge creation | Consumes `oath_seal` item at a Hollow shrine; creates pledge |
| Call Oath | 30 | Call Clan | Teleport a consenting oath member to you (long CD, siege-restricted) |
| Rally Oath | 35 | Run Clan | Oath members near you +speed |
| Brave Aura | 25 | Brave Aura | Oath members in radius: +atk speed / +move |
| Shining Aura | 30 | Shining Aura | Oath members: +AC (P.Def) |
| Glowing Aura | 45 | Glowing Aura | Oath members: +hit / +crit |
| Oath Banner | 40 | Emblem | Placeable banner: aura zone; destroyable in siege |
| Bloodline Command | 50 | new | Oath-wide short buff on castle capture |
| Passive: Frail Blood | 1 | L1 weaker stats | −5% HP/P.Atk vs. Fighter; +20% pledge XP contribution |

Restrictions: auras are **oath-only** (same mechanism the Oathcaller uses — see B4.6), can't be applied to non-members even in party.

If the bible says leadership must be earned rather than born: keep the class but gate "Found Oath" behind a quest rather than birth, and let a non-Royal leader exist with reduced auras.

### B4.5 Grove Elder (EE) / Hollow Elder (SE) — mana batteries & second healers

| Skill | EE | SE | Notes |
|---|---|---|---|
| Recharge (mana transfer) | ✔ | ✔ | Core "battery": transfer `X + 0.4*lvl` MP, costs caster MP |
| Heal / Greater Heal / Battle Heal / Group Heal | ✔ | ✔ | SE lower heal power |
| Resurrection | ✔ | ✔ | XP restore % scales with level |
| Cure Poison / Bleed / Purify | ✔ | ✔ | |
| Bless Shield, Advanced Block | ✔ | ✔ | tank support |
| Mana Regeneration, Clarity, Resist Shock, Wind Walk | ✔ | — | EE: sustain/utility |
| Empower, Wild Magic, Vampiric Rage, Death Whisper | — | ✔ | SE: offense buffs |
| Mass Bless Shield etc. (3rd) | later | later | |
| Lore | Life-sap of the grove | Drawn-blood mana | Both are Oracle→Elder at 40 |

### B4.6 Oathcaller (Overlord) — clan-only chants

Implement a **target filter**: `targets = allies_in_radius where a.pledge_id == caster.pledge_id` (plus alliance flag). Party membership is *not* enough.

| Skill | Lvl | Effect |
|---|---|---|
| Seal of Winter / Slow / Silence / Chaos / Gloom | 40–58 | Debuff AoE on enemies (pledge-independent) |
| Pa'agrio's Fist/Glory/Honor/Blessing → **Oathfire Fist / Glory / Honor / Blessing** | 40–56 | Oath-only buffs (+P.Atk, +max HP, +accuracy, +regen) |
| Heart of Pa'agrio → **Heart of the Oath** | 49 | Oath-only AoE heal + regen |
| Sight of Pa'agrio / Steal Divinity (opt.) | 3rd | |
| Passive: Oathbound Voice | 40 | Chants +50% radius when Oath leader (Hollow Lord) is within range — links the two L1/L2 pieces |

**Warhowler** (Warcryer) gets party-scoped chants (Chant of Battle/Fury/Evasion/Rage/Victory later).

### B4.7 Pure tanks

Shared tank framework:

```python
def add_aggro(mob, player, amount): mob.hate[player] += amount * player.hate_mult
```

| Skill | Bulwark (Paladin) | Grave Warden (DA) | Grove Warden (TK) | Hollow Warden (SK) |
|---|---|---|---|---|
| Aggression / Aura of Hate | ✔ | ✔ | ✔ | ✔ |
| Shield Stun, Shield Fortress, Ultimate Defense, Deflect Arrow | ✔ | ✔ | ✔ | ✔ |
| Signature | Holy Blade, Heal, Holy Armor, Sacrifice | Summon Dark Panther → **Hollow Hound**, Reflect Damage, Lifedrain | Summon cubics (Storm/Heal/Poison) → **Grove Wisps** | Summon cubics (Vampiric/Binding/Poison) → **Hollow Wisps**, Life Drain, Lifetime |
| Race lore | Kindred oath-keepers | Blood-stained warden | Grove | Hollow |

Add `role: "tank"` to templates so AI mob targeting can honor hate tables.

### B4.8 Class-change quests

Store as `data/quests/class_change/<from>_<to>.json`: level req, items to gather (use crafting materials — e.g., Forgehand quest requires crafting 10 Steel, Boneraker requires spoiling 3 mob types), NPC chain, reward (class id + a grade-D equipment coupon + shadow-weapon analogue "**Borrowed Blade**" with limited durability).

---

## B5. Blood Oath (pledge) system: L1 pledge + L2 clan merged

| Feature | Design |
|---|---|
| Founding | Only `royal` class lvl ≥10 with `oath_seal` (L1) — or configurable to allow any lvl 10 with cost (L2) |
| Ranks | Hollow Lord, **Blood Guard** (Guardian Knight), **Sworn**, **Probationary**; L2-style privileges bitmask (invite, warehouse, siege, halls) |
| Oath level 0–8 | Costs: SP/XP + Hollowmarks + **Blood Marks** (from oath wars/sieges) + member count thresholds; unlocks: crest, warehouse, alliance, oath skills, academy |
| Oath skills | L2 clan skills (+HP, +P.Def, etc.) bought with reputation |
| Oath warehouse | shared inventory with rank permissions |
| Sieges | Castles from the bible (e.g., 3–5 strongholds); registration window; gates/artifact; Warsmith siege golems; capturing enables tax on town shops + castle chamber |
| Oath halls | bidding with Hollowmarks (L2 clan halls) |
| Oath wars | declare → PK without karma vs. that oath; Blood Marks on kills |
| Academy | lvl 1–39 apprentices, reputation on graduation |
| Alliance | up to N oaths; Oathcaller chants extend to alliance if config `alliance_chants = true` |

---

## B6. Migration plan (phased)

| Phase | Scope | Touches |
|---|---|---|
| 0 | Externalize existing items/loot/craft/classes into JSON; add `lore_names.json` | `<items>`, `<classes>`, loader |
| 1 | Loot v2: drop groups, currency scaling, level-gap penalty, ownership timers, party loot modes | `<loot code>`, party system |
| 2 | Grades + Bloodshards + Crystallize + Enchant with Sigils | item model (`grade`, `crystal_count`, `enchant`), NPC blacksmith |
| 3 | Crafting v2: material tiers, recipes, Create Item, Common Craft, recipe books | `<craft code>`, inventory |
| 4 | Class tree + Deepkin + Elders + Oathcaller/Warhowler + tanks + hate system | `<class defs>`, `<skills>`, mob AI |
| 5 | Blood Oath system + Royal class + auras | `<clan code>`, new pledge module |
| 6 | Spoil/Sweep, Bound Souls (SA), private craft shops, sieges | loot, skills, world |
| 7 (opt.) | Herbs, augmentation, Old Blood Coin merchant, 3rd classes | |

Each phase ships behind a feature flag (`features.loot_v2`, `features.craft_v2`, …) so old and new can coexist while tuning.

---

## B7. Tunable knobs (put in `config/economy.toml`)

```toml
[rates]
drop = 1.0; spoil = 1.0; currency = 1.0; xp = 1.0; craft_luck = 0.0
[loot]
owner_timer_s = 15; raid_owner_timer_s = 300; sweep_timer_s = 20
level_gap = [1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,0.83,0.66,0.5,0.33,0.16,0.0]
pk_drop_threshold_karma = 240; pk_drop_items_max = 3
[craft]
dwarven_book_base = 50; common_book = 50; common_max_grade = "iron"
[enchant]
safe_weapon = 3; safe_armor = 3; safe_fullbody = 4; p_weapon = 0.66; p_armor = 0.66
[pledge]
royal_required_to_found = true; alliance_chants = false; max_members_by_level = [10,15,20,30,40,40,40,40,40]
```

---

## B8. Test checklist

- Drop roll distribution matches table within 2% over 100k sims per monster.
- Level-gap penalty applies to both currency and groups.
- Party modes: each mode simulated with 9-member party, verify spoil routing.
- Spoil can't be applied twice; Sweep fails for non-owner; timers expire.
- Craft: material removal atomic; failure consumes materials; MP check; book capacity.
- Crystallize yields per grade/enchant; enchant failure yields correct shards.
- Oathcaller chants never hit non-oath party members; Royal auras same.
- Hate table: tank Aggression pulls mob off healer 100% at equal level.
- Pledge: founding restricted by class flag when `royal_required_to_found=true`.

---

## B9. Lore-bible checklist (things to add/confirm in the bible so the systems feel native)

1. A **blood-binding ritual** explaining oath-only magic.
2. **Six named materials/grades** (Ash → Heartblood) and where in the world each is veined.
3. Why **Deepkin** alone can forge Hollowsteel and read corpses (Spoil).
4. Two elven traditions: **Grove** (life-sap mana) vs **Hollow** (drawn-blood mana).
5. A brute race whose shamans bind chants to bloodlines (Oathcaller) vs. to war-bands (Warhowler).
6. **Bound Souls**: named raid-boss souls and what each grants a weapon.
7. **Stained** (karma) condition and why the world "takes back" a Stained one's belongings on death (L1-style drop).
8. Castles/strongholds list for sieges and who held them before players.