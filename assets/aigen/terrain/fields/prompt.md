# prompt.md — Fields of the Overflow tileset

**Status:** B1 generated 2026-09-08 (see *Runs*); plates/edges/prism built by `tools/atlaspack/bh_terrain.py` — **UNVALIDATED in engine** (T-ART-12). Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable, even overcast light, muted desaturated palette only (mud brown, stagnant green, soot grey, bone off-white, black-blue water), no objects, no characters, no text, no border, luma range 30-140"
SUBJECT  = "see BRIEF.md / spec"
PALETTE  = "per zone palette anchors in docs/art/10-terrain.md §6.1–6.5"
NEGATIVE = "bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- plates: one 4× painterly generation per terrain id in the BRIEF list, prompt = PREFIX_TERRAIN + subject line from docs/art/10-terrain.md
- process: box-reduce → blur 0.8 → contrast ≤ 0.75 → floor lift (D3) → family quantize ≤ 32 → cut with bhpix.cut_diamond; QA with bh_qa_sheet.py --kind plate
- edges: bhpix.edge_masks() gives the 8 masks per pair; bhpix.blend_edge() composes a first cut, then hand-paint the boundary (roots, mortar, algae)
- objects/furniture: sprite PREFIX + #00FF00 plate; anchor at feet; light pools as separate 96×48 / 128×64 decals

## Hand-fix list (check in this order after quantize)
- no outlines on ground; outline only trunks / pillars that occlude bodies
- plate mean luma 45–70, min ≥ 24 (bh_qa_sheet plate gates)
- no accent hue anywhere on terrain; warm light only in pool decals

## Cell / sheet
see BRIEF.md

## Runs
- GRASS (furrow turf) reuses the B0 `docs/research-notes/style-tile/plates/fields_ground_4x_raw.png` (covered by the B0 LICENSES row); DARKGRASS = town `graveyard_turf_4x_raw.png` × 0.82 (no new generation).
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/field_mud_4x_raw.png` 1024×1024 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: ploughed field mud, broken furrows, clods, thin puddles in the low lines, rotten stubble. Palette: #3a2f26 mud, #2b2628 soot, #5a5652 ash, stagnant green. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/cart_track_4x_raw.png` 1024×1024 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: cart track across a field: two worn wheel ruts with a grass strip between, packed earth, small stones. Palette: #3a2f26 mud, #5a5652 ash, stagnant green, #2b2628 soot. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/hedge_face_4x_raw.png` 1408×768 · accepted **y** · prompt: "pre-rendered painterly wall texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, flat front elevation view straight on, horizontally seamless tileable strip, even overcast light, muted desaturated palette only, no characters, no text, no border, luma range 30-140. Subject: dead bramble hedge, dense tangled thorny stems, dry leaves, a few pale snapped branches. Palette: #2b2628 soot, #3a2f26 mud, #5a5652 ash, #c9bfae bone (snapped wood). Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, perspective, horizon."
