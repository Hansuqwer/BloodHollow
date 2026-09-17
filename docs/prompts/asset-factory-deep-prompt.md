# BLOODHOLLOW — ASSET FACTORY: DEEP PRODUCTION PROMPT (issued 2026-09-17)

**What this is.** A single, self-contained, execution-grade prompt package for
producing *every visual asset* of BLOODHOLLOW — world/terrain, furniture,
mobs, NPCs, players, items/icons, VFX — in the era-authentic dark-horror
2D isometric pixel-art style of **Dark Eden**, **Lineage 1 (incl.
Remastered)** and **Helbreath**, with **Legend of Mir 2** / **Myth of Soma**
as supporting references. Written for AI art agents (one agent = one asset
folder or one batch) under the human director.

**How to use it.** An agent executing an asset or batch MUST read, in order:
§0–§2 (always), the category section for its asset (§3–§9), §10 (free-pack
policy — always, because every batch may use `assets/free/` reference or
placeholder material), and §12–§13 (QA + delivery). Then produce, QA, and
deliver. If a detail here conflicts with a shipped spec in
`assets/aigen/REGISTRY.md`, `docs/art/*.md` or the brieves in
`assets/aigen/**/BRIEF.md`, **the shipped spec wins** — flag the conflict in
the batch record, never silently fork it.

**Governing documents (never contradict):** `AGENTS.md` (hard rules, incl.
never touch `assets/final/`), `docs/02-gdd.md` (art direction, roster),
`docs/prompts/asset-research-bible.md` (style bible — this document
operationalizes it), `assets/aigen/REGISTRY.md` (paths/cells/anchors —
lookup contract), `docs/art/B0-GATE-DECISION.md` (binding rulings D1–D12),
`assets/LICENSES.md` (provenance gate).

**Free base material now available:** `assets/free/` — 17 curated CC0 packs
(world / furniture / mobs / NPCs / VFX / weapons; Kenney, 2026-09-17).
§10 governs how they may be used.

---

## §0 Mission & tone (applies to EVERY pixel)

The drowned kingdom of **Vessalia**. A blood-plague ("the Hollowing") rose
from the marshes a generation ago. Two successor towns — **Thornwall** (west;
dour fortress of the **Ashen Compact**: ash-grey, soot, iron, nailed plank,
sigil = a nail through a leaf) and **Marrowgate** (east; zealot port of the
**Pale Synod**: bone-white, incense-gold, wax, choir robes, sigil = an open
hymnal with a tooth) — pay bounties for each other's heads. The **Weeping
Castle** between them changes hands every Saturday siege night; weep-stains
run down its masonry, its banner poles recolor with the holding pledge.

Tone keywords (apply to every asset): **mud, rust, candle-light, rot,
incense, teeth.** No elves, no sparkle, no high-fantasy brightness. Horror
comes from *consequence* — death, night, loss — not jump-scares. It must
*feel* like 1999–2003, not "retro-styled 2026". The one licensed exception
to the drained palette is **magic**: violet curses, arterial red, choir-gold
— saturated accents that exist *only* on FX, auras, and the `night` mob
family (Wraith/Bloodfiend/Banshee). Never on terrain, never on skin or
cloth.

Era DNA to reproduce: isometric 2:1, chunky readable silhouettes at tiny
scale, deliberate swing/cast timings, parchment UI, blood that *persists*
on the ground, darkness as a mechanic (02:00 night grade is a production
gate, not a filter).

---

## §1 Style lock — the binding contract

### 1.1 Projection & measurement

- View: **orthographic 2:1 isometric**, screen-space. Ground diamond = 32×16
  px per tile.
- Ground plates: **512×256 px per ground id** (= 8×8 diamonds), painted
  whole then cut at screen position by `bhpix.cut_diamond` (plate coords =
  world coords mod plate). Never paint repeating 32×16 tiles — the
  checkerboard artifact is a reject.
- Edge/transition pieces: **64×32 diamonds**, 8 per adjacency pair (faces
  NE/SE/SW/NW + corners N/E/S/W), drawn *on top of* the base tile — no
  alpha-blend overlays.
- WALL (id 2) = prism: top 64×32 + left/right faces 32×28, painted per
  tileset (`prism/{top,left,right}` + footing `skirt_*`). **WALL never
  bleeds** — a footing skirt tile handles every WALL↔X transition.
- Entities: common **32×48** cell, **anchorY 42** (feet row; body ≤ 43 px);
  elite **40×60**, anchorY 52; boss **64×64**, anchorY 58; furniture
  **48×48**, anchorY 44; NPC 32×48, anchorY 42; portrait **96×96**
  (painterly bust, *not* pixel art — it sits inside the Soma-modal silver
  frame).
- Sheets: one PNG per atlas (loader constraint). Rows = 8 dirs in order
  **E, SE, S, SW, W, NW, N, NE**. Cols = anim frames. Caps: common mob
  ≤ 320×384 (10 cols) · elite ≤ 480×480 (12 cols) · boss ≤ 1344×512 (21
  cols) · player ≤ 736×384 (23 cols) · NPC ≤ 128×384 (twins 256×384).

### 1.2 Colour & light

- Family palettes: **≤ 32 opaque colours per family ramp**
  (`assets/aigen/palettes/families.json`); terrain ≤ 32 per zone
  (`palette_<zone>.png`). Quantize with Bayer-2 dither.
- Luma floors: entity body mean ≥ 25; **per-plate mean 45–70, min ≥ 24**
  (above outline); night 02:00 grade re-render archived for every plate and
  sheet (the R-NIGHT gate — no engine light mask exists yet, so the *painted*
  asset must already read at night).
- Outline: **hard 1 px**, colour `#1a1214`, on every entity and on
  occluding furniture. No anti-aliasing anywhere. Visible dither shading,
  no gradients, no smooth falloff.
