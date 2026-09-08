# Batch B2 — Bonehowl Mine + Drowned Crypt + Thornwall Crypt · 2026-09-08

**Base:** `origin/master @ e855e6f` (B3/B4/B1 already on the branch; B0 gate open) · **Lane:** art only · **Nothing committed or pushed.**
**Status of every new B2 asset:** generation done · offline QA exit 0 · **UNVALIDATED in engine**.
The client still draws flat `terrainColor(t)` diamonds and untextured `iso::drawPrism`; the map boards below are
offline composites of the real `.tmj` ground layers, not in-engine proof. T-ART-12 (textured ground + prism skins +
D12 edge lookup) is still the sibling dependency.

## 1. Batch summary

| zone | map | plates (512×256; mean / min / colours) | D12 pairs | prism | raws / derivations | status |
|---|---|---|---|---|---|---|
| `mine` | `bonehowl_mine` | DIRT 48.5 / 31.9 / 24 · WOOD 52.2 / 31.9 / 32 · MUD 48.8 / 31.9 / 25 · DARKGRASS 46.0 / 31.9 / 16; palette 32 | 4 manifest pairs: **3 painted × 3 variants × 8 = 72**; DIRT↔WALL uses footing skirt | rock + timber face, `prism/v0..v2` (3 face variants) + shared skirt | 4 fresh raws; DARKGRASS = town `graveyard_turf` raw × 0.80 plus restrained lichen tint `(0.92, 1.05, 0.92)` | shipped offline · **UNVALIDATED** |
| `crypt_drowned` | `drowned_crypt` | DIRT 49.5 / 30.6 / 29 · WATER 45.8 / 30.6 / 15 · WOOD 51.8 / 30.6 / 32 · MUD 48.7 / 30.6 / 20 · DARKGRASS 46.1 / 30.6 / 28; shared palette 32 | **5 painted × 3 variants × 8 = 120**; hero DIRT↔WATER included; no WALL pair | none — the authoritative map has no WALL tiles | 2 fresh raws; WOOD = mine gangway; MUD = mine mud × 0.85 plus blue-grey tint; DARKGRASS = town graveyard raw × 0.82 | shipped offline · **UNVALIDATED** |
| `crypt_thornwall` | `thornwall_crypt` | FLOOR 50.7 / 30.6 / 24 · SLAB 50.7 / 30.6 / 24 · BONEPIT 47.4 / 30.6 / 29 · CANDLE 50.3 / 30.6 / 28; shared palette 32 | 4 manifest pairs: **3 painted × 3 variants × 8 = 72**; FLOOR↔WALL uses footing skirt | catacomb masonry face, `prism/v0..v2` (3 face variants) + shared skirt | 4 fresh raws; SLAB = worn flag raw × 0.78 | shipped offline · **UNVALIDATED** |

**B2 total:** 10 new AI generations, 11 painted D12 pair sets × 3 variants × 8 pieces = **264 edge PNGs**,
2 prism families with 3 face variants each, shared footing skirts, one mine ramp and one shared crypt ramp. Every
plate is inside the D3 45.5–69.5 mean window and has min luma ≥ 24; all plate ramps are filtered to ≤32 colours
and luma ≥24. `terrain.json` records the raw source and every derived darken/tint operation.

The crypt raw paintings are kept once in `assets/aigen/terrain/crypt/raw/`; the two usage manifests and delivery
folders are separate (`terrain/crypt/drowned/` and `terrain/crypt/thornwall/`). The shared `gangway` raw also
resolves the B1 town id 4 WOOD stand-in: after the B2 tool/config update, `assets/aigen/terrain/town/terrain.json`
points `WOOD.file` at `assets/aigen/terrain/town/plates/4_WOOD.png` and its raw field at the shared mine gangway.
`sh tools/atlaspack/b1_build.sh` re-ran with that replacement and exited 0.

## 2. Generation record and provenance

The ten fresh raws are:

