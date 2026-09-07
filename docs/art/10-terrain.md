# Terrain & tileset briefs (bible §6, Soma lock) — B1/B2 production spec

All numbers verified against `data/maps-src/*.tmj` (tile histograms and
adjacency pairs computed 2026-09-07) so we paint **only the transitions the
maps actually contain**.

## Anatomy per tileset (amended from B0 research)

| Layer | Spec | Why |
|---|---|---|
| Ground plate | one painted **512×256 px plate per ground id** (= 8×8 diamonds), cut at screen position by `bhpix.cut_diamond` (plate coords = world coords mod plate) | Soma/Mir paint-then-cut; kills the random-tile checkerboard seen in style-tile pass 1 |
| Edge tiles | hand-painted 64×32 diamonds per adjacency pair below, 4 orientations + 4 corners = **8 per pair** | §6.6: no alpha-blend overlays |
| Height (id 2 WALL) | keep `iso::drawPrism` (28 px) **but** skin it: top 64×32 + left/right faces 32×28 painted, per tileset | engine draws walls as prisms today; a skinned prism is the cheapest true upgrade |
| Scatter | non-blocking decals, ≤ 64×64, alpha-cut, no outline | lurking pools, litter |
| Furniture | blocking sprites with feet anchor, 1 px outline where they occlude bodies | wire kinds 64+ |
| Light pools | painted warm ground decals 96×48 / 128×64 under every warm source | R-NIGHT (no engine light mask yet) |
| Palette strip | ≤ 32 colours per **tileset family** (`palette_<family>.png`) | §6.6 QA |

Per-plate luma rule (from the rulebook): mean 45–70, min ≥ 24 (above outline
`#1a1214`), contrast ≤ 0.75 of the raw plate. Every plate is rendered through
`bhpix.night_floor(hour=2)` and archived in `docs/research-notes/qa/`.

## Terrain-id → family mapping (from the generators)

Standard maps (thornwall, fields, mine, drowned_crypt) share ids; each tileset
supplies its own plate per id it uses:

| id | name | thornwall (3072) | fields (3072) | mine (2240) | drowned_crypt (1728) |
|---|---|---|---|---|---|
| 0 | GRASS | 68 % dead-olive turf | 71 % rotten furrow turf | 1 % (lichen) | 1 % (algae ledge) |
| 1 | DIRT | 3 % trampled square | — | 24 % **slick cave floor** | 28 % **causeway flagstone** |
| 2 | WALL (prism) | 8 % nailed plank + stone footing | 13 % hedge/root wall | 74 % rock + timber brace | — |
| 3 | WATER | 8 % Redwater river | — | — | 69 % **still black crypt water** |
| 4 | WOOD | chapel floor | — | brace planks | rotted gangway |
| 5 | PATH | 4 % cobble | 3.5 % cart track | — | — |
| 6 | MUD | 6 % lane mud | 5 % field mud | 1 % drip mud | 1.5 % silt |
| 7 | DARKGRASS | 3 % graveyard | 7 % shadow turf | 1 % | 1 % |

thornwall_crypt (1728 tiles): 0 STONE (unused base) · 1 FLOOR 42 % worn
flag · 2 WALL 57 % catacomb masonry (prism) · 3 SLAB (3) sarcophagus lids ·
4 BONEPIT (10) · 5 CANDLE (6) votive clusters = **the** warm pools.

## Transition tiles actually needed (measured adjacency, ≥ 4 occurrences)

- **Town:** GRASS↔WALL 248 · GRASS↔PATH 119 · GRASS↔WATER 118 · GRASS↔MUD 67 ·
  GRASS↔DARKGRASS 52 · DIRT↔WALL 31 · GRASS↔DIRT 30 · WALL↔DARKGRASS 14 ·
  WALL↔WOOD 12 · PATH↔DARKGRASS 9 → **7 painted pairs** (WALL pairs are the
  prism footing, one "footing skirt" tile handles all WALL↔X).
- **Fields:** GRASS↔WALL 614 (hedge) · GRASS↔PATH 116 · GRASS↔DARKGRASS 80 ·
  GRASS↔MUD 52 · WALL↔PATH 38 → **4 pairs** + hedge footing.
- **Mine:** DIRT↔WALL 210 · DIRT↔MUD 20 · DIRT↔DARKGRASS 20 · DIRT↔WOOD 6 →
  **3 pairs** + rock footing.
- **Drowned crypt:** DIRT↔WATER **245** (the whole map is this edge) ·
  DIRT↔WOOD 31 · DIRT↔MUD 30 · DIRT↔DARKGRASS 22 · WATER↔MUD 10 → **5 pairs**;
  the flagstone-into-black-water edge is the hero transition of B2.
- **Thornwall crypt:** FLOOR↔WALL 212 · FLOOR↔BONEPIT 34 · FLOOR↔CANDLE 24 ·
  FLOOR↔SLAB 9 → **3 pairs** + masonry footing.

Total: 22 edge pairs × 8 = 176 edge diamonds + 5 prism skins + 14 plates.

## 6.1 Thornwall — `terrain/town/`

- **Plates:** turf (dead olive, hoof-trampled), plaza dirt (ash-grey, cart
  ruts), cobble path (wet, soot-stained), lane mud (black, plank-stepped),
  graveyard dark turf (mole hills, one bone), river (black-green, dithered
  ripple, *no animation*).
- **Prism skin:** nailed-plank palisade on fieldstone footing, weep-stains,
  one **nail-through-leaf** sigil stencil every 6th face.
