# Thornwall tileset

**Status:** B1 plates + D12 edge sets + prism skin generated 2026-09-08 (`tools/atlaspack/b1_build.sh`, offline QA exit 0) — **UNVALIDATED in engine** (no textured-ground renderer; T-ART-12). Record: `docs/art/batches/B1-terrain-town-fields.md`; manifest `terrain.json`.
**Spec:** `docs/art/10-terrain.md` (plate list, edge pairs measured from `data/maps-src/*.tmj`, furniture, palette anchors).
**Contents:** plates: turf, plaza dirt, cobble, lane mud, graveyard turf, river · prism skin: nailed palisade on fieldstone · 7 edge pairs · furniture: Widow Anvil (65), Marta stall (64), Wanted Board (66), well, bell post, gallows rim, gravestones ×6

## Anatomy (amended in B0)
- Ground = one painted **512×256 plate per terrain id**, cut to 64×32 diamonds at screen position by `bhpix.cut_diamond` (no random tiling).
- Edge tiles = 8 hand-painted diamonds per adjacency pair listed in the spec; **no alpha-blend overlays**.
- WALL (id 2) = skinned `iso::drawPrism` (top 64×32 + faces 32×28).
- Every warm light source ships with a painted ground pool decal (R-NIGHT).
- One palette strip ≤ 32 colours for the whole tileset (`palette_town.png`).

## Generation prompt
```
PREFIX_TERRAIN = "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable, even overcast light, muted desaturated palette only (mud brown, stagnant green, soot grey, bone off-white, black-blue water), no objects, no characters, no text, no border, luma range 30-140"
SUBJECT        = one line per plate from docs/art/10-terrain.md
NEGATIVE       = "bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette"
```
Objects/furniture use the sprite PREFIX from bible §13 with a #00FF00 plate.

## Delivery checklist
- [ ] plates 4× → `make_ground_tiles`-style reduce (box + blur 0.8, contrast ≤ 0.75, floor lift) → family quantize
- [ ] plate mean luma 45–70, min ≥ 24; render through `bhpix.night_floor(hour=2)` and archive the triptych
- [ ] edge diamonds for every listed pair (4 orientations + 4 corners)
- [ ] prism skin ×1
- [ ] furniture/scatter with feet anchors; light pools under warm sources
- [ ] `assets/LICENSES.md` rows
