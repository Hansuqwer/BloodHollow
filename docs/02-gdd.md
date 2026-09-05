# 02 — Game Design Document (BLOODHOLLOW)

Scope note: this document describes the **whole intended game**, with every feature
tagged **[MVP]**, **[v0.2]**, or **[LATER]**. The MVP cut is what `docs/05-mvp.md`
builds to. If a number appears here, it is the tuning starting point, not gospel —
all balance numbers live in data files (`shared/data/*.json`) so agents can tune
without touching code.

---

## 1. World & fiction

The drowned kingdom of **Vessalia**. A blood-plague ("the Hollowing") rose from the
marshes a generation ago; the king is gone, and two successor towns survive on
opposite banks of the Redwater:

- **Thornwall** (west) — dour fortress-town of the **Ashen Compact**.
- **Marrowgate** (east) — zealot port of the **Pale Synod**.

Characters start neutral in the refugee camp; at **level 19** you must swear to one
town to keep leveling (Helbreath rule). The two towns are at war: killing a sworn
member of the enemy town grants **EK (Enemy Kills)** fame instead of chaos — the
same hemisphere, same laws, different colors. Between them: **Weeping Castle**, held
by whichever bloodpledge took it last siege night.

Tone keywords: *mud, rust, candle-light, rot, incense, teeth.* No elves, no sparkle.
Horror comes from consequence (death, night, loss) more than jump-scares.

## 2. Camera, controls, feel

- **Isometric 2:1 diamond tiles** (64×32 px ground tiles; characters ~56 px tall).
  Base resolution 1024×768, integer-scaled to the window with letterboxing.
- **Click-to-move** (A*), click-enemy to swing; hold-click keeps attacking. WASD
  does *instant* direction-walk as a modern mercy toggle. [MVP]
- **Ctrl+click** forces attack on players (Lineage rule); normal clicks refuse
  friendly targets. [MVP]
- Swing/cast timings are **deliberate**: a swing is 3 animation frames but resolves
  on the server's tick; potions have a 500 ms global sip. Kiting and stutter-step
  are skills. [MVP]
- F1–F8 side-slots for potions/scrolls (Helbreath culture), hotbar 1–8 for skills.
- Zoom: 1×/1.5×/2× integer steps only.

### Art-direction spec (era-authentic)

| Element | Spec |
|---|---|
| Palette | Per-asset ≤32 colors; global dither pass; night = desaturated blue-black multiply + additive light mask. **Soma lesson: never pitch-black — night tint alpha capped at ~65% so the field stays playable (Myth of Soma servers literally shipped "permanent daytime" as a QoL hack; we bake the fix).** |
| Look target | **Helbreath-favored split lock** (director 2026-09-04): **terrain** = Myth of Soma painterly pre-rendered (rooted giant trees, swamp mud, broken aqueducts, taller zoom than Mir); **characters & VFX** = Helbreath hand-drawn sprites with visible paper-doll silhouettes + crowd-readable spell FX (a 15-player pile must still parse); **combat storytelling** = HB red-caps ability callouts over heads ("Power-Swing!", "Hell-Fire!") + party-name colors; **grade** = Lineage 1 dark-horror palette with DarkEden neon-gothic accents reserved for curse/magic light |
| Terrain dressing | Painterly scatter: root clusters, leaning gravestones, broken pillars/aqueduct arches over water, mud puddles; monsters lurk in shadow pools (Soma composition) |
| Sprites | 8 directions; walk 6f, attack 3f (era-authentic minimalism), cast 4f, hurt 2f, die 4f, gib 3f |
| Gore | Blood decals persist on the ground for 10 min; overkill (≥2× lethal dmg) = gib spray; corpses decay in 3 stages [MVP-lite: decal + 1 corpse frame] |
| UI | Helbreath-layout: bottom-left red/blue HP/MP bars (numbers on the bars), right-side wood/brass panels (inventory shows item art + weight), bottom-left chat, level always visible; ornate-silver trim from Soma for modal NPC dialogs |
| Screen | Optional CRT scanline shader (off by default); low-HP heartbeat vignette |
| Audio | Dry, close foley (thwack, squelch); sparse drones + bell ambience; no orchestral score |

## 3. Characters

### Stats (6 — Helbreath model) [MVP]

| Stat | Drives |
|---|---|
| STR | Melee dmg, gear weight reqs (full swing speed), carry weight |
| VIT | Max HP, HP regen |
| DEX | Hit rate, evade |
| INT | Spell-circle unlocks (all classes have small lists), curse resist |
| MAG | Max MP, MP regen, magic dmg, cast success |
| CHA | Party-aura radius, pet slots, guild creation (≥20), vendor prices |

- **+3 points per level** (never auto-assigned), no respec in MVP (a costly NPC
  respec arrives v0.2).
