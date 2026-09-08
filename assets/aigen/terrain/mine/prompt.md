# prompt.md — Bonehowl Mine tileset

**Status:** generation done · offline QA rc 0 · **UNVALIDATED in engine** (T-ART-12 / D6). Final prompts are recorded verbatim under *Runs*; the model/version is **undisclosed** because the provider abstracts it.

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
1. `2026-09-08 · undisclosed · seed unknown · mine/raw/cave_floor_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light, muted desaturated palette only. Slick wet slate cave floor, faint iron-blue mineral glints, subtle drip rings and damp variation. Palette anchors: wet slate #3c4048, iron-blue #5c6a7e, timber #5a4630, bone off-white #c9bfae; ground texture only, no objects, no characters, no text, no border, no accent hue, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
2. `2026-09-08 · undisclosed · seed unknown · mine/raw/mine_mud_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light, muted desaturated palette only. Black cave mud with shallow standing puddles, boot prints, dry cracked lips, damp silt, restrained low-contrast texture. Palette anchors: wet slate #3c4048, iron-blue #5c6a7e, soot grey, mud brown, bone off-white #c9bfae; ground texture only, no objects, no characters, no text, no border, no accent hue, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
3. `2026-09-08 · undisclosed · seed unknown · mine/raw/rock_face_4x_raw.png · accepted y` — "Pre-rendered painterly dark horror fantasy texture in Myth of Soma 2001 isometric MMORPG style: a single seamless orthographic elevation strip for a cave wall face, not a top-down scene. Dark rock excavation face with rough timber braces, iron brackets, soot deposits, damp sheen, irregular stone strata; compose as a tileable 4:1 vertical wall material with no objects or characters. Muted desaturated palette: wet slate #3c4048, iron-blue #5c6a7e, timber #5a4630, bone off-white #c9bfae, soot grey; no bright accent hue. No text, no border, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
4. `2026-09-08 · undisclosed · seed unknown · mine/raw/gangway_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate. Old timber mine gangway planks, wet and iron-banded, moss in the joints, rotted ends, dark worn grain, restrained damp sheen. Muted desaturated palette only: timber brown #5a4630, wet slate #3c4048, iron-blue #5c6a7e, soot grey, bone off-white; no warm glow. Ground plate only, no objects, no characters, no text, no border, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
