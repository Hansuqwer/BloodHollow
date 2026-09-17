# assets/free — free (CC0) base packs

Curated, **CC0-licensed** asset packs (Kenney, www.kenney.nl) ingested
**2026-09-17** for BLOODHOLLOW world / NPC / mob / furniture production.
Every pack folder carries its own `CREDITS.md` (upstream URL + license +
ingest mirror) and the original upstream `License.txt` (unmodified).

Retrieved from the community mirror
[ETdoFresh/kenney.nl](https://github.com/ETdoFresh/kenney.nl)
@ `45df48c4d45f8716216b1a9e22df0b69cd9f5932` (direct kenney.nl downloads are
WAF-gated from the agent sandbox; the mirror is a byte-for-byte extraction of
the official zip contents). 2D-only subsets were copied: 3D models, vector
sources and engine project files were excluded at copy time. Upstream
`License.txt` preserved per pack where the mirror included it; for the 5
packs where the mirror omitted it (`kenney_isometric_buildings`,
`kenney_isometric_landscape`, `kenney_isometric_city`,
`kenney_modular_characters`, `kenney_rpg_pack`) Kenney's standard CC0 block
was restored at ingest time — each such file carries an end-note saying so.

## Pack index

| Folder | Pack (Kenney) | Use in BLOODHOLLOW |
|---|---|---|
| [`kenney_isometric_buildings/`](kenney_isometric_buildings/) | Isometric Tiles: Buildings (v1.0, 2014) | **world** — 2:1 iso building tiles; town/castle blockout |
| [`kenney_isometric_landscape/`](kenney_isometric_landscape/) | Isometric Tiles: Landscape (v1.0, 2014) | **world** — 2:1 iso nature tiles; fields/marsh blockout |
| [`kenney_isometric_city/`](kenney_isometric_city/) | Isometric Tiles: City (v1.0, 2014) | **world + furniture** — 2:1 iso streets/props; town square blockout |
| [`kenney_castle_kit/`](kenney_castle_kit/) | Castle Kit (v2.0) | **world** — iso + topdown castle renders; Weeping Castle (B11) |
| [`kenney_graveyard_kit/`](kenney_graveyard_kit/) | Graveyard Kit (v5.0) | **world** — dead trees, tombstones, fences; horror tone reference |
| [`kenney_furniture_kit/`](kenney_furniture_kit/) | Furniture Kit (v1.0, 2018) | **furniture** — iso + side renders; wire kinds 64+ reference |
| [`kenney_roguelike/`](kenney_roguelike/) | Roguelike/RPG pack (v1.0, 2015) | **world + furniture + NPC** — 16×16 sheet, 128 chars, sample `.tmx` maps |
| [`kenney_roguelike_indoor/`](kenney_roguelike_indoor/) | Roguelike Indoor pack (2014) | **furniture** — 16×16 interiors (beds, tables, kitchens) |
| [`kenney_roguelike_cave/`](kenney_roguelike_cave/) | Roguelike Cave & Dungeons pack (2014) | **world** — 16×16 cave/dungeon sheet; mine/crypt prototyping |
| [`kenney_animal_pack/`](kenney_animal_pack/) | Animal Pack (v1.0, 2015) | **mobs** — 80 pixel animals; rat/hound/animal-roster reference |
| [`kenney_topdown_shooter_pixel/`](kenney_topdown_shooter_pixel/) | Top-down Shooter (v1.0, 2016) | **mobs + furniture** — 580 pixel sprites incl. zombies; placeholder enemies |
| [`kenney_rpg_pack/`](kenney_rpg_pack/) | RPG Pack, 32×32 base (2014) | **mobs + NPC + world** — 32×32 medieval RPG; closest cell-size match, primary era-mood reference |
| [`kenney_medieval_rts_pack/`](kenney_medieval_rts_pack/) | Medieval RTS (v1.0, 2016) | **world** — top-down medieval town/castle; siege layout blockout |
| [`kenney_blocky_characters/`](kenney_blocky_characters/) | Blocky Characters (v2.0) | **NPC** — face + skin colour-variant textures (2D); placeholder crowd colour study |
| [`kenney_modular_characters/`](kenney_modular_characters/) | Modular Characters (v1.0, 2014) | **NPC / paper-doll** — 425 modular parts; layering + trim-% study |
| [`kenney_particle_pack/`](kenney_particle_pack/) | Particle Pack (v1.0, 2018) | **VFX** — 80 transparent particles; placeholder FX + timing reference |
| [`kenney_weapon_pack/`](kenney_weapon_pack/) | Weapon Pack 2D renders (2016/18) | **items** — weapon silhouettes for icons + in-hand attachments |

## Usage tiers (binding)

1. **Placeholder tier** (MVP-OK, may be engine-loaded as-is): blockout maps,
   soak-bot scenes, map-prototype tiles, placeholder enemies/crowds,
   placeholder VFX, UI icons in pre-release builds. Tag any such use in the
   task card; the alpha gate requires these to be replaced or re-quantized.
2. **Reference tier** (study only, **never trace**): every pack doubles as
   silhouette / palette / composition / occlusion study material for the
   AI-generated finals (`docs/prompts/asset-factory-deep-prompt.md` §10).
   Look, measure, re-interpret — do not pixel-copy into finals.
3. **Final tier** (shipped in `assets/aigen/`): only material produced by
   the generation pipeline (§2 prompt system) and the `tools/atlaspack`
   chain, passing all §12 QA gates. A free-pack sprite may become a final
   asset only by being treated as a *source plate*: re-keyed, fitted to
   cell, colour-matched + quantized to a family ramp, outlined, and QA'd —
   with the pack cited in `derivation.json` and `assets/LICENSES.md`.

**Never** under any tier: touching `assets/final/`, tracing/ripping/
upsampling *ancestor-game* art (bible §16 — unrelated to these packs), or
dropping a pack file into a delivery folder without a `LICENSES.md` row.

## Provenance

- License: **CC0 1.0 Universal** for all packs (public domain; no
  attribution required — given anyway, per pack `CREDITS.md`).
- Machine-readable provenance: [`../LICENSES.md`](../LICENSES.md) rows
  "Free packs (assets/free)".
- Production usage rules: [`docs/prompts/asset-factory-deep-prompt.md`](../docs/prompts/asset-factory-deep-prompt.md) §10.