- **Furniture:** Widow Anvil (kind 65 — iron block on oak stump, **ember glow
  in the coal trough = the one warm light**, 48×48 + 96×48 pool decal) ·
  Marta's stall (kind 64 — plank counter, tally-sticks, flour sacks, 64×56) ·
  Wanted Board (kind 66 — 2 states has-bounty/empty, 32×48) · well · bell
  post (chapel; bell is bronze-green, rope frayed) · gallows pit rim (4 tiles,
  no rope swinging — "quietly dreadful") · gravestones ×6 (leaning, 3 with
  bell motifs) · market crates.
- **Scatter:** rat droppings, straw, broken cart wheel, soot smears, puddles,
  hanging laundry line (2 states), rusted nails.
- **Palette anchor:** ash `#5a5652` · soot `#2b2628` · iron `#4a4e58` · mud
  `#3a2f26` · bone `#c9bfae` · ember (anvil only) `#c8622a`.

## 6.2 Fields of the Overflow — `terrain/fields/`

- **Plates:** furrow turf (the B0 plate, re-prompted with "furrows at 26.6°"),
  black field mud, cart track, shadow turf; fence-line root wall skin for id 2.
- **Scatter (day):** collapsed fence posts, rotten pumpkins, scarecrow husk
  ×2 (one with a bell), root clusters ×3, plough blade, drowned cart.
- **Scatter (night-only variants, §6.2):** pale mushrooms ring, wisp glint
  (single 4 px bone-white point, *not* additive), open grave.
- **Old Maw den:** 3-tile gnawed burrow mouth with rat bones (named-elite
  anchor decal).
- Same tiles must read calm-by-day / wrong-by-night — proven by the style
  tile: no night-specific palette, the overlay does it.

## 6.3 Bonehowl Mine — `terrain/mine/`

- **Plates:** slick cave floor (wet sheen strokes, iron-blue glints), drip
  mud, lichen ledge; **rock + timber brace** prism skin (74 % of the map is
  wall — this skin is the zone's face; 3 variants so the tunnel walls don't
  strobe).
- **Furniture:** Blackiron ore node (3 growth states, dull iron-blue `#5c6a7e`
  glint — **not** an accent hue), lantern hook (with pool decal), mine cart,
  brace collapse, bone-pile midden (worked into wall), Red Widow web sheet
  (chamber-wide 4-tile decal + egg sacs).
- **Palette anchor:** wet slate `#3c4048` · iron-blue `#5c6a7e` · timber
  `#5a4630` · bone `#c9bfae` · lantern `#c8924a`.

## 6.4 Drowned Crypt + Thornwall Crypt — `terrain/crypt/`

- **Drowned:** causeway flagstone plate (algae edge), **still black water**
  plate (`#0c0e16` with 2-colour dithered ripple, painted column reflections
  where pillars stand), silt; the DIRT↔WATER edge set is hand-painted with
  submerged steps. Boss nave: round painted plate 8×8 with a **great cracked
  bell** centrepiece (blocking, 128×128, feet-anchored), one arterial-red
  runner from the door to the bell (DE licence: it's *cloth*, the only
  saturated terrain pixels in the game — flag for director).
- **Thornwall crypt:** worn flag plate, catacomb masonry prism skin (skull
  niches every 4th face), sarcophagus slabs ×3, bone pit (4 states of
  disturbance), **candle cluster** (id 5) = 6 tiles of warm pool; bell ropes
  hanging into dark (scatter, 2 lengths); rot-bloom decals.
- **Cantor Vex chapel:** side-chapel dressing = a bell rack + lectern.

## 6.5 Weeping Castle kit — `terrain/castle/` (design now, ship post-MVP)

Grey weep-stained ashlar plate; wall-walk plate; gate (3 states: intact,
breached 50 %, rubble) as a 2-tile prism skin set; Heartstone (64×96, dull
grey until pledge-recoloured); throne dais (3 tiles, 1 step); banner poles
with a **recolour mask channel** (PNG alpha = paint, separate `_mask.png` =
tint area) so ownership recolours at runtime; sconces every 4 tiles.

## Generation prompts (per plate)

`PREFIX_TERRAIN` = "pre-rendered painterly ground texture, Myth of Soma 2001
isometric MMORPG style, dark horror dark-fantasy, top-down orthographic,
seamless tileable, even overcast light, muted desaturated palette only (mud
brown, stagnant green, soot grey, bone off-white, black-blue water), no
objects, no characters, no text, no border, luma range 30–140" +
`NEGATIVE` = "bright, saturated, neon, lens flare, bloom, 3D render, photo,
blur, watermark, text, gradient sky, vignette".

| plate | subject line |
|---|---|
| town turf | "dead olive turf trampled by hooves, ash dust, a few bone-white pebbles" |
| plaza dirt | "packed ash-grey dirt with cart ruts and soot smears" |
| cobble | "wet soot-stained cobblestones, mortar dark, one rust stain" |
| lane mud | "black sucking mud with sunk plank steps, boot prints" |
| graveyard turf | "dark mole-hilled turf, one exposed rib, grey moss" |
| river | "still black-green river water, painted highlight strokes only" |
| furrow turf | "waterlogged rotten plough furrows at 26.6 degrees matching a 2:1 isometric grid, sickly grass tufts, straw" |
| field mud | "black field mud with standing puddles and drowned stalks" |
| cave floor | "slick wet slate cave floor, faint iron-blue mineral glints, drip rings" |
| causeway flag | "old cathedral flagstones with algae at the edges, cracked, bone dust in the joints" |
| crypt water | "still black crypt water, near-flat, faint painted column reflections, no waves" |
| worn flag | "worn catacomb flagstones, candle-wax drips, rot bloom in corners" |
| castle ashlar | "grey weep-stained ashlar stone, vertical mineral streaks, moss in joints" |

Prism skins and furniture use the object skeleton from bible §13 with the
Soma prefix and a `#00FF00` plate; palette per family is built with
`bhpix.build_palette` over the whole family before quantizing anything.
