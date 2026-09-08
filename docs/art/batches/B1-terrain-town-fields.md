# Batch B1 — terrain: Thornwall (town) + Fields Overflow (fields) · 2026-09-08

**Base:** `origin/master @ 7723929` (gate open, `B0-GATE-DECISION.md`) · **Lane:** art only · **Nothing committed.**
**Status of every asset in this batch: generation done · offline QA exit 0 · in-engine UNVALIDATED.**
The client still draws the ground as flat `terrainColor(t)` diamonds (`client/src/game.cpp:658/671`) and
the walls as untextured `iso::drawPrism` (`engine/render/iso.cpp:52–74`): **nothing in this batch is visible
in the game until T-ART-12 (textured ground + prism skins + D12 edge lookup) exists.** The "map boards" below
are an offline composite of the real `.tmj` ground layer produced by our own tool — a proposal for what
T-ART-12 should draw, not evidence of an engine pass.

## 1. Batch summary

| zone | map | plates (512×256, ≤32 col zone palette) | D12 edge sets | prism skin | raws (4×) | status |
|---|---|---|---|---|---|---|
| town | thornwall (64×64) | 0 GRASS turf · 1 DIRT plaza · 3 WATER river · 5 PATH cobble · 6 MUD lane · 7 DARKGRASS graveyard turf · (4 WOOD = PATH stand-in, 0.4 % of map) | **10 pairs × 3 variants × 8 pieces = 240** + footing skirt (7 WALL pairs) | plank palisade: top 64×32 (plank ends) + left/right 32×28 + skirt ×8 | 7 | shipped offline · **UNVALIDATED** |
| fields | fields_overflow (64×64) | 0 GRASS furrow turf (B0 `fields_ground` raw) · 5 PATH cart track · 6 MUD field mud · 7 DARKGRASS shadow turf (town graveyard raw × 0.82) | **4 pairs × 3 × 8 = 96** + skirt (2 WALL pairs) | dead bramble hedge: top (bramble) + faces + skirt | 3 (+1 reused B0 raw) | shipped offline · **UNVALIDATED** |

Every adjacency pair measured from the two maps (17 town / 6 fields) is covered: 14 organic pairs get painted
edge sets, the 9 WALL pairs get the footing skirt (D12: **WALL never bleeds**). Together with B2 (mine + crypt)
this is the brief's "22 pairs ≥ 3 variants"; B1 delivers the 14 that exist on these two maps.

Deterministic: `sh tools/atlaspack/b1_build.sh` twice → identical md5s over every plate/edge/prism PNG
(checked this session). Exit 0 covers: 10 plate QA runs (`bh_qa_sheet.py --kind plate`, gates mean 45–70,
min-luma ≥ 24, colours ≤ 32), the edge seam audit, both map boards, both edge boards **and** a mob R-LUMA
cross-check of the B3/B4 sheets against the real B1 plates (7 pairs, all pass; full 30-pair table below).

### Plate stats (after quantize + floor clamp; `terrain.json` of each zone)

| zone | id | plate | mean | min | colours | gain applied | wrap seam (mean |Δ| across wrap vs interior) |
|---|---|---|---|---|---|---|---|
| town | 0 | GRASS turf | 48.5 | 30.3 | 23 | 0.708 | 6.3 / 6.2 (H) · 6.2 / 6.3 (V) |
| town | 1 | DIRT plaza | 48.6 | 30.3 | 23 | 0.646 | 6.2 / 6.3 · 6.3 / 6.3 |
| town | 3 | WATER river | 46.1 | 38.0 | 17 | 1.182 | 4.8 / 4.9 · 4.9 / 4.9 |
| town | 5 | PATH cobble | 48.5 | 30.3 | 25 | 0.770 | 6.3 / 6.3 · 6.3 / 6.4 |
| town | 6 | MUD lane | 47.3 | 30.3 | 28 | 0.680 | 5.4 / 5.6 · 5.8 / 6.3 |
| town | 7 | DARKGRASS graveyard | 45.9 | 30.3 | 28 | 0.890 | 6.0 / 6.2 · 6.2 / 6.3 |
| fields | 0 | GRASS furrow turf | 49.3 | 26.0 | 31 | 1.000 | 6.1 / 6.2 · 6.7 / 6.6 |
| fields | 5 | PATH cart track | 48.4 | 26.0 | 24 | 0.708 | 6.3 / 6.3 · 6.3 / 6.4 |
| fields | 6 | MUD field mud | 46.2 | 26.0 | 32 | 0.765 | 6.3 / 6.5 · 5.9 / 7.1 |
| fields | 7 | DARKGRASS shadow turf | 46.1 | 26.0 | 30 | 1.000 (×0.82 pre-darken) | 5.9 / 6.3 · 6.3 / 6.3 |

