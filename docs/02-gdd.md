# 02 — Game Design Document / BIBLE v2 (BLOODHOLLOW)

BIBLE v2 (2026-09-16, director-locked): **horror Lineage1 / Dark Eden, really dark.**
Feel law, not flavor. Factions SKIPPED for MVP (see §1, §5, §10). Dark law in §2 + §9
is normative: if a number conflicts, §2/§9 wins over older rows.

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

Characters start neutral in the refugee camp. **MVP: no town oath, no level gate, no
second-city sim** (director cut 2026-09-16 — Marrowgate exists as lore + EK-target
placeholder only, no population). The L19 Helbreath oath (swear to Thornwall vs
Marrowgate) returns in **[v0.2] with the Vampire race** as a true race-war. MVP
politics is pledges + castle holder vs everyone + karma/red-names (see §5, §8, §10).
Between them: **Weeping Castle**, held by whichever bloodpledge took it last siege night.

Tone keywords: *mud, rust, candle-light, rot, incense, teeth.* No elves, no sparkle.
Horror comes from consequence (death, night, loss) more than jump-scares.
**BIBLE v2 feel law: Lineage1 dread (open PK, red-names, drops, guards, siege tax)
× Dark Eden night warfare (night-dominant horrors, light = life, infection).
Really dark, but readable-dark — never blind (see §2, §9).**

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

### Art-direction spec (era-authentic) — HORROR LAW [MVP, normative]

Horror rule: darkness is gameplay, not blindness. Night must force torches,
lanterns and Vigil affixes, hide monsters in shadow pools, and make Curse/magic
light the only neon on screen — while a human at 04:00 with a torch can still
grind. T-164 proves this with screenshots (torch vs no-torch, zoom 1/1.5/2).

| Element | Spec |
|---|---|
| Palette | Per-asset ≤32 colors; global dither pass; night = desaturated blue-black multiply **78–80% alpha + additive light mask** (BIBLE v2: darker than the old 65% cap; Soma lesson still holds — never pitch-black, servers that went full-black shipped permanent-daytime as a QoL hack). Day stays mud/rust/candle-light; night goes Lineage-C + DarkEden dread. |
| Look target | **Horror Lineage1 × Dark Eden split lock** (BIBLE v2 2026-09-16, supersedes 2026-09-04 HB-favored lock): **terrain** = Myth of Soma painterly pre-rendered (rooted giant trees, swamp mud, broken aqueducts, taller zoom than Mir) graded dark-horror; **characters & VFX** = Helbreath hand-drawn sprites with visible paper-doll silhouettes + crowd-readable spell FX (a 15-player pile must still parse); **combat storytelling** = HB red-caps ability callouts over heads ("Power-Swing!", "Hell-Fire!") + party-name colors; **grade** = Lineage 1 dark-horror palette with DarkEden neon-gothic accents reserved for curse/magic light — curse/magic is the ONLY neon |
| Terrain dressing | Painterly scatter: root clusters, leaning gravestones, broken pillars/aqueduct arches over water, mud puddles; monsters lurk in shadow pools (Soma composition). Night: shadow pools are real hiding spots (night-only spawns sit in them) |
| Sprites | 8 directions; walk 6f, attack 3f (era-authentic minimalism), cast 4f, hurt 2f, die 4f, gib 3f |
| Gore | Blood decals persist on the ground for 10 min; overkill (≥2× lethal dmg) = gib spray; corpses decay in 3 stages [MVP-lite: decal + 1 corpse frame]. Gore is the horror UI — blood on the ground tells the story |
| UI | Helbreath-layout: bottom-left red/blue HP/MP bars (numbers on the bars), right-side wood/brass panels (inventory shows item art + weight), bottom-left chat, level always visible; ornate-silver trim from Soma for modal NPC dialogs. Night clock always visible in HUD |
| Screen | Optional CRT scanline shader (off by default); low-HP heartbeat vignette (<25% HP). Fog edge at light-radius falloff |
| Audio | Dry, close foley (thwack, squelch, footsteps); sparse bell drones + night dread ambience; low-HP heartbeat; distant sow-bell for nameds; no orchestral score, no music. Quiet is dread — no global spam |

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
- **Enemy-town (EK) rule [MVP-rescoped BIBLE v2, full town-war in v0.2]:** MVP has NO
  town oath and NO lawful town-vs-town channel (skip-factions cut 2026-09-16).
  `/ek` + the board track **pledge/PK fame** (kills, reds, castle flips) instead of
  town EK. Town-war (sworn kills cost no karma + public town EK leaderboard) returns
  in v0.2 with Vampire race-war. Old note retired: MVP no longer ships any two-town
  war as the lawful-PvP channel — lawful PvP in MVP is duels + siege + pledge wars.