- Race: **Human only [MVP]**. **Vampire** race [v0.2] (Dark Eden asymmetry: no
  weapon slots, ring/amulet stacks, blood-drain XP, night-dominant) — see §10.

### Classes [MVP — all three]

| Class | Role | Stats | Identity |
|---|---|---|---|
| **Ravager** | Melee DPS / off-tank | STR → VIT/DEX | Big deliberate hits, armor shred, gap-close. Simple to start, timing to master. |
| **Gravecaller** | Ranged burst / control | INT → MAG | Plague-and-fire caster; DoTs, slows, fear; mana-shield gambling; corpse ammo. |
| **Cultist** (Pale Choir) | **Support**: heals + buffs + pet | VIT/INT → MAG/CHA | The party multiplier. Mend/Mass Mend, Bless, Ironskin, Haste, Purify, skeleton pet, curse debuffs. Resurrect at 20 = social glue. |

Skill lists (numbers = MVP start values, all cooldowns in server ticks @20 Hz):

**Ravager** — Power Swing (weapon 140%, 40t CD) · Sunder (-15% target DEF, 8s, 60t)
· Bull Rush (dash 3 tiles + brief knockdown, 120t) · War Stomp (PBAoE 6m dmg +
1s stagger, 160t) · Executioner (+100% dmg vs targets <20% HP, 100t) · Second Wind
(heal 25% over 5s, breaks on hit, 600t).

**Gravecaller** — Firebolt (nuke) · Frost Spike (lesser nuke + 20% slow 3s) · Wither
(DoT 12s, stacks 3) · **Corpse Explosion** (detonates a corpse: AoE, costs the
corpse — horror and utility in one) · Terror (single-target fear 2s, 400t) · Blood
Bolt (nuke costing own HP, +25% dmg at night) · Mana Shield (absorb via MP).

**Cultist** — Mend (single heal) · Mass Mend (party-hot, 300t) · **Bless** (+10% hit
& dmg, 5 min) · **Ironskin** (+20% DR, 5 min) · **Haste** (+12% move/atk speed,
3 min) — buffs are target-cast at first, party-wide via **Chorus** upgrades at 15/20 ·
Purify (cleanse + brief immunity) · Curse of Weakness (-15% target dmg/def) · Raise
Skeleton (1 pet tank/backup; CHA adds slots) · **Sanctuary** (ground healing circle,
600t) · **Resurrect** (lvl 20, 5 min CD, returns 50% of XP debt — the reason every
party wants one).

> Support-class design rule: a Cultist's raid DPS contribution ≈ 0.4 of a damage
> class, but a party WITH one out-farms/out-survives an equal party without one.
> Party XP share (§6) counts **healing at 100% contribution weight** so supports
> level at parity. This is the Taoist/Healer/Enchanter lesson made explicit.

### Weapon mastery (Soma adoption) [S6: weapon-skill combat spine; S10: auras]

- Every weapon class has a **use-based skill** (sword skill up by sword hits,
  era HB/Soma). Each 20 skill ⇒ +1 attack (min-damage bonus arm up).