- Base world palette (use as anchors, vary per zone): mud brown, rust red,
  bone off-white, soot grey, stagnant green. Candle-light warmth is an
  *accent* (ember in the anvil trough, votive candles, window glints) —
  it never lights a whole tile.
- Luma ceiling: **no pixel above luma 200** unless it is a light source,
  eye glint, hit-flash, or bone highlight.
- Accent-hue rule: violet / arterial / choir-gold **only** on FX, auras,
  buff strips, and the `night` family. On skin, cloth, or terrain-coloured
  gear it is an automatic reject.

### 1.3 Anatomy rules (era lock)

- One full 1 px outline, closed. AI breaks outlines first at: claw/teeth/
  tail tips, weapon tips, staff heads, nail-through-leaf sigils. Hand-fix
  these before anything else.
- Feet on the anchor row in every frame of every direction; no painted
  floating shadows (the pipeline adds an alpha-70 contact ellipse).
- Weapon/staff attachment stays in the **RIGHT hand in all 8 dirs**.
  Derive W-side dirs by img2img — **never mirror output** (gear sides flip).
- Silhouette must read at 1× in a crowd: if you can't name the creature
  from its black shape alone against a mid-luma field, redesign the shape,
  not the colour.

---

## §2 The universal prompt system (bible §13, operational)

Every generation prompt is assembled as:

```
Full = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE
```

### 2.1 PREFIX (verbatim — do not alter; it is the style lock)

```
1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy,
muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green),
hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients,
matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background,
orthographic 2:1 isometric view
```

### 2.2 SUBJECT (per asset — pattern: `[Name], [2–3 defining shapes],
[2–3 texture/material beats], [pose/motion], [3/4 south-facing unless noted],
[single figure/creature/structure, feet/roots on ground]`)

Substance rules: name the *shapes* (teardrop, S-curve, wedge), not moods;
give the pose a verb with a frame (e.g. "bite lunge"); state the ground
contact explicitly; say "single creature" — double figures are a reject
(we crop them out, and crops lose the plate contract).

### 2.3 PALETTE (5–8 named colours, HEX, from the asset's family ramp)

Example (vermin family): `wet slate #4a4a50, mud #5a4a3a, grey-pink belly
#9a7f7e, bone teeth #cfc6b4, black ichor #141014, rust eye #7a2a2e`.
For terrain plates: name the zone ramp anchors + bleed colour. The family
ramp is the ceiling — the prompt may *use* ramp colours, never exceed 32.

### 2.4 NEGATIVE (verbatim baseline; extend per category)

```
anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render,
painterly character, bloom, lens flare, modern UI, text, watermark, logo, signature,
extra limbs, extra fingers, asymmetry errors in gear, elf ears, sparkle,
design-tool purple, checkerboard tiling, anti-aliasing, green background
```

Category extensions — mobs: `+ second creature, ground shadow painted in`;
terrain: `+ horizon, sky, fog wall, perspective distortion, repeating tile
seams`; furniture: `+ glass, gloss, plastic, modern wood grain`; NPCs:
`+ smiling heroically, brand new clothing, shine on armor`.

### 2.5 Plate protocol (generation → cell)