Zone palettes: 32 / 32 colours (`palette_town.png`, `palette_fields.png`; median-cut over the zone's raws,
filtered to luma ≥ 24 so the ramp cannot fight the floor clamp; no accent hue on any terrain plate).
Map boards (busiest 16×16 window of the real ground layer, day + night overlay (18,22,70,α145)):
town window [0,2] — 49 edge pieces, 39 walls, ground mean **48.6 day / 35.9 night**;
fields window [16,14] — 36 pieces, 60 walls, **38.8 / 31.7** (window is ⅔ hedge faces, which are darker
than ground by design; the ground-only plates are all 46–49).

### D3 note — why the means sit at 46–49, not 51
R-LUMA night is `Δnight = Δday × (1 − 145/255) ≈ 0.43·Δday`, so the ≥ 15 night gate needs **Δday ≥ 35**.
The darkest fields mob (1007 Waxen Celebrant, body luma ≈ 90) clears that only on plates **≤ 54**; the
brief's window 45–70 therefore has a usable top of ~54 for zones where 1007 spawns. `bh_terrain.py`
pins each plate by a pure gain to a per-material target (`MEAN_DEFAULT` 51, DARKGRASS 47.5, WATER 48,
MUD 49.5) with one corrective pass if the ramp snap pulls it outside 45.5–69.5. Material contrast is carried by
texture/hue, not by mean value (the Soma rule). The B0 approved plate (`plate_fields_mud_b0.png`, 50.3)
stays byte-identical — `make_ground_tiles` now clamps the floor by default (the brief's "fix plate clamp
first") and the B0 tile call pins `clamp_floor=False` explicitly, verified `np.array_equal` this session.

### R-LUMA cross-check — B3/B4 sheets on the real B1 plates (`bh_qa_sheet.py --kind sheet --plate …`)

| plate | 1001 rat | 1002 ghoul | 1003 hound | 1004 bat | 1005 gnoll | 1006 widow | 1007 celebrant |
|---|---|---|---|---|---|---|---|
| town GRASS | 47.6 / 20.8 | 56.8 / 24.9 | — | 54.1 / 23.7 | — | — | — |
| town PATH | 47.4 / 20.8 | 56.6 / 24.9 | — | 54.0 / 23.7 | — | — | — |
| town MUD | 46.9 / 20.2 | 56.1 / 24.3 | — | 53.5 / 23.1 | — | — | — |
| town DIRT | 47.5 / 20.8 | 56.7 / 24.9 | — | 54.1 / 23.7 | — | — | — |
| fields GRASS | 46.2 / 20.2 | — | 43.5 / 19.0 | 52.8 / 23.1 | 40.7 / 17.8 | 48.5 / 21.2 | **39.7 / 17.3** |
| fields MUD | 47.4 / 20.8 | — | 44.7 / 19.5 | 54.0 / 23.7 | 41.9 / 18.4 | 49.7 / 21.8 | 40.9 / 17.9 |
| fields PATH | 47.5 / 20.8 | — | 44.8 / 19.5 | 54.1 / 23.7 | 42.0 / 18.4 | 49.8 / 21.8 | 41.1 / 17.9 |

(Δday / Δnight; gates ≥ 25 / ≥ 15; all 30 pass, rc 0.) Numbers measured before the final wrap-blend change
of this session; the plate means moved ≤ 1.1 since, well inside the margin. The seven `b1_xcheck_*` runs in
`b1_build.sh` regenerate the binding subset every build.

## 2. Pipeline (all in `tools/atlaspack/`, deterministic, no hand-painting)

`bh_terrain.py <zone> [--variants N] [--preview]` — raw 4× paint → centre-crop 2:1 → BOX 512×256 →
blur 0.8 → contrast 0.70 → `×0.95×darken + 14` → gain to the D3 target → **roll-and-mask wrap-blend
(96 px)** → quantize onto the zone ramp (bayer2, 0.05) → `clamp_plate_floor` (≥ 24) → re-snap →
plate QA. Edge sets: `bhpix.edge_masks(depth by bleeder WATER .7 / MUD .6 / GRASS .55 / DIRT .5, cap .45,
feather 2, seed 1999 + 17·pair + variant)` → `blend_edge` on two real cut diamonds → overlay form keeps
exactly the pixels the Bayer decision assigned to the bleeder (the era checker band survives; the seam audit
checks `face_all_B` on all four faces of every variant — 30 + 12 sets true). Prism: faces 32×28 cut from the
face raw (right ×0.82, left ×0.94), top cut from the **face paint** (`face:0.78` town = plank ends,
`face:0.92` fields = bramble), skirt = 8 px footing band (`edge_masks(depth .28, cap .22)`).
`bh_terrain_preview.py` — offline map-window composite (real `.tmj` ground ids, D12 lookup by 8-neighbour
set, prism = `shear_face` of the skins at h 28, B3 rat/ghoul cells dropped as scale reference, day + night).
`b1_edge_board.py` — per pair: the 8 pieces on their base tile + a 5×5 autotile patch with rotating variants.

Edge piece naming (`terrain.json` → `pairs[]`): directory `edges/<BASE>_<OVERLAY>/v<k>/`, files
`edge_{NE,SE,SW,NW}.png` (one face bleeds) and `corner_{N,E,S,W}.png` (a point only, drawn when neither
adjoining face bleeds — `bhpix.edge_mask_for`). Overlay = the D12 bleeder, drawn on the **base** tile:
GRASS→PATH, WATER→GRASS, MUD→GRASS, DARKGRASS→GRASS, GRASS→DIRT, DARKGRASS→PATH, WATER→PATH,
DARKGRASS→DIRT, DIRT→PATH, MUD→PATH.

## 3. QA record (`docs/research-notes/qa/`)

- `b1_town_plate_{0_GRASS,1_DIRT,3_WATER,5_PATH,6_MUD,7_DARKGRASS}_{qa_3x.png,audit.json}` (6),
  `b1_fields_plate_{0_GRASS,5_PATH,6_MUD,7_DARKGRASS}_*` (4) — plate triptychs + audits, all rc 0.
- `b1_{town,fields}_map_3x.png` + `_map_audit.json` — offline map boards (day | night), `engine_validated: false`.
- `b1_{town,fields}_edges_3x.png` + `_edges_audit.json` — edge boards (pieces + autotile patch per pair).
- `b1_xcheck_<zone>_<plate>_<mob>_{qa_3x.png,audit.json}` (7) — mob R-LUMA on real plates.
- Per-zone manifest with the numbers: `assets/aigen/terrain/<zone>/terrain.json` (plates, gains, pairs,
  bleeders, variants, seam audit); palettes `palette_<zone>.png`.

## 4. LICENSES

Three new rows in `assets/LICENSES.md` (B1 raws · B1 derived plates/edges/prism/manifests · B1 QA
artefacts). Model/version **undisclosed** (provider-abstracted), date 2026-09-08, prompts verbatim in
`assets/aigen/terrain/{town,fields}/prompt.md` §Runs (10 generations, all accepted, none traced/ripped/upscaled
from ancestor art; the fields GRASS plate reuses the B0 `fields_ground_4x_raw.png` already covered by the B0 row).

## 5. Open decisions

- ⟨DIRECTOR⟩ **D3 top clipped to ≈ 54 for spawn zones** (see §1 note). Proposal: amend D3 to "floor ≈ 51,
  window 45–54 on any plate a ≤ 90-luma mob can stand on; 45–70 elsewhere (crypt CANDLE, mine lantern spill)".
- ⟨DIRECTOR⟩ **Prism top = face paint** (plank ends / bramble), not a ground plate. The first build used the
  plaza-dirt plate on top and the walls read as dirt boxes; the face-cut top reads as a palisade / hedge in the
  boards. Confirm or ask for a dedicated top generation in B2 (1 image per zone).
- ⟨DIRECTOR⟩ **Fields wall footprint:** `fields_overflow` WALL is 13.3 % of the map in long E–W hedge rows
  (614 GRASS↔WALL edges); at h 28 the rows read as dark boxes. Options: (a) keep, (b) lower hedge prism to
  h 20 (engine constant, sibling), (c) hedge as a 2-tile-tall sprite row instead of a prism (T-ART-12 scope).
- ⟨DIRECTOR⟩ **WOOD (id 4, chapel floor, 0.4 % of thornwall)** is a PATH stand-in until B2 ships the crypt
  gangway plank plate (shared art); `terrain.json` flags it (`"file": null`).
- ⟨DIRECTOR⟩ **Fields MUD** raw is the brightest paint of the batch (raw mean 69.5 → plate 46.2 via gain 0.765);
  the field mud reads slightly flatter than the town lane. Acceptable for B1; regenerate only if the director
  wants a wetter fields MUD (1 image).
- ⟨SIBLING⟩ **T-ART-12 proposal, now concrete** (`terrain.json` is the contract): ground = `cut_diamond(plate,
  wx, wy)` by world px; per tile compute the 8-neighbour set → for each bleeder in it draw the four
  `edge_*` faces present, then a `corner_*` only where neither adjoining face bleeds; variant =
  `(tx·7 + ty·13) mod 3`; WALL neighbour → `prism/skirt_*` on the ground tile, wall itself =
  `drawPrism` with `prism/{top,left,right}.png`; night overlay after all of it (D6b pools come in B8).
  Engine reads `terrain.json`; no other data files needed.
- ⟨BLOCKED⟩ Every visual claim here is offline until T-ART-12 exists (the client has no textured ground path).

## 6. Hand-off note

Uncommitted tree on `7723929`; lane files only (`assets/aigen/terrain/{town,fields}/**`, `tools/atlaspack/
{bh_terrain.py,bh_terrain_preview.py,b1_edge_board.py,b1_build.sh}`, `tools/atlaspack/make_style_tile.py`
(clamp default + explicit B0 pin, B0 plate byte-identical), `docs/research-notes/qa/b1_*`, this file,
`docs/art/README.md`, `assets/aigen/_pipeline/README.md`, `assets/aigen/REGISTRY.md`,
`assets/aigen/palettes/families.json` (+ two terrain strips), `assets/LICENSES.md`).
Cumulative patch (B3 + B4 + B1) in `/home/user/handover/b1-b4-art-lane.patch` (clean `git apply --check` on
`7723929`). Suggested commit message:

```
art(B1): terrain town + fields — 10 plates, 14 D12 edge sets ×3, prism skins, offline QA (UNVALIDATED in engine)

bh_terrain.py / bh_terrain_preview.py / b1_edge_board.py / b1_build.sh; plate clamp default-on
(B0 plate byte-identical); zone palettes ≤32; WALL never bleeds (footing skirt); T-ART-12 contract
in terrain.json. Nothing loads in the client until T-ART-12.
```