- **Auras** at weapon-skill 20/50/80/120/150, claimed at the Bonesmith with
  *monster-part + gold tolls* (Soma's aura quests): e.g. +3 atk, 2× HP regen,
  3-target multiattack, power-swing amplifications. Aura grind = the "48-min
  farming +1" of our world.
- Spear geometry note: reach 1–2 (hit past an ally) reserved for a future
  4th weapon harness (cheap: range parameter in the resolver).

## 4. Combat math (lite)

- Hit: `hitChance = clamp(0.55 + (attackerACC - defenderEVD) * 0.01, 0.05, 0.98)`;
  ACC from DEX+gear, EVD from DEX/2+gear. (Era feel = whiffs are real early on.)
- Damage: `raw = weaponDmg * (1 + STR*0.02) * skillMult`, then mitigation
  `dmg = raw * (100 / (100 + DEF))` — diminishing armor, no hard cap.
- Crits: base 5% (+1%/10 DEX above 30), 170% dmg; melee overkill (≥2× lethal)
  triggers gore spray + tiny morale debuff aura on witnesses? **No** — keep gore
  purely cosmetic in MVP.
- PvP damage scalar **0.65** vs monsters so fights last seconds, not instants; pots
  500 ms global sip keep sustain honest.
- Cast interruption: taking a hit during a >0.5s cast adds +25% cast time
  (stagger), era-authentic counterplay vs casters.

## 5. Death, alignment, PK

- **Death [MVP]:** XP debt = 10% of current level's bar at low levels, scaling to
  ~25% at 25 (≈30–60 min of grind, Lineage-true); at 0 XP you **de-level**. You
  keep equipped items unless chaotic. Corpse marker persists 15 min (cosmetic +
  Cultist Resurrect anchor).
- **Alignment [MVP]:** karma −1000…+1000. Neutral 0–500, Lawful >500, **Chaotic <0
  = red name**. Chaos on killing a lawful/neutral non-enemy-town player: −(300 +
  20 × level-diff-if-victim-lower) — killing someone 15+ levels below you is instant
  deep-red. Karma regen: +1 per mob kill (≤ your level), +20/hour logged-in.
- **Chaotic penalties [MVP]:** town guards attack on sight; NPC shops refuse (fence
  NPC at the Smugglers' Cove buys at 60%); on death drop **1–6 random inventory
  items** and 15% chance per equipped slot; respawn at the gallows pit instead of town.
- **Moral economy (Soma lesson) [P3 tuning knob]:** alignment shouldn't be pure
  whip — lawful play carries a **small XP-faster carrot**, chaotic play a
  **loot-richer carrot**, so red is a gamble, not just a stain. Returns are tuned
  on alpha data, start ±5%-ish, capped and visible on the character sheet.
- **Safe zones:** towns + chapel grounds. **Combat zones:** all field/dungeon maps.
  Dueling flag (`/duel`, consent, no penalties) [MVP].
- **Enemy-town (EK) rule [MVP-lite, full in v0.2]:** sworn towns at war → kills of
  enemy sworn players cost no karma and feed the public **EK leaderboard**. MVP
  ships the leaderboard + war state; the opt-in declaration Nuance v0.2. *(Updated:
  since town-swear is level 19+, MVP must ship war-state or nobody is 19 in alpha —
  resolved: MVP ships the two-town war as the ONLY lawful-PvP channel.)*
- **Blood Curse [MVP-lite]:** bitten-type night mobs can apply a stacking curse
  (-10% all stats at night, +1% at dawn death); cured free at any chapel. Flavor
  seed for the Vampire race.

## 6. Progression, grind, party

- **Level cap 25 [MVP]** (100 planned full-game). XP curve:
  `xpNext(L) = 100 · L^1.85` rounded to 10s — L1→2 ≈ 100, L10→11 ≈ 7,000,
  L24→25 ≈ 36,000. Tuned for ~60–90 h to cap in alpha (era-slow but not cruel).
- **Monster XP** is per-mob flat by tier; elites ×8, nameds ×20, boss ×100.
- **Party [MVP]:** up to 5. Kill XP pool = `mobXP · (1 + 0.12·(n−1))`, split as
  **50% even-level share / 50% contribution share** (damage + healing×1.0 + tanking
  via damage-taken×0.5). Aura radius 12 tiles (CHA+0.5/stat). 8-man parties v0.2.
- **Why this curve:** a 5-man party earns 1.48× total XP per kill vs solo —
  mathematically the optimal way to play, always. Group-focus is enforced by math,
  not slogans.
- Leash range 18 tiles; mobs regen to full when leashed (kite-grief protection).

## 7. Items, loot, enhancement

**Slots:** weapon, shield/off-hand, helm, armor, gloves, boots, belt, amulet, 2× ring,
cape. (Vampires later: claws-fangs body slots + 6 jewelry.)

**Rarity tiers [MVP]:** Common (white) 78% · Magic (blue, 1–2 affixes) 17% ·
Rare (yellow, 2–3 affixes) 4.6% · Unique (named, fixed rolls, boss-only) 0.4%.
Affix pools are dark-themed: *of the Leech* (2–4% life on hit), *Grim* (+dmg to
undead), *Festering* (poison proc), *of the Vigil* (+light radius — mechanically
real at night), *Bloodforged* (+dmg at night), *of the Choir* (+buff duration).

**Enhancement — "the Anvil" [MVP]:** at the blacksmith, spend
**Blackiron Ore** (mined in Bonehowl Mine, purity 1–10) + a **fodder accessory** +
gold to refine a weapon/armor from +0 to +7:

| To level | Success | On failure | Notes |
|---|---|---|---|
| +1 | 100% | — | tutorializes the system |
| +2 | 90% | stays | |
| +3 | 80% | stays | |
| +4 | 65% | **−1 level** | |
| +5 | 50% | −1 level | |
| +6 | 35% | −1 level | item glows from +5 |
| +7 | 25% | **resets to +0** | pre-nerve ceiling; +8–10 [LATER] with break-risk + Blessed/Protection scrolls |

Ore purity adds −4%…+10%; fodder tier adds up to +5%. Every attempt costs
durability + gold → the great gold sink. (Mir's break-gamble arrives as scrolls
[LATER]; MVP resets hurt enough to generate stories without mass-quit risk.)

**Durability [MVP]:** items lose durability on death (5) and on refine (10); repair
at blacksmith for gold; at 0 the item is unusable (not destroyed).

**Economy [MVP]:** gold from mobs/selling; sinks = potions, repair, refine, teleport
scrolls, guild upkeep, castle tax. Player-to-player **trade window** [MVP];
stalls/auction [LATER]. Server-side transaction log for every trade (dupe audits).

## 8. Bloodpledges & the Weeping Castle siege

- **Bloodpledge (guild) [v0.2-minus… see MVP doc]:** creation needs CHA ≥ 20 +
  100k gold; ranks (Liege → Bloodsworn → Initiate); pledge storage; pledge chat.
  *Cut decision in 05-mvp: MVP ships pledge-lite = name/emblem/members/chat; storage
  & pledge-XP later.*
- **Siege [MVP, simplified]:** every **Saturday 20:00 UTC, 90 minutes**, one castle.
  Attackers (any pledged group, registration closes 24h prior) breach 2 destructible
  **gates** (100k HP, siege-damage-type skills ×3) → destroy the courtyard
  **Heartstone** (Guardian-Tower analogue) → any attacking liege starts a **60 s
  crown channel** on the throne; interrupt = restart; complete = ownership flips
  **immediately** and defenders become attackers. Most holds at horn = keep.
- **Holdings [MVP]:** owner sets town shop **tax 0–15%** (collected hourly to pledge
  vault), members get *Castle's Favor* buff (+10% XP & drops in territory), spawn
  shortcut. Mir-style winner buff package.
- Bots fill sieges in alpha (different bot profiles per archetype) so a 10-person
  alpha can still experience a 40-"player" siege.

## 9. Day/night & events

- **Cycle [MVP]:** 4 real hours = 24 game hours: Day 120 min, Dusk 30, Night 90,
  Dawn 30 (clock UI in HUD).
- **Night [MVP]:** global darkness (light radius: base 6 tiles; torches/lantern/
  of-the-Vigil extend), night-only spawns (Wraiths, Bloodfiends) worth +50% XP,
  Blood Bolt-type effects buffed, ambient dread audio.
- **Blood Moon [v0.2]:** weekly 20-min event — all spawn rates ×2, world boss
  **The Pale Sow** roams, all drops +loot-tier.
- Named elites on 15–60 min random timers (world-announced first-kill): *Old Maw*
  (fields), *The Red Widow* (mine), *Cantor Vex* (crypt).

## 10. Post-MVP content map

- **[v0.2]** Vampire race (bite→drain XP; night-dominant; jewelry-only gear;
  bat-dash; slayers gain "Hunter" sub-skills incl. see-invisible), full pledge
  systems (levels, storage, wars), Blood Moon, +8–10 scrolls, stalls.
- **[LATER]** Ouster-like elementalist; Crusade (monthly 6–9h nation war);
  second castle; Blood Bible relic hunts; mounts? (no — wagons/carriages flavor
  instead); hardcore ruleset server flag.

## 11. MVP content inventory (the art/asset budget)

| Asset | Count (MVP) | Notes |
|---|---|---|
| Tilesets | 4 (town, fields, mine, crypt) + castle kit | Tiled-compatible, 64×32 |
| Player sprites | 3 classes × (m/f bodies × 3 skin tones) from one base | gear = paper-doll tint/attach in MVP, full art swaps later |
| Monsters | 12 common + 2 elites + 3 named + 1 boss | see roster below |
| NPCs | 8 | static, 1 idle anim |
| UI | full skin set, ~60 widgets/icons | placeholder skin first, era-skin at P5 |
| VFX | ~25 (swings, casts, buffs, gore) | particles + sprite anims |
| SFX | ~40 | foley + ambience |

**Monster roster [MVP]:** Marsh Rat · Plague Bat · Feral Ghoul · Bonepicker Gnoll ·
Pale Cultist (caster) · Hollow Hound → **Mine:** Mine Wretch · Lantern Spider ·
Mud Golem → **Crypt:** Crypt Revenant · Grave Banshee (elite) · Bell Ringer
(summoner elite) → **Boss:** *The Gravemother* (crypt end: 3 phases, bell adds,
ground rot telegraphs). Each common mob: 8-dir walk(4f)/attack(3f)/die(3f) ≈
80 frames ≈ one evening of AI-gen + cleanup, or ~2 days of pixel-artist work —
**this is the number that governs the schedule** (see 05-mvp risk R2).

## 12. What we are explicitly NOT building

Auction house, achievements, quest-heavy PvE (quests = bounty board only:
kill-N/pickup-N), instanced dungeons, matchmaking, cosmetics shop, mobile/Windows
ports, controller support, localization (en only at alpha), account web portal
(plain launcher), pets-as-loot, fishing. Every one of these has sunk a similar
project; they are listed here so a future agent can't "just add" one casually.
