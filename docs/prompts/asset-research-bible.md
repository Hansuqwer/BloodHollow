# BLOODHOLLOW — Asset Research & Production Bible (issued 2026-09-07)

**Scope:** research, design, and production-ready specification for every visual
asset of BLOODHOLLOW — NPCs, monsters, terrain, weapons, item/skill icons, and
animations/VFX — in the era-authentic dark-horror 2D style of **Dark Eden**,
**Lineage 1 (incl. Remastered)**, **Helbreath**, with **Legend of Mir 2** and
**Myth of Soma** as supporting references.

**Governing documents (read first, never contradict):** `docs/01-research.md`
(era DNA), `docs/02-gdd.md` (art direction §2, roster §11), `AGENTS.md`. Shipped
content tables (`shared/content/mobs.h`, `items.h`, `kits.h`, `auras.h`) are
ground truth for names/stats; the GDD roster is the full target. Serve both.

**Outputs:** (1) per-ancestor reference dossier, (2) per-asset design briefs +
generation prompts, (3) production sheets conforming to §5, dropped in
`assets/aigen/` with a `LICENSES.md` entry (model + date). **Never touch
`assets/final/`** — human-art directory, AGENTS.md hard rule.

---

## 0. Execution order

One deliverable batch per stage, each gated by the §15 QA checklist:

1. **Research dossier** (§3) — study only; annotated screenshots/links per game.
2. **Style lock proof** (§4) — one 256×256 test scene (field + mob + player +
   spell) proving palette/scale/grade. Director approves before any volume work.
3. **Terrain** (§6) → **mobs** (§7) → **NPCs + players** (§8–9) →
   **weapons/items/icons** (§10–11) → **VFX/animations** (§12).
4. **Cleanup & packing** (§14) → **delivery** (§17).

Timebox anchor (GDD §11): one common mob ≈ 80 frames ≈ one evening of AI-gen +
cleanup. If a category overruns, simplify the design — never the spec.

---

## 1. Mission & world brief

The drowned kingdom of **Vessalia**. A blood-plague ("the Hollowing") rose from
the marshes a generation ago; two successor towns — **Thornwall** (west, dour
fortress of the **Ashen Compact**) and **Marrowgate** (east, zealot port of the
**Pale Synod**) — pay bounties for each other's heads; the **Weeping Castle**
between them changes hands every Saturday siege night.

Tone keywords (apply to EVERY asset): **mud, rust, candle-light, rot, incense,
teeth.** No elves, no sparkle, no high-fantasy brightness. Horror comes from
consequence — death, night, loss — not jump-scares. It must *feel* like
1999–2003, not "retro-styled 2026".

## 2. Fiction anchors for visual design

- **The Hollowing:** waterlogged flesh, black ichor veins, bell motifs (the dead
  are "rung" home), swollen corpse-gas silhouettes, grave-goods fused to bodies.
- **Ashen Compact (Thornwall):** ash-grey, soot, iron, nailed plank, dour
  military. Sigil: a nail through a leaf.
- **Pale Synod (Marrowgate):** bone-white, incense-gold, wax, choir robes,
  zealot script. Sigil: an open hymnal with a tooth.
- **Weeping Castle:** weep-stains down masonry, rusted portcullis, banner poles
  that recolor with the holding pledge.
- **The Pale Choir (Cultist class):** buffs/heals as choir-light and hymn-glyphs.
- **Night:** a predator, not a filter. Shadow pools in terrain should feel *placed*.


---

## 3. Ancestor research assignments (dossier stage)

Per game: capture 20–40 reference screenshots (mid-swing characters, terrain
edges/transitions, dense-PvP crowds, UI open, night scenes), then a 1-page
findings note answering the listed questions. Archive thumbnails under
`docs/research-notes/` (existing: `hb-video-thumb.jpg`).

### 3.1 Dark Eden (1997 / relaunch, SOFTON) — the horror register

- Study: vampire-vs-slayer asymmetry (race-readable silhouettes at tiny scale);
  the red/black/arterial palette; gothic UI framing; how curse/magic light uses
  saturated accents against a drained world. This is our **only** licensed
  source of neon: violet curses, arterial red, choir-gold — never on terrain.