- **Blood Curse [MVP — horror law]:** bitten-type night mobs apply a stacking curse
  (-10% all stats at night per stack, +1% dawn-death risk); cured free at any chapel.
  Curse light is neon-gothic (the only neon). Flavor seed for the Vampire race, and
  MVP's main night-dread mechanic alongside light radius.

## 6. Progression, grind, party

- **Level cap 25 [MVP]** (100 planned full-game). XP curve:
  `xpNext(L) = 100 · L^1.85` rounded to 10s — L1→2 ≈ 100, L10→11 ≈ 7,000,
  L24→25 ≈ 36,000. Tuned for ~60–90 h to cap in alpha (era-slow but not cruel).
- **Monster XP** is per-mob flat, set in content rows (`shared/content/mobs.h`
  `xp` — the ×8/×20/×100 tier-multiplier law was never implemented; rows ARE
  the law, ADR-0013).
- **Party [MVP]:** up to 5. Kill XP pool = `mobXP · (1 + 0.12·(n−1))`, split as
  **50% even-level share / 50% contribution share** (damage + healing×1.0 + tanking
  via damage-taken×0.5). Aura radius 12 tiles (CHA+0.5/stat). 8-man parties v0.2.
- **Why this curve:** a 5-man party earns 1.48× total XP per kill vs solo —
  mathematically the optimal way to play, always. Group-focus is enforced by math,
  not slogans.
- Leash range 18 tiles default, per-row `leashRadius` in content (`mobs.h`);
  mobs regen to full when leashed (kite-grief protection).

## 7. Items, loot, enhancement

**Slots [MVP-shipped, T-159]:** weapon, armor, helm, amulet, ring (5 equippable)
+ consumables + junk. Shield/off-hand, gloves, boots, belt, cape are [LATER].
(Vampires later: claws-fangs body slots + 6 jewelry.)

**Rarity tiers [MVP-shipped, T-159]:** Common (white, no affix) 78% · Magic (blue,
1 affix) 17% · Rare (yellow, 1 affix) 4.6% · Unique (named, fixed rolls,
boss-only) 0.4%. Multi-affix items need a schema change — deferred (T-159f1).
Affix pools are dark-themed (20 affixes): *of the Leech* (2–4% life on hit), *Grim* (+dmg to
undead), *Festering* (poison proc), *of the Vigil* (+light radius — mechanically
real at night), *Bloodforged* (+dmg at night), *of the Choir* (+buff duration),
*of the Hollow* (+3 flat dmg), *Grave-touched* (+1 OOC regen), *of the Crypt*
(-10% incoming dmg), *of the Marrow* (+3% lifesteal), *of the Pall* (+2 acc +1 evd),
*of the Boneyard* (+5% crit), *of the Dirge* (+4 dmg at night), *of the Husk* (+2 flat def),
*of the Tithemaster* (+15% kill gold), *of Last Rites* (+8 dmg &lt;20% hp).

**Enhancement — "the Anvil" [MVP-shipped law, ADR-0013]:** at the blacksmith,
spend one **Blackiron Ore** (flat, mined in Bonehowl Mine — purity tiers [LATER])
+ one **junk** + 50g toll to refine gear from +0 to +7:

| To level | Success | On failure | Notes |
|---|---|---|---|
| +1 | 100% | — | tutorializes the system |
| +2 | 100% | stays | mercy row (shipped) |
| +3 | 60% | **SHATTERS** | destruction coin (shipped) |
| +4 | 65% | **−1 level** | |
| +5 | 50% | −1 level | item glows from +5 |
| +6 | 35% | −1 level | |
| +7 | 25% | **resets to +0** | pre-nerve ceiling; +8–10 [LATER] with break-risk + Blessed/Protection scrolls |

Fodder tiers [LATER] with +8–10 scrolls. Every attempt costs 50g + toll junk →
the great gold sink. (Mir's break-gamble arrives as scrolls [LATER]; MVP
shatter-at-2 + resets hurt enough to generate stories without mass-quit risk.)
(Dead law, harmless: a +10 mythic silhouette exists in code but is unreachable
at the +7 cap.)

**Durability [MVP]:** items lose durability on death (5) and on refine (10); repair
at blacksmith for gold; at 0 the item is unusable (not destroyed).

**Economy [MVP]:** gold from mobs/selling; sinks = potions, repair, refine,
guild upkeep, castle tax (teleport scrolls [LATER] — no scroll item ships, T-163
records the cut). Player-to-player **trade window** [MVP]; stalls/auction
[LATER]. Server-side transaction log for every trade (dupe audits).