1. Generate **4× plates** named `<slug>_<DIR>_4x_raw.png` (S, SE, E native —
   three generations, same seed family; pose line "facing south" / "facing
   south-east" / "facing east"). Plain flat `#00FF00` key background.
2. SW/W/NW/N/NE via img2img from the S/E plates, **denoise ≤ 0.35**, pose
   guidance only — never flip the output.
3. `bhpix` chain: `key_out_green` → `harden_alpha` → `fit_to_cell`
   (NEAREST) → family `quantize` (Bayer-2) → `outline`.
4. Hand-fix list, in order: outline gaps (claws/teeth/tips) → weapon hand
   side across 8 dirs → feet on anchor → kill luma>200 strays → accent-hue
   audit.
5. Frames per category spec (§3–§9) → `cells/` → `bh_pack_sheet.py` →
   `sheet.png` + `sheet.json` (v1 schema, `anchorY` in every anim block) →
   `validate_atlas`.

### 2.6 Runs log (mandatory per `prompt.md`)

Every folder gets `prompt.md` with the assembled prompt and a **Runs**
section, one line per generation, appended verbatim:

```
- <DIR> · <date> · <provider/tool> · model <name or "undisclosed"> · seed <n|n/a> ·
  notes (md5-distinct? plate accepted y/n? what was reworded for moderation?) · accepted **y/n**
```

Write "undisclosed" when the provider hides the model — **never omit the
field**. Rejected/quarantined generations stay listed with `accepted **n**`
and the reason (the Ravager byte-identical batch, 2026-09-10, is the
standing example).

---

## §3 World & terrain (batches B1/B2 done · B11 castle open)

### 3.1 Zone contract (from `docs/art/10-terrain.md` — measured maps)

| Zone | Map | Ground ids in use | Edge pairs (measured) | Prism / footing |
|---|---|---|---|---|
| `town` (Thornwall) | thornwall | 0 GRASS · 1 DIRT · 3 WATER · 4 WOOD · 5 PATH · 6 MUD · 7 DARKGRASS | 7 pairs × 3 variants | nailed-plank palisade + stone footing skirt |
| `fields` | fields_overflow | 0 GRASS · 5 PATH · 6 MUD · 7 DARKGRASS | 4 pairs × 3 | dead-bramble hedge + skirt |
| `mine` (Bonehowl) | bonehowl_mine | 1 DIRT · 4 WOOD · 6 MUD · 7 DARKGRASS (+WALL) | 3 pairs × 3 | rock + timber brace, faces v0–v2 |
| `crypt_drowned` | drowned_crypt | 1 DIRT · 3 WATER · 4 WOOD · 6 MUD · 7 DARKGRASS | 5 pairs × 3 (DIRT↔WATER = hero edge) | none (no WALL) |
| `crypt_thornwall` | thornwall_crypt | 1 FLOOR · 2 WALL · 3 SLAB · 4 BONEPIT · 5 CANDLE | 3 pairs × 3 | catacomb masonry, faces v0–v2 |
| `castle` (Weeping) | — (B11) | to be measured from the siege map | — | weep-stained masonry + rusted portcullis |

Deliverables per zone: `raw/*_4x_raw.png` plates → `plates/` (512×256) →
`edges/<BASE>_<OVER>/v<k>/{edge_NE,edge_SE,edge_SW,edge_NW,corner_N,E,S,W}.png`
→ `prism/{top,left,right[,v<k>/],skirt_*}.png` → `palette_<zone>.png` (≤ 32)
→ `terrain.json` (T-ART-12 contract) → plate QA triptychs + seam audit.

### 3.2 Plate prompt template (terrain)

```
PREFIX_terrain = PREFIX (2.1) with "sprite" replaced by "painted ground plate"
                and "orthographic 2:1 isometric view" → "flat top-down 2:1 isometric
                ground texture, painted as one continuous 512×256 plate, 8×8 tile
                diamonds, no horizon, no perspective, wraps seamlessly"
SUBJECT = "<zone, id name>, <material beats>, <worn-by beats: hoof-prints, cart ruts,
          drip lines, algae, weep-stains>, <one allowed warmth accent if the zone has one>
PALETTE = "<zone ramp anchors 5–8 hex + bleed colour>"
NEGATIVE = baseline + "tiles, checkerboard, repeating pattern, horizon, sky, buildings,
           creatures, perspective, blurry water"
```

Worked example (town turf):
```
SUBJECT = "Thornwall town turf, dead-olive dead grass under hoof traffic,
trampled bare patches, faint cart-rut lines, two mole hills, one half-buried
bone at the graveyard edge, no light sources"
PALETTE = "dead olive #55583a, dark mud #4a3a2e, soot grey #55524e, bone #cfc6b4,
stagnant green #4a5a40, wet dark #26221c"
```

Rules: plate luma mean 45–70 / min ≥ 24; paint the *worst* lighting of the
zone (mid-afternoon overcast); water = dithered ripple, **no animation**
(2 frames max at night, and only if a later loader allows — ship static);
the DIRT↔WATER crypt edge (69 % of the drowned crypt) is the B2 hero —
flagstone stepping *into* still black water, one algae fringe, one submerged
bone at the edge.

### 3.3 Furniture-in-terrain

Blocking sprites with feet anchor, 1 px outline where they occlude bodies,
wire kind 64+ (§4). Non-blocking scatter: ≤ 64×64, alpha-cut, **no outline**
(litter, pools, moss tufts). Light pools: painted warm ground decals
96×48 / 128×64 under every warm source (anvil, candles, windows) — these are
the night-grade carry of the whole map.

### 3.4 Free-pack usage for world (binding defaults)

- **Blockout tier (placeholder, engine-loadable now):** the three
  `kenney_isometric_*` packs + `kenney_castle_kit/Isometric` +
  `kenney_medieval_rts_pack` may be dropped into blockout scenes and
  prototype maps as-is (re-tint to zone ramp first if luma is off).
  `kenney_roguelike` / `roguelike_cave` / `roguelike_indoor` sheets are the
  16×16 map-prototyping source (their sample `.tmx` maps work with
  `tools/mapgen` study).
- **Reference tier:** `kenney_graveyard_kit` (occlusion of dead trees,
  tombstone rhythm, fence line breaks) studies the DARKGRASS/gravyard
  plates; `kenney_castle_kit/Topdown` studies siege-map layout (wall turns,
  gate sightlines, tower spacing) before the Weeping Castle map is
  generated; `kenney_isometric_landscape` studies tree/rock silhouette
  cadence for fields.
- **Final tier:** generated per §3.2–3.3 + `bh_terrain.py` chain. Free-pack
  pixels never ship as final plates without full source-plate treatment
  (§10.3).

---

## §4 Furniture (wire kinds 64+; shipped 64–66, reserved band per REGISTRY)

### 4.1 Shipped & reserved

| Kind | What | Cell | Anims | Note |
|---|---|---|---|---|
| 64 vendor | Marta's stall | (stall = terrain furniture, 48×48) | idle2 | stall is a **terrain furniture**, the NPC is separate |
| 65 anvil | **The Widow Anvil** — iron block on oak stump | 48×48, a44 | idle2 (ember pulse) + `pool_128x64.png` | the anvil is *furniture, not an NPC*; ember in the coal trough = the one warm light of the square |
| 66 bounty | Bounty Board on two posts | 32×48, a42 | **state 2** (has-bounty / empty) | has-bounty = 3 notices, one blood-stained, top notice carries the quarry silhouette stencil (rat/gnoll/widow/gravecaller/bell); empty = nail holes + one torn corner |

Reserved (T-ART-06): 67 Bonesmith twins (stand beside the anvil — they are
NPCs, kind 67), 68 chapel furniture set, 69 cove crates/stall, 70–71 guard
quarters, 72 pledge vault, 73 castle steward's desk.

### 4.2 Furniture prompt template

```
SUBJECT = "<name>, <build/material beats: iron block on oak stump, plank board on
two posts>, <wear beats: rust bloom, nail heads, wax drips, weep-stain>,
<one functional beat: ember in coal trough, three pinned notices>,
standing on flat ground, single object, full object visible"
PALETTE = "<family ramp, ≤ 8 named hex; anvil: iron #3a3a40, coal #1a1214,
          ember #c8622a (allowed warmth), oak #5a4a3a, rust #7a2a1e>"
NEGATIVE = baseline + "glass, gloss, plastic, steam, smoke rising, modern fasteners"
```

Rules: ember/wax/candle are the *only* allowed warm pixels (≤ 3 % of the
cell); every warm piece ships its pool decal (96×48 or 128×64, painted,
warm radial, no alpha-blend); state-2 furniture ships 2 dir-rows identical
(8-row pack) until a state loader exists.

### 4.3 Free-pack usage for furniture

- `kenney_furniture_kit/Isometric` = the silhouette/occlusion reference for
  *every* piece (table height vs 32×48 body, chair back readability at 1×,
  shelf depth vs prism faces).
- `kenney_roguelike_indoor` (16×16, magenta-keyed) = placeholder interiors
  for chapel/inn/pledge-vault blockouts (re-key magenta with
  `bhpix.key_out_green(key='#FF00FF')`, quantize to zone ramp).
- `kenney_isometric_city` = street furniture (barrels, crates, market
  stalls, lamps) silhouette reference for town squares.
- Final pieces are generated per §4.2; a free-pack piece may ship as
  placeholder *only* if the task card says so.

---

## §5 Mobs (B3/B4 shipped · roster 12 common + 2 elites + 3 named + 1 boss)

### 5.1 Families & palettes (shared ramps, ≤ 32 each)

| Family | Members | Ramp anchors |
|---|---|---|
| vermin | Marsh Rat, Plague Bat, Old Maw | wet slate `#4a4a50`, mud `#5a4a3a`, grey-pink `#9a7f7e`, bone `#cfc6b4`, ichor `#141014` |
| ghoul-flesh | Feral Ghoul, Crypt Revenant, Mine Wretch, skeleton pet | grey-pink `#8e7776`, bruise `#5a4a5e`, rib bone `#cfc6b4`, dried blood `#3a080c`, rag `#4a4238` |
| hound | Hollow Hound, Pale Sow | hound-slate, pale hide, black ichor, rust eye |
| grave-goods | Bonepicker Gnoll, Revenant Sexton, Sepulcher Elite | bone, rusted iron, nailed leather, soot |
| widow | Charnel Widow, Lantern Spider, Red Widow | chitin black-brown, silk grey, **lantern = allowed warm accent** |
| choir-wax | Waxen Celebrant, Pale Cultist, Cantor Vex, Bell Ringer, Gravemother | wax ivory, hymn-gold (choir licence), soot, bell bronze |
| night | Wraith, Bloodfiend, Banshee | **only family with accent-ramp entries** (violet/arterial) |

### 5.2 Frame budget (the schedule-governing number)

- Common: 8 dirs × (walk 4 + attack 3 + die 3) = **80 frames** → 10 cols.
  Contact frame = index 1 of the 3 (0-based).
- Elite: + hurt 2 → **96 frames** → 12 cols (40×60 cell).
- Boss: walk 4 + attack 3 + cast 4 + hurt 2 + die 4 + summon 4 = 21 cols
  (64×64).
- Fly/spider kinds: hover 12 px above ground; shadow ellipse at 45; body
  bottom row ≈ 30.
- Walk styles: bob (humanoids) · glide (fly) · drag (grounder) · bell
  (boss, lower 16 px ≤ 6 colours, luma σ ≤ 22).
- Die always ends on a **corpse frame** (last die frame persists as ground
  decal) + gib spec (e.g. ghoul: torso burst = ribs + 3 chunks + head).

### 5.3 Shipped roster prompt lines (B3/B4 — regenerate only as upgrades)

See each `assets/aigen/mobs/<id>_<slug>/prompt.md` for the verbatim
assembled prompt + Runs. The ten shipped (1001 Marsh Rat · 1002 Feral Ghoul
· 1003 Hollow Hound · 1004 Plague Bat · 1005 Bonepicker Gnoll · 1006 Charnel
Widow · 1007 Waxen Celebrant · 1008 Revenant Sexton · 1009 Gravemother (boss)
· 1010 Sepulcher Elite) are offline-QA'd; in-engine validation = T-ART-01/05/10.

### 5.4 GDD roster to produce (MVP, `docs/02-gdd.md` §11)

| Mob | Zone | Silhouette beat | Family | Cell |
|---|---|---|---|---|
| Mine Wretch | Bonehowl Mine | lopsided crouch, one arm dragging a pick, ribs showing | ghoul-flesh | 32×48 |
| Lantern Spider | Bonehowl Mine | low body, 8 legs splayed, **lantern hung from the thorax (warm accent)** | widow | 32×48 (spider kind, drag) |
| Mud Golem | Bonehowl Mine | tall slab body of compacted mud, glowing seam eyes (ichor, *not* magic), broken timber in one fist | new ramp **mudstone** (define ≤ 32 before first plate) | 40×60 (elite-scale mass, common anims) |
| Crypt Revenant | Drowned Crypt | drowned soldier, flagstone-grey skin, half-dissolved tabard, pike splinter | ghoul-flesh | 32×48 |
| Grave Banshee (elite) | Drowned Crypt | tall thin bell-shaped shroud, no feet (hover), face = hollow bell mouth | night | 40×60, hover 12 |
| Bell Ringer (summoner elite) | Drowned Crypt | robed figure mid-swing of a **hand bell** (cast anim, not attack), bell bronze + soot | choir-wax | 40×60 |
| Old Maw (named) | Fields/night | the marsh itself opens: a mouth-shaped mound with fangs, low horizontal | vermin | 40×60, drag, walk = undulate |
| Pale Sow (named) | Fields | gaunt sow, ribs like a harp, foaming bit, one blind eye | hound | 32×48 |
| Wraith (named, night) | Night-only | drifting shroud, no contact shadow (night family may glow violet at the hem) | night | 32×48, glide, hover 8 |

Rules: silhouette test vs the *nearest* existing shape (the Banshee must not
read as the Celebrant at 1× — the bell mouth is the tell); night-only mobs
get a night cue *and* a day-plate that still passes R-LUMA; summoner elites
carry the summon anim (4f) even in MVP if the boss uses them.

### 5.5 Mob prompt template + hand-fix order

Template = §2 with SUBJECT per the row above; then hand-fix in the §2.5(4)
order, then: eyes glint check (only `night` family + hit-flash may glint;
the Marsh Rat's eyes **do not** — pathetic by design), gib decal frame,
night re-render vs zone plate.

### 5.6 Free-pack usage for mobs

- `kenney_animal_pack` (80 animals) = gait + silhouette reference for the
  vermin/hound family (rat tail whip, hound head-down, sow mass).
- `kenney_topdown_shooter_pixel` (incl. zombies) = placeholder enemies for
  soak-bot scenes and netcode drills (engine-loadable as-is, re-tinted).
- `kenney_rpg_pack` (32×32) = the closest cell-size match: use it for
  **era-mood** study — how 1999 mobs read at 32 px (rim light, one accent,
  readable arm swing) before any final plate.
- `kenney_medieval_rts_pack` = unit-spacing/crowd-density study for the
  crowd-test boards.
- Finals are always generated per §2/§5; free-pack mobs are never finals
  without full source-plate treatment + re-silhouetting (their 16×32/32×32
  top-down geometry does not fit the 32×48 iso cell).

---

## §6 NPCs (B5 placeholders shipped · finals open; 8 NPCs, 3 palette families)

Palette families: **npc-ash** (Marta, Confessor, Ashen guards, Steward) ·
**npc-synod** (Cove fence, Synod guards, Registrar) · **npc-ember**
(Bonesmith twins — the one warm family; ember rim light on veil edges is
permitted).

Sheet: idle 4f × 8 dirs = 32 frames → 128×384 (twins: one 64×48 cell holding
both, so they never desync). Portrait: 96×96 painterly bust (see §6.2).
Every NPC ships a one-line **bark** (the dialog opener) in the brief.

### 6.1 Sprite briefs (from `docs/art/30-npcs-players.md`)

1. **Marta** (vendor, kind 64) — broad flour-dusted apron over ash-grey
   wool, key ring + tally-sticks, rolled sleeves, forearms like a smith's;
   idle = wipes hands. Two portraits: neutral (tired half-smile) and
   **sneer** (karmaBand==2 — portrait swap, not sprite swap).
2. **The Bonesmith twins** (kind 67) — soot-veiled sisters flanking the
   anvil; left holds a hammer, right holds tongs; ember rim on veil edges;
   idle: hammer taps / tongs turn a bar. One portrait, both faces
   half-lit by coals, eyes different (one hollow, one bright).
3. **Chapel Confessor** (68) — ash-robe, nail-through-leaf sigil on the
   stole, nailed-plank prayer board under one arm; idle = turns a page.
4. **Bounty Board** (66) — furniture, §4.
5. **Smugglers' Cove fence** (69) — hooded, wax-seal ledger, dockside
   grime; idle = seals a page (tiny warm wax drip — the one warm pixel).
6. **Town guards ×2** (70 Ashen / 71 Synod) — same base body, faction
   tabard/sigil/weapon differ; shoulders 14 px, weapons at +15 % (they are
   the chaotic-hunting threat); portraits: helmets on, sigils are the faces.
7. **Pledge Registrar** (72) — zealot clerk, bone-white robe,
   **incense-gold ink stains** (choir-gold licence: blessing ink); idle =
   dips quill.
8. **Castle Steward** (73) — neutral grey robes with a **weep-stain motif**
   (vertical dark streaks from the shoulders), key on a chain, no sigil;
   idle = polishes the key.

Prompt: `PREFIX + "<name>, <one-line above>, 3/4 south-facing, standing
idle, full body, feet on ground plane, single figure, era MMORPG NPC" +
NEGATIVE + "smiling heroically, brand new clothing, shine on armor"`.

### 6.2 Portrait prompt (verbatim pattern)

```
"painterly bust portrait, 1999 Korean MMORPG NPC dialog portrait, dark horror,
single candle key light (warm key, cold fill), muted palette,
<face brief — gaunt, kind, soot on the fingers; a bell rope visible behind>,
bust to mid-chest, 3/4 view, ornate silver frame NOT included, plain dark
background" + NEGATIVE
```

≤ 32 colours after quantize (it's *paint* at 1× in the frame — paint
grain, not pixel steps).

### 6.3 Free-pack usage for NPCs

- `kenney_blocky_characters` (v2: face sprites + skin colour-variant
  textures) = placeholder *crowd colour study* + face-sprite placeholder
  (the pre-v2 16×16 4-dir animated sheet is the era-cadence reference if
  ever re-ingested — see the pack CREDITS note).
- `kenney_modular_characters` (425 parts) = paper-doll *study*: measure the
  trim-% rule (faction trim ≤ 8 % of pixels) against their colour-variant
  system; use as placeholder crowd variation until B14.
- `kenney_rpg_pack` (32×32 heroes) = silhouette + armour-readability study
  for the guard tabards and the player class overlays.
- Finals are generated per §6.1–6.2 with the `b5_npc_proxy.py` chain as the
  fallback (the current B5 sheets are procedural placeholders and are
  overwritten in-place — no loader changes).

---

## §7 Players (B6 in progress; 3 classes × m/f × 3 skin tones)

Paper-doll layers (MVP = tint/attach): **base body (m/f) → skin tone →
class overlay (robe/armour) → weapon attach → trim colour** (faction/party
≤ 8 % of pixels; karma never changes the sprite).

Cells 32×48, anchorY 42, body rows 0–42. Anims per (class, sex):
idle 1 · walk 6 · attack 3 · cast 4 · hurt 2 · die 4 · gib 3 = **23 cols ×
8 rows = 736×384**. Skin tones: pale `#d6c6be`, sallow `#b8a48a`,
weathered-tan `#8e6e52` (D7: bake ×3 sheets per (class, sex) = 18 sheets;
the A3 index maps stay in `palettes/families.json`).

Class signatures (silhouette first):

| Class | Read at 1× | Attack tell | Cast |
|---|---|---|---|
| **Ravager** | bulky, cleaver in right hand, no helm | attack f1 = cleaver overhead (widest frame of the set) | ch5 firebolt is a *thrown knife* in lore, ember comet in art |
| **Gravecaller** | robed, **bell-staff** in right hand, hood up | attack = staff slam with dust | bell-light halo, hymn-glyphs |
| **Cultist (Pale Choir)** | white robe, **open hymn book**, no visible weapon | attack = hymn-strike (no physical swing — ring + glyphs) | choir-light column, mass mend |

Prompt: §2 pattern, SUBJECT = `<class> <sex> base, <signature line>,
<trim line: faction tabard/robes in <family> trim ≤ 8%>, 3/4 south-facing,
full body, feet on ground, single figure, era MMORPG player character`.
Plate order per (class, sex): walk f0 (S/SE/E) → die f3 (S/SE/E) →
attack f1 (SE/E) → cast f2 (SE/E) — then the `bh_pack_sheet.py`
`--pad-missing` WIP sheet, completed when the full set is generated.
Right-hand weapon verified at *accept* (the Ravager m/f batch, 2026-09-10,
is the reference; the earlier byte-identical batch was rejected — see
`prompt.md` Runs).

Free-pack usage: `kenney_rpg_pack` 32×32 heroes = era-mood reference only
(era mismatch: their palette is bright — use geometry/timing, never colour).
Player finals are **never** free-pack-derived (they are the identity of the
game).

---

## §8 Items, icons, in-hand attachments

- Item icons: one sheet, **32×32 + 24×24** per item (`icons/items/<id>_<slug>`);
  language: 1 px outline, ≤ 32 colours, one accent pixel budget (the rust
  on the Rusty Shank, the fill state on the Blood Vial — 3 fill states),
  black-iron coin for gold (F15: *not* yellow).
- In-hand weapons: `players/_weapons/<id>/` — 8 dirs × refinement states
  +0/+5/+9/+10 (the +9/+10 states carry the gamble-enhancement glow:
  choir-gold at +10, arterial at +9 — the only UI-visible enhancement
  colours).
- Skill icons: `icons/skills/ch<1..8>_<slug>` for shipped channels
  (power_swing, mend, bless, ironskin, firebolt, chorus, mass_mend, haste);
  one glyph per skill, readable at 16 px.
- UI: karma badges 3 × 6×6, buff strip 9 × 16×16, callout font (D4: 7×11
  red-caps) — font ships as strips + JSON (`bhfont.py`).
- Prompt: `PREFIX with sprite→icon + "single object centered, 32×32, no
  ground shadow" + <object beats> + <state line>`; refine-state variants
  are generated as img2img from the +0 icon at denoise ≤ 0.25 (state
  differences must be glow/edge only — never reshape the object).
- Free-pack usage: `kenney_weapon_pack` (2D renders + silhouettes of a
  *modern/shooter* weapon set) = generic small-size shape/silhouette
  readability study only (mass distribution, grip-vs-weapon legibility at
  16–32 px) — never for weapon-set content (our set is medieval);
  `kenney_roguelike` (16×16 items) = icon *readability* study at tiny size. Finals are
  generated; the weapon pack ships no pixel into finals (its renders are
  3D-shaded, wrong era).

---

## §9 VFX (31 FX, `docs/art/50-vfx.md`)

Rules (Helbreath readability rulebook R-FX): **≤ 0.6 s** hit FX; persistent
results are **ground decals** (blood pools, rot); FX covers **≤ 40 %** of
the caster body; reserved ramp on magic only. Colour language: iron/red
(swing) · ash-blue (cast) · bone-white (hit) · gold (bless/choir) · violet
(curse/night).

Frames: 8×8 – 64×64, listed `size × frames @ fps` in the 50-vfx table.
Loader reality (D8): `dirs:1` strips are **not** accepted by `loadAtlas` —
every VFX ships **both** as `strip.png` (1 row) and as `sheet.png` with 8
identical rows until the strip loader lands. Folder =
`vfx/<group>/<nn>_<slug>/` (nn from the 50-vfx table).

Prompt: `PREFIX with sprite→"sprite strip frame" + <FX beats from the 50-vfx
row> + "on plain flat #00FF00 background, single frame, centered, no
shadow"`; generate the f1 (contact/full) frame first, derive f0/f2 by
pixel ops (scale 0.75/0.5 + alpha drop) — only re-generate if the derived
frame fails the silhouette check. Callouts ("Power-Swing!") use the red-caps
font, never a generated font.

Free-pack usage: `kenney_particle_pack` (80 particles, 512×512) =
placeholder FX sprites (engine-loadable, re-tinted to the colour language)
+ frame-shape/timing reference for B13. `smoke/dust` pieces may ship as
placeholder; *magic* FX are always generated (the reserved ramp is a
contract).

---

## §10 Free base packs — `assets/free/` (ingested 2026-09-17, CC0)

**Inventory (17 packs, 35 MB, Kenney, CC0 1.0 — provenance in each pack's
`CREDITS.md` + `assets/LICENSES.md` "Free packs" section):**

| Category | Packs |
|---|---|
| world (2:1 iso) | `kenney_isometric_buildings` · `kenney_isometric_landscape` · `kenney_isometric_city` · `kenney_castle_kit` (iso+topdown) · `kenney_medieval_rts_pack` |
| world (16×16 topo) | `kenney_roguelike` · `kenney_roguelike_cave` · `kenney_roguelike_indoor` |
| world (tone) | `kenney_graveyard_kit` (iso+side) |
| furniture | `kenney_furniture_kit` (iso+side) · `kenney_roguelike_indoor` |
| mobs | `kenney_animal_pack` · `kenney_topdown_shooter_pixel` · `kenney_rpg_pack` · `kenney_medieval_rts_pack` |
| NPCs | `kenney_blocky_characters` · `kenney_modular_characters` |
| VFX | `kenney_particle_pack` |
| items | `kenney_weapon_pack` |

### 10.1 Why these packs

They cover, in order of fit to BLOODHOLLOW: the **only** CC0 true 2:1
isometric tile series (the iso trio — same projection as the engine's
`iso::drawPrism`), a castle + graveyard kit for the two signature sites
(Weeping Castle, the town graveyard), a 32×32 medieval RPG pack (closest
cell size to our 32×48 cells → primary *era-mood* reference), 16×16
top-down sheets with sample maps (map prototyping + placeholder
interiors/dungeons), character face + skin colour-variant sets (placeholder
crowd colour study), a particle set (placeholder FX), and a 2D weapon
render set (generic silhouette-reading reference). Every pack is
public-domain, so placeholder use in released
builds is legally safe; the *art-direction* constraints below are binding
regardless.

### 10.2 Usage tiers (repeat of `assets/free/README.md` — binding)

1. **Placeholder tier** — engine-loadable as-is (re-tint to family/zone
   ramp first if luma is off): blockout scenes, prototype maps, soak-bot
   enemies/crowds, placeholder FX, pre-release UI. Task cards must name
   which placeholder ships where.
2. **Reference tier** — study only, **never trace**: silhouette, palette,
   composition, occlusion, frame cadence, spacing. Measure and
   re-interpret; never pixel-copy.
3. **Final tier** — only pipeline-generated material (or a free-pack sprite
   treated as a *source plate*: re-keyed, fitted to cell, colour-matched +
   quantized to the family ramp, outlined, passed every §12 gate, with the
   pack cited in `derivation.json` + `assets/LICENSES.md`).

### 10.3 Hard rules for free packs

- Keep `License.txt` + `CREDITS.md` in every pack folder (already done).
- Any pack file that enters an `assets/aigen/` delivery folder is a
  **source-plate event**: it needs the full §2.5 chain + a LICENSES row
  naming the pack.
- No free-pack pixel may be *traced* (redrawn pixel-for-pixel by a
  generator "using this as reference" at denoise > 0.35): re-interpret the
  *silhouette*, generate the pixel.
- The ancestor-game no-trace rule (bible §16) is untouched: it governs
  Dark Eden / Lineage 1 / Helbreath / Mir 2 / Soma art specifically; these
  CC0 packs are the approved non-ancestor source for placeholders and
  study.
- `assets/final/` is still forbidden for everything (AGENTS.md).

---

## §11 Batch plan (this prompt's execution order)

| Batch | Content | Inputs | Exit gate |
|---|---|---|---|
| **B10** | Free-pack triage drop: re-tint the iso trio + medieval-rts to the town/fields zone ramps; build 2 placeholder blockout scenes (Thornwall square, drowned-crypt entry) + 1 placeholder crowd (8 blocky chars) + placeholder FX set (particle pack re-tinted) | §10.2 tier 1 | scenes render headless via `bh_probe_leg.sh`; every used file has a LICENSES row (done) |
| **B11** | Weeping Castle terrain: plate list + measured pairs from the siege map, weep-stain masonry prism (3 faces), portcullis furniture, banner-pole state set (3 pledges) | §3, `kenney_castle_kit` (ref), B1/B2 chain | `b11_build.sh` rc 0; seam audit clean; night triptychs |
| **B12** | Item + skill icon set (shipped ids + 8 skill glyphs), black-iron coin, karma badges, buff strip | §8, `kenney_weapon_pack` (ref) | every icon legible at 16 px on parchment; ≤ 32 colours |
| **B13** | VFX 1–31 per 50-vfx table (strip + 8-row sheet each) | §9, `kenney_particle_pack` (placeholder until done) | R-FX gates: ≤ 0.6 s, ≤ 40 % body, reserved ramp audit |
| **B14** | NPC finals: 8 NPCs × (sheet + portrait[, ×2]) overwriting the B5 procedural placeholders in-place | §6, `kenney_blocky_characters`/`modular` (ref) | `bh_qa_sheet.py --kind sheet` pass × 8; portrait ≤ 32 colours |
| **B15** | Player completion: Gravecaller m/f + Cultist m/f full 23-col sets (Ravager WIP finished) | §7 | feet-on-anchor + R-LUMA remediation of the B6 fail (T-142) |
| **B16** | Furniture finals: anvil pool set, bounty states, chapel/cove/vault/castle sets | §4, `kenney_furniture_kit` (ref) | warm-pixel % audit ≤ 3 %; pools present |

Sequencing rationale: B10 unblocks *every other batch* (placeholder scenes
exist for in-engine validation while finals land) — it is the payoff of
`assets/free/`. B11 is the last unseen zone; B12/B13 are small and
de-risk the UI; B14/B15/B16 are the volume work.

---

## §12 QA gates (an asset is not done until all pass)

Run via `tools/atlaspack/` (see `assets/aigen/_pipeline/README.md`):

1. **R-LUMA** — body (excl. outline) Δ vs its zone plate: **day ≥ 25,
   night (02:00) ≥ 15** (`bh_qa_sheet.py`). The B6 Ravager fail (Δ 10.7
   day) is the standing warning: muddy mid-tones are the #1 reject.
2. **Colour count** — ≤ 32 opaque per family/zone; palette strip saved
   (`palette_<family>.png` / `palette_<zone>.png`).
3. **Feet-on-anchor** — lowest opaque body row == anchorY in every frame
   (42/44/52/58 per cell; fly kinds: shadow at 45, body bottom ≈ 30).
4. **Outline closure** — no gap at claws/teeth/tail/weapon/staff tips;
   1 px, `#1a1214`.
5. **Luma ceiling** — no pixel > 200 except light source / eye glint /
   hit-flash / bone.
6. **Accent-hue audit** — violet/arterial/choir-gold only where §1.2
   allows.
7. **Silhouette test** — black shape vs nearest existing shape at 1×;
   name the creature from the shape alone.
8. **Night triptych** — day | night 02:00 | greyscale archived to
   `docs/research-notes/qa/` with the `_audit.json`.
9. **Loader replica** — `validate_atlas` (sheet.json v1, anchorY present,
   one PNG per atlas, 8 dir rows in E,SE,S,SW,W,NW,N,NE order).
10. **Terrain only** — seam audit (edge pieces on the base tile, no
    checkerboard), WALL-never-bleeds check, prism footing present,
    plate luma mean 45–70 / min ≥ 24.
11. **Provenance** — `prompt.md` Runs complete (model field never empty),
    `assets/LICENSES.md` row written, pack cited if any free-pack source
    plate was used.

Any failed gate = `exit 1` in the build script; the asset goes back to
§2.5(4) hand-fixes — **never** re-quantize with a looser ramp to pass.

---

## §13 Delivery & provenance contract

Per delivery folder (id-keyed, never slug-keyed lookups):

```
<category>/<id>_<slug>/
  sheet.png            # one PNG, packed (bh_pack_sheet.py)
  sheet.json           # v1 schema, anchorY in every anim block
  palette.png          # the family/zone strip actually used
  prompt.md            # assembled prompt + Runs (model field mandatory)
  derivation.json      # which dirs/frames are derived, from what, at what denoise
  pack_report.json     # builder output
  plates/              # 4× raws: <slug>_<DIR>_4x_raw.png
```

- Mobs: `mobs/<id>_<slug>/` · NPCs: `npcs/<slug>/` · players:
  `players/<class>/{m,f}/` (+ `players/_weapons/<id>/`) · terrain:
  `terrain/<zone>/{raw,plates,edges,prism,scatter,pools,palette_*.png,
  terrain.json}` · icons: `icons/{items,skills,ui}/...` · VFX:
  `vfx/<group>/<nn>_<slug>/{strip.png,sheet.png}`.
- One batch = one drop = one LICENSES.md block = one QA triptych set.
- `assets/aigen/` for AI-gen + free-pack-derived · `assets/final/` is
  human-art-only and **never touched** · placeholders marked in the task
  card.
- Sheet-size caps are hard (loader + GPU budget): 320×384 / 480×480 /
  1344×512 / 736×384 / 128×384 — split into two PNGs rather than exceed
  (boss precedent: 1344×512 single file is the ceiling).

---

## §14 Accept / reject checklist (paste into every task card)

**Auto-reject (any one = reject, record reason in prompt.md Runs):**

- [ ] anti-aliasing or gradient anywhere (edge inspection at 4×)
- [ ] luma > 200 outside the §1.2 exceptions
- [ ] accent hue (violet/arterial/choir-gold) on skin/cloth/terrain gear
- [ ] weapon/staff in the wrong hand in any dir; any mirrored output
- [ ] feet not on anchorY in any frame; painted floating shadow
- [ ] > 32 opaque colours; palette strip missing
- [ ] outline gap at any extremity
- [ ] second creature/figure in a single-subject plate
- [ ] text, watermark, signature, logo, design-tool purple
- [ ] extra limbs / fingers / asymmetry errors in gear
- [ ] R-LUMA gate fail (day < 25 or night < 15)
- [ ] silhouette collision with the nearest existing shape
- [ ] provenance gap (Runs model field empty; LICENSES row missing)

**Accept (all):**

- [ ] §12 gates 1–11 pass (build script rc 0)
- [ ] prompt.md Runs line present with provider + model + date + accept flag
- [ ] night triptych archived under `docs/research-notes/qa/`
- [ ] REGISTRY.md row matches (id, cell, anchorY, anims, sheet px)
- [ ] task card updated: placeholder? in-engine validation status?

---

## §15 Hard rules (condensed from AGENTS.md + bible — violation stops the batch)

1. `assets/final/` is human-art-only. Never touch it.
2. No tracing/ripping/upsampling of **ancestor-game** art (DE/L1/HB/Mir2/
   Soma) — ever. Free CC0 packs are the approved placeholder/reference
   source (§10).
3. No new third-party dependency without an ADR (assets included: a new
   *engine-loaded* asset source needs a LICENSES row minimum; a new
   *format* needs an ADR).
4. The server never trusts the client — art must not encode gameplay state
   in pixels (karma changes never change sprites; enhancement glow is the
   one sanctioned exception, §8).
5. Determinism and code rules of AGENTS.md are unchanged by this prompt —
   this prompt governs pixels, not C++.
6. When in doubt between this document and a shipped spec
   (`REGISTRY.md`, `docs/art/*.md`, `BRIEF.md` files): **the shipped spec
   wins**; file the conflict in the batch record.
7. Batch discipline: one drop per batch, one LICENSES block, one QA set,
   build script exit 0 — then and only then does the folder ship.
