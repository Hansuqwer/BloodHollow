# assets/aigen — asset registry (lookup contract for T-ART-05 / 06 / 10)

**Status:** contract draft, 2026-09-07 (B0.5 A17). Nothing listed here exists
as art yet; the table freezes *paths, cells and anchors* so the client's
`atlasFor(wireKind)` table and the art drops cannot drift apart.
Sources: `shared/content/mobs.h` (ids/names verbatim), `shared/content/wirekind.h`
(64/65/66), `server/src/world.cpp:186–197` (mob wireKind = 1-based `kMobs`
index), `engine/assets/atlas.cpp` (v1 schema; `anchorY` is **not read yet** —
T-ART-10), `docs/art/20-mobs.md`, `30-npcs-players.md`, `40-items-icons.md`.

## Conventions

- **Folder key = content id** (`mobs/<id>_<slug>/`). A rename in `mobs.h`
  (e.g. F2, 1007) changes `name` only; the folder keeps the id and the slug is
  cosmetic. Client lookups must key on **id / wireKind**, never on slug.
- Every delivery folder: `sheet.png` + `sheet.json` (v1, one PNG, per-anim
  offset blocks, rows = 8 dirs E,SE,S,SW,W,NW,N,NE) + `prompt.md` + `palette.png`.
- `anchorY` (feet row) is written into every anim block **now**; loaders
  that don't read it default to 42 (correct for all 32×48 cells).
- Skin-tone / class-tint variants: `sheet.png` is the base; `tone_<n>.png`
  are palette index maps (if the palette-swap shader lands) **or** baked
  `sheet_<tone>.png` (if not) — decided at B6 (D7).
- wireKind → mob id: `wireKind = index_in_kMobs + 1` (1001 → 1 … 1010 → 10).
  Furniture band starts at 64 (`kWireKindFurnitureFloor`); NPC kinds 67–73
  are **reserved here, not shipped** (T-ART-06).

## Mobs (shipped `kMobs`, ground truth)

| wireKind | id | name (mobs.h) | folder | cell | anchorY | anims (cols) | sheet px | family |
|---|---|---|---|---|---|---|---|---|
| 1 | 1001 | Marsh Rat | `mobs/1001_marsh_rat/` | 32×48 | 42 | walk4 attack3 die3 (10) | 320×384 | vermin |
| 2 | 1002 | Feral Ghoul | `mobs/1002_feral_ghoul/` | 32×48 | 42 | 10 | 320×384 | ghoul-flesh |
| 3 | 1003 | Hollow Hound | `mobs/1003_hollow_hound/` | 32×48 | 42 | 10 | 320×384 | hound |
| 4 | 1004 | Plague Bat | `mobs/1004_plague_bat/` | 32×48 | 42 (shadow at 45; body hovers, bottom row ≈30) | 10 | 320×384 | vermin |
| 5 | 1005 | Bonepicker Gnoll | `mobs/1005_bonepicker_gnoll/` | 32×48 | 42 | 10 | 320×384 | grave-goods |
| 6 | 1006 | Charnel Widow | `mobs/1006_charnel_widow/` | 32×48 | 42 | 10 | 320×384 | widow |
| 7 | 1007 | Gravecaller | `mobs/1007_gravecaller/` | 32×48 | 42 | 10 | 320×384 | choir-wax |
| 8 | 1008 | Revenant Sexton | `mobs/1008_revenant_sexton/` | 32×48 | 42 | 10 | 320×384 | grave-goods |
| 9 | 1009 | Gravemother | `mobs/1009_gravemother/` | 64×64 | 58 | walk4 attack3 cast4 hurt2 die4 summon4 (21) | 1344×512 | choir-wax |
| 10 | 1010 | Sepulcher Elite | `mobs/1010_sepulcher_elite/` | **D5:** 40×60 (spec) / 48×64 (card) | 52 / 56 | walk4 attack3 hurt2 die3 (12) | 480×480 / 576×512 | grave-goods |