## 8. Bloodpledges & the Weeping Castle siege

- **Bloodpledge (guild) [MVP-lite]:** creation needs **L≥10 + 10,000g (shipped law,
  BIBLE v2)**; ranks (Liege → Bloodsworn → Initiate); pledge storage deferred;
  pledge chat + emblem + vault (tax-only). CHA ≥20 + 100k gate was the old spec —
  deferred to v0.2 with factions (CHA decision in T-160).
  *Cut decision in 05-mvp: MVP ships pledge-lite = name/emblem/members/chat; storage
  & pledge-XP later.*
- **Siege [MVP-shipped law, ADR-0014]:** every **Saturday 20:00 UTC, 90 minutes**,
  one castle. Attackers (any pledged group, registration closes 24h prior)
  breach 2 destructible **gates** (300 HP each, felled by the `/breach` ram at
  10 dmg ≈ 30 actions — no siege-damage skills in MVP) → contest the courtyard
  **Heartstone** (Guardian-Tower analogue: 60 s uncontested presence attunement)
  → any attacking liege starts a **10 s crown channel** on the throne
  (re-kneel mid-channel is a no-op); interrupt = restart; complete = ownership
  flips **immediately** and defenders become attackers. Most holds at horn = keep.
- **Holdings [MVP]:** owner sets town shop **tax 0–15%** (collected hourly to pledge
  vault), members get *Castle's Favor* buff (+10% XP & drops in territory), spawn
  shortcut. Mir-style winner buff package.
- Bots fill sieges in alpha (different bot profiles per archetype) so a 10-person
  alpha can still experience a 40-"player" siege.

## 9. Day/night & events — HORROR LAW [MVP, normative]

- **Cycle [MVP]:** 4 real hours = 24 game hours: Day 120 min, Dusk 30, Night 90,
  Dawn 30 (clock UI in HUD, always visible).
- **Night [MVP — really dark, readable-dark]:** global darkness **78–80% desat
  blue-black multiply + additive light mask** (BIBLE v2 law, T-164 proves with
  screenshots). Light radius law: **base 0; torch 6, lantern 8, of-the-Vigil
  extends** — without light you can walk, you cannot grind. Torch economy is
  balanced around this. Night-only spawns (Wraiths, Bloodfiends) sit in shadow
  pools, worth **+50% XP**, Blood Bolt-type effects buffed (+25% at night).
  Ambient dread audio (bell drone, no music), fog at light falloff. First-kill
  world call only — quiet is dread.
- **Blood Moon [MVP-lite shipped, full event v0.2 — ADR-0014]:** curse duration
  ×2 + night bite 130%, GM-raised session flag lasting to next dawn, never
  persisted. Weekly 20-min event shape — all spawn rates ×2, world boss **The
  Pale Sow** roams, all drops +loot-tier — stays [v0.2].
- Named elites on 15–60 min rotating timers (world-announced first-kill, T-097 reword):
  *Old Maw* (fields — shipped T-101: Gnoll-base L7, 30-min pit), *The Red Widow*
  (mine — shipped T-102: Widow-base L9, 45-min nest), *Cantor Vex* (crypt —
  shipped T-103: Sexton-base L12 bolt-caster, 60-min choir).

## 10. Post-MVP content map

- **[v0.2 — faction return]** Town oath L19 (Ashen Compact vs Pale Synod) + Marrowgate
  population/city sim + town-war EK + CHA ≥20 pledge gate; Vampire race (bite→drain
  XP; night-dominant; jewelry-only gear; bat-dash; slayers gain "Hunter" sub-skills
  incl. see-invisible), full pledge systems (levels, storage, wars), Blood Moon,
  +8–10 scrolls, stalls.
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
ground rot telegraphs — shipped: bolt replaced by 60t wind-up + radius-2 slam,
bolt-mirror damage/curse, movers dodge; T-091). Each common mob: 8-dir walk(4f)/attack(3f)/die(3f) ≈
80 frames ≈ one evening of AI-gen + cleanup, or ~2 days of pixel-artist work —
**this is the number that governs the schedule** (see 05-mvp risk R2).

## 12. What we are explicitly NOT building

Auction house, achievements, quest-heavy PvE (quests = bounty board only:
kill-N/pickup-N), instanced dungeons, matchmaking, cosmetics shop, mobile/Windows
ports, controller support, localization (en only at alpha), account web portal
(plain launcher), pets-as-loot, fishing. **BIBLE v2 MVP cuts: town oath + second-city
(Marrowgate) sim + town-war EK + Blood Moon event + CHA≥20 pledge gate (all v0.2).**
Every one of these has sunk a similar project; they are listed here so a future
agent can't "just add" one casually.