| # | raw | prompt source / result |
|---:|---|---|
| 1 | `mine/raw/cave_floor_4x_raw.png` | slick wet slate cave floor, iron-blue glints, drip rings; accepted |
| 2 | `mine/raw/mine_mud_4x_raw.png` | black cave mud, puddles, boot prints, dry cracked lips; accepted |
| 3 | `mine/raw/rock_face_4x_raw.png` | dark rock excavation face, timber braces, iron brackets, soot and damp sheen; accepted as elevation strip |
| 4 | `mine/raw/gangway_4x_raw.png` | wet iron-banded mine gangway planks, moss joints, rotted ends; accepted; shared with drowned crypt and town WOOD |
| 5 | `crypt/raw/causeway_flag_4x_raw.png` | cathedral flagstones, algae edges, cracks, bone dust; accepted |
| 6 | `crypt/raw/crypt_water_4x_raw.png` | still black-green crypt water, two-tone ripple paint, faint column reflections, no glow; accepted with moderation-safe wording |
| 7 | `crypt/raw/worn_flag_4x_raw.png` | worn cathedral flagstones, wax smears, hairline cracks, rot bloom; accepted |
| 8 | `crypt/raw/masonry_face_4x_raw.png` | catacomb masonry elevation strip, skull niches every fourth block, lime mortar and damp; accepted |
| 9 | `crypt/raw/bone_pit_4x_raw.png` | flagstone rim, aged matte bones and bone dust, no shine; accepted |
| 10 | `crypt/raw/candle_wax_4x_raw.png` | flagstones pooled with old wax and burnt stubs, subdued warm-sickly read; accepted |

The final prompts are appended verbatim under `assets/aigen/terrain/mine/prompt.md` (runs 1–4) and
`assets/aigen/terrain/crypt/prompt.md` (runs 5–10). Provider/model is **undisclosed**; no seed was exposed.
No ancestor art was traced, ripped or upscaled. The crypt water prompt deliberately avoids the moderation-blocked
wording recorded in B4 `1009_gravemother/prompt.md`; no run was blocked in B2.

## 3. Pipeline changes

- `bh_terrain.py` now registers `mine`, `crypt_drowned`, and `crypt_thornwall`; crypt raw sources are shared,
  usage manifests are separate, and the town WOOD contract points to the real gangway plate.
- `DARKEN`/`TINT` are pixel operations for the four derived B2 materials. `BLEED_DEPTH` includes BONEPIT 0.60
  and CANDLE 0.45; WATER remains 0.70 for the hero DIRT↔WATER edge.
- QA prefixes are per-zone: B1 remains `b1_*`; all B2 artefacts are `b2_*`. `b1_edge_board.py --batch b2`
  writes the B2 edge boards without renaming any B1 evidence.
- Mine and Thornwall crypt prism faces use `prism/v0..v2`; the footing skirt is invariant. `terrain.json` writes
  `prism_variants` for those zones and `engine_validated: false` for all three manifests.
- The preview scorer rewards WALL/WATER and also BONEPIT/CANDLE in Thornwall crypt. Each B2 map board drops two
  B3/B4 scale witnesses: hound + gnoll for mine; Revenant Sexton + Waxen Celebrant for both crypt maps.

## 4. QA record

All audit JSON files below report no failures and `engine_validated: false`. The completed gate command was:

```sh
sh tools/atlaspack/b2_build.sh   # exit 0
```

The B2 gate was run twice; the second run produced **identical md5s** for the B2 terrain outputs and all
`docs/research-notes/qa/b2_*` files.

### Plate triptychs + audits (13 plates)

- `docs/research-notes/qa/b2_mine_plate_{1_DIRT,4_WOOD,6_MUD,7_DARKGRASS}_{qa_3x.png,audit.json}`
- `docs/research-notes/qa/b2_crypt_drowned_plate_{1_DIRT,3_WATER,4_WOOD,6_MUD,7_DARKGRASS}_{qa_3x.png,audit.json}`
- `docs/research-notes/qa/b2_crypt_thornwall_plate_{1_FLOOR,3_SLAB,4_BONEPIT,5_CANDLE}_{qa_3x.png,audit.json}`