Sheet-size caps: ≤ 320×384 common · ≤ 480×480 elite · ≤ 1344×512 boss
(`docs/art/01-FLAGS.md` F5 supersedes the bible's figures).

## Furniture / NPC kinds

| wireKind | status | folder | cell | anchorY | anims | note |
|---|---|---|---|---|---|---|
| 64 | shipped (`kWireKindVendor`) | `npcs/marta/` | 32×48 | 42 | idle4 (4) + `portrait_neutral.png`, `portrait_sneer.png` 96×96 | stall is terrain furniture (`terrain/town/`) |
| 65 | shipped (`kWireKindAnvil`) | `terrain/town/anvil/` | 48×48 | 44 | idle2 (ember pulse) + `pool_128x64.png` | the Widow Anvil is furniture, not an NPC; the Bonesmith twins (67) stand beside it |
| 66 | shipped (`kWireKindBounty`) | `npcs/bounty_board/` | 32×48 | 42 | state2 (has-bounty, empty), 1 dir → still packed as 8 identical rows | |
| 67 | reserved | `npcs/bonesmith_twins/` | 64×48 | 42 | idle4 + portrait | |
| 68 | reserved | `npcs/confessor/` | 32×48 | 42 | idle4 + portrait | lawful respawn anchor |
| 69 | reserved | `npcs/cove_fence/` | 32×48 | 42 | idle4 + portrait | |
| 70 | reserved | `npcs/guard_ashen/` | 32×48 | 42 | idle4 + portrait | |
| 71 | reserved | `npcs/guard_synod/` | 32×48 | 42 | idle4 + portrait | |
| 72 | reserved | `npcs/pledge_registrar/` | 32×48 | 42 | idle4 + portrait | |
| 73 | reserved | `npcs/castle_steward/` | 32×48 | 42 | idle4 + portrait | post-MVP |

## Players

| kit (kits.h) | class | folder | cell | anchorY | anims (cols) | sheet px |
|---|---|---|---|---|---|---|
| 1 | Ravager | `players/ravager/{m,f}/` | **D2:** 32×48 (body ≤ 43 px, rows 0–42) / 32×56 (body ≤ 50 px) | 42 / 50 | idle1 walk6 attack3 cast4 hurt2 die4 gib3 (23) | 736×384 / 736×448 |
| 2 | Gravecaller | `players/gravecaller/{m,f}/` | same | same | 23 | same |
| 3 | Cultist (Pale Choir) | `players/cultist/{m,f}/` | same | same | 23 | same |

Skin tones: `pale`, `sallow`, `weathered` (3). Weapon hand = right in all 8
dirs. Faction trim ≤ 8 % of pixels; karma never changes the sprite.

## Items & icons (ids from `items.h`; reserved ids marked)

| id | name | icon path (32×32 + 24×24 in one sheet) | in-hand attachment |
|---|---|---|---|
| 2001 | Rusty Shank | `icons/items/2001_rusty_shank` | `players/_weapons/2001/` (8 dirs, +0/+5/+9/+10 states) |
| 2002 | Pit Blade | `icons/items/2002_pit_blade` | `players/_weapons/2002/` |
| 2101 | Hide Armor | `icons/items/2101_hide_armor` | overlay layer (paper-doll) |
| 2102 | Bone Plate | `icons/items/2102_bone_plate` | overlay layer |
| 3001 | Blood Vial | `icons/items/3001_blood_vial` (3 fill states) | — |
| 4001–4005 | Rat Pelt, Ghoul Finger, Hound Fang, Widow Silk, Revenant Ash | `icons/items/400n_*` | — |
| — | gold | `icons/items/gold` | — (F15: black-iron coin) |
| 5001 (reserved) | Blackiron Ore | `icons/items/5001_blackiron_ore` | — |
| 5101–5103 (reserved) | fodder tiers | `icons/items/510n_fodder_*` | — |

Skill icons: `icons/skills/ch<1..8>_<slug>` for the shipped channels
(1 power_swing, 2 mend, 3 bless, 4 ironskin, 5 firebolt, 6 chorus,
7 mass_mend, 8 haste); GDD-set icons under `icons/skills/_later/`.
UI: `icons/ui/karma_badges` (3 × 6×6), `icons/ui/buff_strip` (9 × 16×16),
`icons/ui/callout_font_cap{7|11}.png/.json` (D4 picks one).

## VFX (strip loader pending — D8)

Until a `dirs:1` strip loader exists, every VFX ships **both** as
`strip.png` (1 row) and as `sheet.png` packed with 8 identical rows so
`loadAtlas` accepts it today. Folder = `vfx/<group>/<nn>_<slug>/` with `nn`
from `docs/art/50-vfx.md` (#1–#31).

## Terrain (no renderer yet — D6 / proposed T-ART-12)

`terrain/<zone>/plates/<terrain_id>_<name>.png` (512×256, cut by
`bhpix.cut_diamond` at world px), `terrain/<zone>/edges/<a>_<b>/{edge_NE,
edge_SE,edge_SW,edge_NW,corner_N,corner_E,corner_S,corner_W}.png` (64×32),
`terrain/<zone>/prism/{top,left,right}.png`, `terrain/<zone>/scatter/`,
`terrain/<zone>/pools/`. Terrain ids per `docs/art/10-terrain.md`.