- Note: blood-as-resource feedback (drain beams, blood pools); Enchanter-class
  buff telegraphing (our Cultist reference).

### 3.2 Lineage 1 (1998, NCSoft) + Remastered — the grade and the politics

- Study: the overall dark-fantasy color grade (this is OUR base grade);
  parchment/iron UI; alignment language (lawful white vs chaotic **red name**
  overhead — we need a 3-band karma name-tint set: lawful/neutral/chaotic);
  siege crowd composition at gates; castle interior dressing.
- Note: how Remastered upscales era sprites without losing the read — that is
  our cleanup tolerance limit: sharpen silhouettes, never re-style.

### 3.3 Helbreath (1999, Siementech) — the character & combat-storytelling lock

- **Character/VFX style lock** (director decision 2026-09-04). Hand-drawn,
  chunky, paper-doll-readable sprites; slightly exaggerated weapon scale so gear
  stays visible in a pile.
- Study: 15+ player mass-PvP readability (koreahb official video, thumbnail at
  `docs/research-notes/hb-video-thumb.jpg`); red-caps ability callouts over
  heads ("Ice-Storm!", "Berserk!") — our callout system; right-side wooden
  inventory panel with item art + weight "( 50 / 50 )" + UPGRADE button;
  bottom-left red/blue HP/MP bars with numeric readouts; green party names;
  floor/depth caption ("던전 지하 4층").
- Deliverable: a **readability rulebook** — max silhouette complexity, rim-light
  rules, team-color placement — extracted from those crowd shots.

### 3.4 Legend of Mir 2 (2001, WeMade) — the grind texture