### Map boards + audits (3)

- `docs/research-notes/qa/b2_mine_map_3x.png` + `b2_mine_map_audit.json` — witnesses `1003_hollow_hound`, `1005_bonepicker_gnoll`; 14 edge pieces, 138 walls; ground window 44.7 day / 34.1 night.
- `docs/research-notes/qa/b2_crypt_drowned_map_3x.png` + `b2_crypt_drowned_map_audit.json` — witnesses `1008_revenant_sexton`, `1007_gravecaller`; 82 edge pieces, 0 walls; 43.7 day / 33.8 night.
- `docs/research-notes/qa/b2_crypt_thornwall_map_3x.png` + `b2_crypt_thornwall_map_audit.json` — witnesses `1008_revenant_sexton`, `1007_gravecaller`; 27 edge pieces, 164 walls; 62.0 day / 41.7 night (window includes BONEPIT/CANDLE scoring).

### Edge boards + audits (3)

- `docs/research-notes/qa/b2_mine_edges_3x.png` + `b2_mine_edges_audit.json` — 3 painted pairs.
- `docs/research-notes/qa/b2_crypt_drowned_edges_3x.png` + `b2_crypt_drowned_edges_audit.json` — 5 painted pairs, including DIRT↔WATER boundary length 245.
- `docs/research-notes/qa/b2_crypt_thornwall_edges_3x.png` + `b2_crypt_thornwall_edges_audit.json` — 3 painted pairs; FLOOR↔WALL is represented by the prism footing.

### R-LUMA cross-checks (11 sheets; all gates pass)

- Mine: `b2_xcheck_mine_1_DIRT_1003_hollow_hound_*`, `b2_xcheck_mine_6_MUD_1004_plague_bat_*` (`--hover 12`),
  `b2_xcheck_mine_1_DIRT_1005_bonepicker_gnoll_*`, `b2_xcheck_mine_1_DIRT_1006_charnel_widow_*`.
- Drowned crypt: `b2_xcheck_crypt_drowned_1_DIRT_1007_gravecaller_*` (Waxen Celebrant folder),
  `..._1008_revenant_sexton_*`, `..._1009_gravemother_*` (64×64 / anchor 58),
  `..._1010_sepulcher_elite_*` (40×60 / anchor 52).
- Thornwall crypt: `b2_xcheck_crypt_thornwall_1_FLOOR_1007_gravecaller_*`,
  `..._1002_feral_ghoul_*`, `..._1006_charnel_widow_*`.

The decisive dark-plate checks are Waxen Celebrant on crypt DIRT: Δday 40.2 / Δnight 17.3 on Drowned Crypt,
and on Thornwall FLOOR: Δday 39.0 / Δnight 17.1. Larger B4 cells also pass: Gravemother 43.1 / 18.5 and
Sepulcher Elite 43.0 / 18.6 on Drowned DIRT. The `b2_xcheck_*_audit.json` files are the numeric source of truth.

### Other build gates

After the B2 tool/config changes, these all exited 0:

```sh
sh tools/atlaspack/b1_build.sh
sh tools/atlaspack/b3_build.sh
sh tools/atlaspack/b4_build.sh
```

The B1 town WOOD replacement additionally wrote `b1_town_plate_4_WOOD_{qa_3x.png,audit.json}`. `cells/` and
`tools/atlaspack/__pycache__/` are regenerable/ignored; `__pycache__` is removed at hand-off.

## 5. LICENSES rows

Four provenance rows are added to `assets/LICENSES.md`:

1. B2 terrain raws (10 AI-generated raws, provider/model **undisclosed**, prompts in the two `prompt.md` Runs sections).
2. B2 derived plates, 264 D12 edge PNGs, prism face variants/skirts, shared palettes, manifests and `b2_build.sh`.
3. B2 QA artefacts (60 files: 30 triptychs/boards + 30 audits; numbers only).
4. The B1 town WOOD provisional is retired in favour of the shared gangway plate; the offline manifest now points to
   `4_WOOD.png` while the client remains UNVALIDATED.

