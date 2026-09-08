# prompt.md — Thornwall tileset

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
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/turf_4x_raw.png` 1408×768 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: trampled village turf, sparse dull grass over packed grey-brown earth, small stones, faint boot ruts. Palette: #5a5652 ash, #2b2628 soot, #3a2f26 mud, #4a4e58 iron, stagnant green. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/plaza_dirt_4x_raw.png` 1408×768 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: hard-packed market plaza dirt, cart-wheel ruts, scattered straw, dried puddle rings. Palette: #3a2f26 mud, #5a5652 ash, #2b2628 soot, #c9bfae bone (straw only). Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/cobble_4x_raw.png` 1408×768 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: old rounded cobblestones with dark mortar and moss in the joints, uneven, worn. Palette: #5a5652 ash, #4a4e58 iron, #2b2628 soot, stagnant green moss. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/lane_mud_4x_raw.png` 1024×1024 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: churned wet lane mud with standing water in the ruts, boot prints, rotting straw. Palette: #3a2f26 mud, #2b2628 soot, black-blue water, #5a5652 ash. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/graveyard_turf_4x_raw.png` 1408×768 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: dark neglected graveyard turf, matted grass, dead leaves, bare patches of black soil. Palette: #2b2628 soot, #3a2f26 mud, stagnant green, #5a5652 ash. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/river_4x_raw.png` 1408×768 · accepted **y** · prompt: "pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic view straight down, seamless tileable, even overcast light, muted desaturated palette only, no objects, no characters, no text, no border, luma range 30-140. Subject: slow dark river water, faint ripples, streaks of foam scum and drifting silt, opaque. Palette: #0c0e16 black-blue water, #3a3e48 slate, #4a4e58 iron, dull foam grey. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, objects, perspective, horizon."
- 2026-09-08 · generate_image (Arena Agent Mode), model/version **undisclosed** · seed unknown · `raw/palisade_face_4x_raw.png` 1376×768 · accepted **y** · prompt: "pre-rendered painterly wall texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, flat front elevation view straight on, horizontally seamless tileable strip, even overcast light, muted desaturated palette only, no characters, no text, no border, luma range 30-140. Subject: nailed timber palisade of split logs on a low fieldstone footing, weathered grey wood, iron nails, rope lashings. Palette: #5a5652 ash, #4a4e58 iron, #2b2628 soot, #3a2f26 mud. Avoid: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, perspective, horizon."