- Study: mob density composition; "wall of monsters" field scenes; refine-gamble
  UI theater (the anvil moment — anticipation, toll, outcome); Taoist support FX
  (our Cultist's second ancestor).
- Note: what makes its fields feel endless and hostile at tiny sprite scale.

### 3.5 Myth of Soma (2001, Digital Bros) — the terrain lock

- **Terrain style lock:** painterly, pre-rendered-looking ground; dense rooty
  forests; swamp mud; broken aqueducts over still water; **tall, non-chibi
  humanoids** (sprites large relative to tiles — the zoom signature we want).
- Study: how scatter (root clusters, leaning gravestones, arches) is composed so
  monsters lurk in painted shadow pools; ornate gothic-silver UI trim (adopted
  for our modal NPC dialogs).
- **The Soma lesson (locked):** private servers shipped "permanent daytime"
  because nights were unreadably dark. Night tint alpha is capped at ~65% —
  horror comes from content, not darkness. Verify EVERY terrain piece under the
  capped night multiply before delivery.

---

## 4. Locked art direction (non-negotiable)

### 4.1 The split lock (director decision 2026-09-04)

| Layer | Lock | Meaning |
|---|---|---|
| **Terrain** | Myth of Soma | Painterly, pre-rendered look; rooted giant trees, swamp mud, broken aqueducts; taller zoom than Mir/HB peers. |
| **Characters & VFX** | Helbreath | Hand-drawn sprites, visible paper-doll silhouettes, crowd-readable spell FX. |
| **Combat storytelling** | Helbreath | Red-caps ability callouts over heads ("Power-Swing!", "Hell-Fire!"); party-name color coding. |
| **Grade** | Lineage 1 + Dark Eden accents | L1 dark-horror palette everywhere; DarkEden neon-gothic ONLY on curse/magic light (violet curse, arterial red, choir-gold). |

### 4.2 Palette & grade

- **≤32 colors per asset** (per-sprite-sheet / per-tileset-family), then a global
  dither pass. Era dithering is a feature — visible 2×2 checkerboard blends are
  correct; smooth gradients are an error.
- Base grade: desaturated earths — mud browns, rust reds, bone off-whites,
  soot greys, stagnant greens. Flesh is grey-pink, blood is the deepest red on
  screen. Candle-light is the warmest permitted light source.
- Reserved accent ramp (magic/curse only): violet `#8B5CF6`-family for curses,
  arterial crimson for blood magic, choir-gold for blessings. If an accent hue
  appears on terrain or mundane gear, the asset is rejected.
- Skin tones: 3 human tones (pale, sallow, weathered-tan) shared across classes.

### 4.3 Night rule (locked research finding)

Night = desaturated blue-black multiply + additive light mask, tint alpha capped
at ~65%. Every asset must pass the **night-floor test**: placed under the capped
night multiply it remains identifiable by silhouette alone. Warm light sources
(torch, lantern, "of the Vigil" gear) carve readable pools — design sprite
self-illumination hints accordingly (e.g. Lantern Spider's abdomen).

### 4.4 Gore language

Blood decals persist on ground (era: 10 min); overkill (≥2× lethal damage) = gib
spray; corpses decay in stages. Blood is matte and dark, never glossy cartoon
red. Gibs are meat-chunks + bone chips, ≤3 frames, readable at 32×48.

### 4.5 Crowd readability (Helbreath rule)

A 15-entity pile — players, mobs, summons, spell FX — must still parse:
- Every creature has a unique silhouette readable at 1× zoom in greyscale.
- Team/faction identity sits in ONE consistent location (overhead name tint +
  one gear trim color), never scattered across the sprite.
- Spell FX announce with shape + red-caps text; FX never fully occlude the caster.
- Weapon silhouettes exaggerated ~15% over anatomical so gear reads in a pile.


---

## 5. Hard technical spec (engine reality — verify against code, not memory)

- **Projection:** isometric 2:1 diamond. Ground tile **64×32 px**. World units
  are pixels; entities anchor at tile center, feet on the diamond.
- **Character frame:** **32×48 px** cell (procedural placeholder:
  `engine/assets/placeholder.cpp` — feet at ~y=42 of the cell, soft ellipse
  contact shadow painted inside the frame). Characters render ~56 px tall
  including headroom; oversize creatures use larger cells (bosses 48×64 or
  64×64) but keep the same feet-anchor and shadow rule.
- **Directions:** 8 rows per anim, row order = `sim::kDx/kDy` =
  **0=E, 1=SE, 2=S, 3=SW, 4=W, 5=NW, 6=N, 7=NE** (clockwise from East; y grows
  south/down-screen). Never mirror-flip E/W in the pipeline — draw all 8;
  era sprites have asymmetric gear.
- **Atlas format:** uniform grid PNG + JSON sidecar, loaded by
  `engine/assets/atlas.cpp`. Schema v1:
  `{"anims": {"walk": {"frameW":32,"frameH":48,"frames":6,"dirs":8,"fps":10,"offsetX":0,"offsetY":0}}}`
  One PNG per creature; rows = direction, columns = frames, per-anim blocks.
- **Animation sets & frame counts (GDD §2 lock):**
  - Players/NPCs: idle 1f · walk 6f · attack 3f · cast 4f · hurt 2f · die 4f ·
    gib 3f. FPS 8–12 per anim (era-chunky, not smooth).
  - Common mobs (budget line): walk 4f · attack 3f · die 3f ≈ **80 frames/mob**.
  - Elites/nameds: + hurt 2f. Boss (Gravemother): walk 4f · attack 3f ·
    cast 4f · hurt 2f · die 4f · summon 4f.
- **Timing truth:** swings resolve on the server's 20 Hz tick; anim is theater.
  Attack anims must hit their contact frame by frame 2 of 3.
- **Filtering:** point/nearest at render (integer zoom 1×/1.5×/2× only, base res
  1024×768 letterboxed). No anti-aliased edges — hard 1px outlines.
- **Transparency:** straight alpha, no premultiplied halos; 1px dark outline
  (~#1a1214 family) on all sprites for ground separation.
- **File targets:** common mob sheet ≤ 256×384 px; boss sheet ≤ 512×512; keep
  total shipped art under engine-friendly sizes — this is a zero-asset-download
  repo today and should stay lean.


---

## 6. Terrain & tilesets (Soma lock)

General anatomy per tileset: ground base tiles (dirt/mud/stone variants, ≥6
non-repeating 64×32 tiles each), edge/transition tiles between every adjacent
terrain pair (grass→mud, mud→water, floor→pit), wall/height tiles where the map
uses elevation illusion (drawn as raised blocks, top + left/right faces — see
`iso::drawPrism` for the engine's mental model), and a scatter layer
(non-blocking decals) + furniture sprites (blocking, wire kinds 64+).

### 6.1 Thornwall (town) — `thornwall.bhmap`

Dour fortress-town of the Ashen Compact. Muddy lanes between nailed-plank and
stone-footed buildings; soot-stained smithy (the **Anvil** — needs an anvil
furniture piece with ember-glow accent, the ONE warm light in town); chapel
grounds (safe zone) with leaning gravestones and a bell post; bounty board near
the square; gallows pit at the town edge (chaotic respawn — make it quietly
dreadful, not theatrical). Palette anchor: ash-grey, soot, iron, mud brown.

### 6.2 Fields of the Overflow — `fields_overflow.bhmap`

Drowned farmland: rotten furrows, fence lines collapsed into mud, stagnant
puddles with painterly reflections, scarecrow husks, root clusters at field
edges. This is the L1–7 grind zone — must read calm-by-day, wrong-by-night
using the SAME tiles (night multiply does the work; add 2–3 night-only scatter
variants like pale mushrooms/wisps). Named elite *Old Maw* dens here.

### 6.3 Bonehowl Mine — `bonehowl_mine.bhmap`

Wet cave: slick stone floors, braced timber tunnels, Blackiron ore veins
(refine currency — ore nodes are furniture with a dull iron-blue glint), drip
puddles, bone piles worked into walls, lantern hooks. Darker ambient than
fields but still within the night-floor rule. Named elite *The Red Widow*
(large spider) webs a chamber.

### 6.4 Drowned Crypt / Thornwall Crypt — `drowned_crypt.bhmap`, `thornwall_crypt.bhmap`

Submerged catacombs: flooded floor tiles (knee-deep black water with painterly
ripple dither), sarcophagi rows, votive candle clusters (warm light pools),
bell ropes hanging into dark, rot-bloom on masonry. Boss arena for **The
Gravemother**: a round drowned nave with a great cracked bell centerpiece.
Named elite *Cantor Vex* rings a side chapel.

### 6.5 Weeping Castle kit (post-MVP, design now)

Breachable gates (2 destructible states + rubble state), courtyard Heartstone,
throne dais, banner poles with pledge-recolor mask channel, wall walk tiles.
Castle is grey stone + weep-stains; identity comes from banners, not masonry.

### 6.6 Transition & tiling rules

- All transitions hand-painted (Soma painterly), NOT alpha-blended overlays.
- Water is still/black with dithered ripple — no animated water in MVP; motion
  comes from light and mobs.
- Every tileset ships a **palette strip PNG** (its ≤32 colors) for QA diff.


---

## 7. Monster briefs (Helbreath sprite lock)

Per-mob design template (fill ALL fields for each mob below):
`silhouette (1 line) · palette (5–8 named colors) · idle/walk motion note ·
attack tell (frame-2 contact) · death/gib note · night behavior cue ·
era reference (which ancestor mob it's descended from) · silhouette-test note`.

Shipped roster (from `shared/content/mobs.h` — these MUST exist):

| ID | Mob | Lv | Design brief |
|---|---|---|---|
| 1001 | Marsh Rat | 1 | Swollen drowned rat, matted fur, black ichor drool; low skitter; death = flop + bloat. Tutorial mob — must look pathetic. |
| 1002 | Feral Ghoul | 3 | Starved human-ish, grey-pink flesh, long nail-hands; hunched lope; attack = double rake; gib = torso burst. HB ghoul lineage. |
| 1003 | Hollow Hound | 5 | Ribs-visible mastiff, exposed skull half-mask; pack silhouette (lean, forward); lunge bite; eyes catch light at night (painted glint). |
| 1004 | Plague Bat | 2 | Fast, low-damage swarm mob (the "die red-faced" exam question); 2-frame wing blur is acceptable; greenish bloat belly; dies as a wet smack decal. |
| 1005 | Bonepicker Gnoll | 7 | Hyena-man wearing grave-goods: rib-cage armor, femur club; hunched knuckle-walk; laugh-tell before swing. Mid-fields anchor. |
| 1006 | Charnel Widow | 9 | Bloated grave-spider, abdomen = fused coffin-lid, silk = grey shroud-thread; slow deliberate steps, fast strike; drops Widow Silk. |
| 1007 | Gravecaller (mob) | 11 | Hollowed plague-priest caster: wax-mask face, bell censer; cast anim swings censer (arc of ember-dots); robe hem trails grave-mud. |
| 1008 | Revenant Sexton | 12 | Armored gravekeeper revenant, rusted spade-halberd, keys on belt; heavy 4f walk with drag step; attack = overhead spade split. |
| 1009 | **The Gravemother** (BOSS) | 14 | Crypt-end boss, 3 phases: drowned matriarch fused to a great bell — torso emerges from bell mouth; brood-spawn (bell-adds) cast anim = she rings herself; ground rot telegraph decals; 48×64 or 64×64 cell. Must dominate a crowd shot without hiding players. |
| 1010 | Sepulcher Elite | 12 | ×8 elite: giant tomb-warden variant of the Sexton — same family, 1.25× scale, chalk-white grave-bindings trim for elite readability. |

GDD-target roster not yet shipped (design now, implement later): **Pale Cultist**
(field caster, robe + choir mask), **Mine Wretch** (lantern-jawed miner undead),
**Lantern Spider** (night-light abdomen — see §4.3), **Mud Golem** (fields
heavy), **Crypt Revenant**, **Grave Banshee** (elite, wail = visible ring FX),
**Bell Ringer** (summoner elite, handbell + bell-adds), night-only spawns
**Wraith** (translucent, additive-light friendly) and **Bloodfiend**
(arterial-red accent license), nameds **Old Maw** (giant field rat), **The Red
Widow** (mine), **Cantor Vex** (crypt), and the v0.2 world boss **The Pale Sow**
(roaming plague-beast, Blood Moon event).

Elite/named readability: elites = same-family variant + white/gold trim + slight
upscale; nameds = unique silhouette + overhead name; never palette-swap alone.


---

## 8. NPC briefs (GDD §11: 8 NPCs, static, 1 idle anim each)

Static furniture-class entities (wire kind 64+); idle anim 4–6f optional. Each
needs: full sprite (32×48, 8-dir unnecessary — S/SW/SE facing enough if the
engine ever rotates them, but ship 8-dir idle for uniformity), a dialogue
portrait (96×96 painterly, ornate-silver frame), and one-line bark personality.

1. **Marta** — town vendor (Thornwall square). Broad, flour-dusted, keys and
   tally-sticks on belt; the sneer variant for refusing chaotics (red-name
   customers) is a PORTRAIT swap, not a sprite swap.
2. **The Bonesmith / Anvil priestesses (twins)** — aura + refine NPCs (RFC-0001
   Soma adoption): two soot-veiled sisters flanking the Anvil, one with hammer,
   one with tongs; ember-glow rim light; the gamble UI theater belongs to them.
3. **Chapel Confessor** — cures Blood Curse, lawful respawn anchor; Ashen
   Compact ash-robes, nailed-plank prayer board.
4. **Bounty Board** (furniture, not a person) — nailed notices, one blood-stained;
   needs 2 states (has-bounty / empty).
5. **Smugglers' Cove fence** — hooded, wax-seal ledger; buys from chaotics at
   60%; dockside grime, Marrowgate-adjacent.
6. **Town Guard ×2 factions** — Ashen Compact (ash-grey tabard, nail sigil) and
   Pale Synod (bone-white tabard, hymnal-tooth sigil); same base body, tabard +
   helm swap; these are the chaotic-hunting threat so they must look dangerous.
7. **Pledge Registrar** — Marrowgate zealot clerk, incense-gold ink stains;
   handles bloodpledge creation (CHA ≥ 20 + 100k gold).
8. **Castle Steward** — Weeping Castle; neutral grey robes, weep-stain motif;
   tax/ownership dialog anchor.

## 9. Player characters (3 classes × m/f × 3 skin tones, one shared base)

Paper-doll: one base body per sex; class identity via robe/armor overlay +
weapon; gear = tint/attach in MVP. 32×48 cells, all 8 dirs, full anim set
(idle 1 / walk 6 / attack 3 / cast 4 / hurt 2 / die 4 / gib 3).

- **Ravager** (melee DPS/off-tank): heavy shoulders, butcher-chain belt,
  oversized cleaver-axe silhouettes; movement is weight-forward. STR look.
- **Gravecaller** (ranged burst/control): wax-mask half-face, ichor-stained
  scholar robes, bell-censer off-hand; casts are censer arcs + hand jabs.
  Squishy read — narrow shoulders, longer robe.
- **Cultist of the Pale Choir** (support): choir robe with tooth-hymnal sigil,
  votive candles at shoulder-clasp (tiny warm lights, §4.3), staff-crook;
  cast anims are hymn gestures (open palm up / both arms raised for Chorus).
- Karma readability: chaotic players get red overhead name (wire already ships
  karmaBand); sprites themselves do NOT change — alignment is text + UI, not art.
- v0.2 note (design only): **Vampire race** — no weapon slots, claws/fangs body
  slots, 6 jewelry; Dark Eden asymmetry; night-dominant. Document silhouette
  direction now so the base body doesn't paint us into a corner.


---

## 10. Weapons, armor & item icons

Icon cell: **32×32** (inventory grid), 24×24 acceptable for stack-junk; hard
outline + 1px inner shadow; painterly-real (HB inventory look), not flat-glyph.
Rarity frame tints: Common white · Magic blue · Rare yellow · Unique named-gold
(GDD §7 drop rates 78/17/4.6/0.4 — the UNIQUE frame must feel like an event).

Shipped items (`shared/content/items.h` — MUST exist):

| ID | Item | Icon brief |
|---|---|---|
| 2001 | Rusty Shank | Nailed shiv, orange rust bloom, rag wrap. |
| 2002 | Pit Blade | Pit-fighting cleaver, notched edge, dark oil sheen. |
| 2101 | Hide Armor | Stitched swamp-hide cuirass, mud-cured patches. |
| 2102 | Bone Plate | Rib-bone lamellar over leather, grave-goods wiring. |
| 3001 | Blood Vial | Thumb-sized vial, dark red, wax cork; the era potion — must read instantly at 32px. |
| 4001 | Rat Pelt | Matted grey pelt, tail still on. |
| 4002 | Ghoul Finger | Severed grey finger, black nail. |
| 4003 | Hound Fang | Curved yellowed fang, root up. |
| 4004 | Widow Silk | Spool of grey shroud-thread. |
| 4005 | Revenant Ash | Pinch of pale ash in twist of paper (aura currency — slightly special frame). |

Also required: **Blackiron Ore** (purity 1–10 — one base icon + numeral overlay,
dull iron-blue glint), fodder accessory tier icons, gold coins (3 stack sizes),
affix pips for name suffixes (of Whet / of Warding / of Leech — tiny inline
glyphs for tooltips), and **enhancement states**: +1..+7 shown as numeral badge;
**from +5 the item icon gains a glow variant** (GDD §7: "item glows from +5") —
produce glow overlay sprites, engine composites them.

Weapon-future note: sword/axe/spear families exist in design (weapon mastery,
aura tiers at skill 20/50/80/120/150); spears need a longer diagonal silhouette
for the 1–2 tile reach read. Draw the icon family tree now even though MVP
ships two weapons.

## 11. UI icons & widgets (Helbreath layout, Soma modal trim)

- **Skill hotbar icons 1–8** (32×32, glyph + colored backing): Power Swing,
  Sunder, Bull Rush, War Stomp, Executioner, Second Wind (Ravager) · Firebolt,
  Frost Spike, Wither, Corpse Explosion, Terror, Blood Bolt, Mana Shield
  (Gravecaller) · Mend, Mass Mend, Bless, Ironskin, Haste, Chorus, Purify,
  Curse of Weakness, Raise Skeleton, Sanctuary, Resurrect (Cultist).
  Backing-color language (T-066 lock): heal **green**, bless **gold**, curse
  **violet**, damage **iron/red**, utility **ash-blue**.
- **F1–F8 potion/scroll side-slots**: vial/scroll mini-icons + count badge.
- **Core chrome**: bottom-left red/blue HP/MP bars with numeric readouts; level
  always visible; right-side wood/brass panels (inventory shows item art +
  weight "( 50 / 50 )" + UPGRADE button); bottom-left chat; clock dial
  (day/dusk/night/dawn quadrants — the 4-hour cycle); EK leaderboard panel;
  bounty board UI; trade window; anvil/refine modal (the gamble theater —
  success/degrade/destroy states need distinct chrome).
- **Modal NPC dialogs**: ornate gothic-silver trim (Soma), parchment body.
- **Karma bands**: lawful white / neutral grey / chaotic **red** name tints +
  overhead badge glyphs.
- Target ≈ 60 widgets/icons (GDD §11); placeholder skin first, era-skin at P5.


---

## 12. VFX & combat animation briefs (~25 total, GDD §11)

Sprite-anim FX (grid sheets, same atlas pipeline) + particle-friendly frames.
All FX obey crowd-readability: short (≤0.6 s), shaped, never opaque over a
caster; night-visible (FX are additive-light friendly).

- **Swing arcs** (3 weapon families × 3f): iron-grey arc + 1-frame white
  contact flash (the T-066 2-frame hit flash: white→palette).
- **Firebolt / Frost Spike / Blood Bolt**: bolt 4f + impact 4f. Blood Bolt is
  arterial-crimson and gets a +night variant brightness (mechanically +25% at
  night — FX should hint it).
- **Wither (DoT)**: creeping violet-black ground blight decal, 3-stage.
- **Corpse Explosion**: corpse-frame → meat-burst 4f + decal; the horror/utility
  signature — this one may be the loudest FX in the game.
- **Terror**: violet skull-glyph flash over head, 2f.
- **Mana Shield**: translucent violet shell, 2-state (idle/absorb-flash).
- **Mend / Mass Mend**: choir-gold rising motes; Mass Mend adds a ground ring.
- **Bless / Ironskin / Haste**: gold/grey-blue/ash-blue overhead glyph + brief
  body rim; buffs must be distinguishable by silhouette at a glance.
- **Sanctuary**: persistent healing circle ground decal, hymn-glyphs on ring.
- **Raise Skeleton**: ground-crack 3f + skeleton pet (own mini-sheet: 8-dir
  walk 4f/attack 3f/die 3f — reuse ghoul-family skeleton bones look).
- **Resurrect**: white-gold pillar + choir sting moment; the social-glue spell,
  make it feel like relief.
- **Callouts**: red-caps text system ("Power-Swing!", "Hell-Fire!") — deliver
  the bitmap-font treatment + backing plate, not per-word sprites.
- **Gore**: blood decal set (6+ variants, 3 decay stages), gib chunks (3f),
  corpse frames per mob family, petrify-fade death dissolve (T-066).
- **Aura tiers I–V** (weapon mastery rewards): subtle weapon-trail tints I–II,
  visible aura corona from III (multiattack tier), V = the "48-min farming +1"
  badge — must be visible across a siege crowd.
- **Boss kit (Gravemother)**: Blood Bolt cast (6-tile range), bell-ring summon
  (shockwave ring + add spawn flash), ground-rot telegraph decals (danger
  readability is mechanical — telegraphs must be unmistakable but era-styled).

## 13. Master image-generation prompt templates

**Global style prefix (prepend to every generation):**
> "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror
> dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone
> off-white, soot grey, stagnant green), hard 1px dark outline, visible dither
> shading, no anti-aliasing, no gradients, matte dark blood, candle-light
> warmth only as accent, plain flat #00FF00 or transparent background,
> orthographic 2:1 isometric view"

**Global negative prompt (append always):**
> "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient,
> 3D render, painterly character (painterly is TERRAIN ONLY), bloom, lens
> flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear"

**Per-category skeletons:**
- Mob: `PREFIX + "[name], [silhouette line], [5–8 palette colors], [pose: 3/4
  south-facing idle], full body, feet on ground plane, single character" +
  NEGATIVE` → then direction/frame expansion via img2img with low denoise.
- Terrain tile: `PREFIX (painterly override: "pre-rendered painterly ground,
  Myth of Soma style") + "[biome], seamless isometric diamond tile 2:1,
  top-down ground texture" + NEGATIVE`.
- Icon: `PREFIX + "[item], single object, centered, 3/4 view, inventory icon,
  dark wood panel background" + NEGATIVE`.
- VFX frame: `PREFIX + "[effect], game VFX sprite, single frame, additive-glow
  friendly, [accent color from reserved ramp]" + NEGATIVE`.

Document every final prompt next to the delivered sheet (reproducibility =
an agent can re-derive the asset).


---

## 14. Cleanup & post-processing spec

1. Generate at 4× target, downscale with nearest-neighbor to final cell size.
2. Background removal → straight alpha; hand-repair the 1px outline (AI output
   almost always breaks outlines on claws/teeth/staves — check these first).
3. Palette quantize to the asset's ≤32-color ramp (one shared ramp per mob
   family/tileset); apply the global dither pass last.
4. Direction expansion: generate S/SE/E natively, then derive the remaining 5
   by img2img with pose guidance — hand-fix gear asymmetry (a shield must stay
   in the same hand across all 8 dirs).
5. Frame expansion: build walk/attack cycles by pose-interp + hand cleanup;
   enforce the contact-frame rule (attack hits on frame 2 of 3).
6. Pack to the uniform-grid atlas PNG + write the JSON sidecar (§5 schema);
   validate by loading through `bh::loadAtlas` in a scratch client build —
   the engine is the final linter.
7. Night-floor pass: screenshot every sprite/tile under the capped night
   multiply; failures get rim-light or palette lift, never alpha hacks.

## 15. Acceptance criteria (QA gates per batch)

- [ ] **Greyscale silhouette test**: asset identifiable vs 3 same-tier peers in
      greyscale at 1× zoom.
- [ ] **Crowd test**: composited 15-entity pile (players + mobs + 2 spell FX)
      still parses; callouts readable.
- [ ] **Night-floor test**: readable under the ~65% capped night multiply.
- [ ] **Palette audit**: ≤32 colors, palette strip shipped, no accent-ramp hues
      outside magic/curse assets, dither present.
- [ ] **Era test**: would this screenshot pass as a 1999–2003 capture next to
      HB/L1/DarkEden references? If it reads "modern indie", it fails.
- [ ] **Tech audit**: 8-dir row order correct (E,SE,S,SW,W,NW,N,NE), feet on
      the diamond, frame counts per §5, atlas JSON loads in engine, point
      filtering clean at 1×/1.5×/2×.
- [ ] **Fiction audit**: tone keywords honored; faction sigils correct; no
      elves, no sparkle.

## 16. Licensing & IP guardrails

- Study ancestors; never trace, rip, or upscale their shipped sprites. All
  output must be original generation + hand cleanup.
- AI-generated assets → `assets/aigen/` + `assets/LICENSES.md` entry (model,
  date, prompt reference). Free packs need upstream URL + license entry.
  `assets/final/` is reserved for human-commissioned art (AGENTS.md) — agents
  must not write there.
- No real-world religious iconography verbatim; the Pale Synod/Choir is
  fictional and must stay visually fictional.

## 17. Delivery, naming & batching

```
assets/aigen/
  terrain/{town,fields,mine,crypt,castle}/   tiles + scatter + palette strip
  mobs/{1001_marsh_rat,...,1010_sepulcher_elite}/  sheet.png + sheet.json + prompt.md
  npcs/{marta,bonesmith_twins,confessor,...}/      sheet + portrait + prompt.md
  players/{ravager,gravecaller,cultist}/{m,f}/     paper-doll sheets
  icons/{items,skills,ui}/                          icon sheets + atlas JSON
  vfx/{swings,casts,buffs,gore,boss}/               frame sheets + prompt.md
docs/research-notes/   ancestor dossiers + style-tile proof + QA composites
```

Batches (each = one reviewable drop, mirroring the repo's one-system-one-commit
discipline): **B0** dossiers + style tile · **B1** town+fields terrain ·
**B2** mine+crypt terrain · **B3** mobs 1001–1005 · **B4** mobs 1006–1010 +
boss · **B5** NPCs + portraits · **B6** player bases ×3 classes · **B7** item +
skill + UI icons · **B8** VFX + gore + callouts · **B9** night-floor + crowd
composite QA pass on everything.

Definition of done for the whole engagement: every shipped mob/NPC/item row in
`shared/content/*.h` has matching art loaded by the engine; the GDD §11 MVP
inventory (4 tilesets + castle kit, 12+2+3+1 monsters, 8 NPCs, ~60 UI widgets,
~25 VFX) is either delivered or has a dated design brief ready for production.