## 6. Open decisions and hand-off tags

| tag | item / decision |
|---|---|
| ⟨DIRECTOR⟩ **D3 carried item — propose close** | Waxen Celebrant (body luma ≈89.7) clears the binding night gate on both crypt spawn plates: Drowned DIRT Δnight 17.3 and Thornwall FLOOR Δnight 17.1 (day deltas 40.2 / 39.0). Keep the implemented D3 targets/window for legal plates; propose closing the carried D3 top-≈54 question after director confirmation. |
| ⟨DIRECTOR⟩ **NEW crypt WATER amendment request** | The requested near-black `#0c0e16` day plate cannot coexist with the binding plate gate (mean 45–70, min ≥24). B2 ships the darkest legal green-black water plate (mean 45.8, min 30.6, 15 colours), with two-tone ripple paint; the night overlay darkens it. Request a D3 amendment only if the director wants a genuinely ≤24-luma day water exception; do not silently break the gate. |
| ⟨DIRECTOR⟩ **Prism top = face paint carried** | Mine rock top uses a face cut (`face:0.78`) and Thornwall masonry uses a face cut (`face:0.82`); boards read as brace/block ends. No image was spent on dedicated top generations. Carry the option to request one dedicated top generation per family later. |
| ⟨DIRECTOR⟩ **Fields hedge rows** | B1 h28 hedge footprint remains a B1 item; carry options (keep / h20 sibling constant / sprite row) unchanged. |
| ⟨DIRECTOR⟩ **Fields MUD flatness** | B1 field MUD remains a B1 carry; B2 mine/crypt mud uses separate wet/silt paint plus gain/tint and passes the board/plate gates. |
| ⟨RESOLVED⟩ **Town WOOD stand-in** | B1 id 4 WOOD PATH stand-in is retired. The shared B2 gangway raw/plate is registered in town, Drowned Crypt and Mine; B1 revalidation exits 0. Still UNVALIDATED in engine until T-ART-12. |
| ⟨SIBLING⟩ **T-ART-12** | Terrain renderer must read each `terrain.json`; cut 64×32 diamonds from plates by world px, apply D12 lookup/variant `(tx·7 + ty·13) mod N`, use `prism/v{k}` for WALL faces and root footing skirts, and run night overlay after ground/edges/prisms (pools later, D6b). Drowned Crypt has no WALL tiles and must not be given a prism. |
| ⟨SIBLING⟩ **T-ART-01 / 04 / 05 / 06 / 09 / 10** | Mob sheet lookup, animation playback, per-kind atlas loading, NPC/portrait channel, decal/pool layer, and elite/boss cell/anchor support remain sibling work. B2 only consumes the offline sheets as scale witnesses and does not claim those cards shipped. |
| ⟨BLOCKED⟩ **Engine validation** | No client integration was attempted; all visual evidence is offline. Nothing in this batch is shipped in-engine. |

## 7. Hand-off note (do not commit or push without explicit instruction)

The tree is intentionally **uncommitted**. Suggested four-commit split, mirroring the established art lane:

```sh
# 1 — tools
art(tools): extend terrain pipeline for B2 mine + crypt usage manifests, prism variants and QA prefixes

# 2 — B2 art (including the resolved town WOOD replacement)
art(B2): terrain mine + both crypts — 11 D12 edge sets x3, rock/masonry prism skins, shared crypt+gangway raws, offline QA (UNVALIDATED until T-ART-12)

# 3 — batch record + QA evidence
art(B2): batch record and offline QA boards — mine + both crypts (rc 0, UNVALIDATED)

# 4 — docs / provenance / registry
art(docs): register B2 terrain mine + both crypts — LICENSES, REGISTRY, ramps, pipeline, art index
```

Do not run `git commit` or `git push` unless the user explicitly asks. Next queued batch is B5 NPCs + portraits; do not start it in this hand-off.
