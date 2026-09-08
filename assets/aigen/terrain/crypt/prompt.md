# prompt.md — Drowned Crypt + Thornwall Crypt tileset

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
5. `2026-09-08 · undisclosed · seed unknown · crypt/raw/causeway_flag_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light, muted desaturated palette only. Old cathedral flagstones with algae at the edges, cracked surfaces, bone dust in the joints, worn masonry, subtle damp staining. Palette: soot grey, stagnant green, bone off-white #c9bfae, wet slate #3c4048, black-blue water #0c0e16 used only as deep shadow; no objects, no characters, no text, no border, no accent hue, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
6. `2026-09-08 · undisclosed · seed unknown · crypt/raw/crypt_water_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light, muted desaturated palette only. Still black-green crypt water, near-flat surface, only two-tone dithered ripple paint, faint painted column reflections, deep shadows, no glow, no waves. Palette anchor black-blue water #0c0e16 interpreted as a deep green-black hue with readable subdued midtone texture, soot grey, stagnant green; no objects, no people, no text, no border, no accent hue, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard, luminous water." Generation wording intentionally avoids the moderation-blocked terms recorded in B4 1009/prompt.md.
7. `2026-09-08 · undisclosed · seed unknown · crypt/raw/worn_flag_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light, muted desaturated palette only. Worn cathedral floor flagstones, centuries of feet, candle-wax smears, hairline cracks, rot bloom in corners, quiet damp wear. Palette: ash grey, soot grey, bone off-white #c9bfae, stagnant green, wet slate #3c4048; ground texture only, no objects, no characters, no text, no border, no saturated accent hue, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
8. `2026-09-08 · undisclosed · seed unknown · crypt/raw/masonry_face_4x_raw.png · accepted y` — "Pre-rendered painterly dark horror fantasy texture in Myth of Soma 2001 isometric MMORPG style: a single seamless orthographic elevation strip for a dark catacomb masonry wall face, not a top-down scene. Skull niches every fourth block, lime mortar, damp stains, rough aged blocks, deep recesses that remain readable at low resolution; compose as a tileable 4:1 vertical wall material. Muted desaturated palette only: soot grey, wet slate #3c4048, bone off-white #c9bfae for masonry and niche rims, stagnant green, black-blue shadows; no glow, no objects beyond the wall surface, no characters, no text, no border, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard."
9. `2026-09-08 · undisclosed · seed unknown · crypt/raw/bone_pit_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light. Flagstone floor at the rim of a bone pit, aged bones piled within the opening, matte dusty surfaces, no shine, bone dust and worn stone, restrained contrast so it reads as a rare edge material. Palette: soot grey, bone off-white #c9bfae, wet slate #3c4048, mud brown, stagnant green; no objects beyond the ground treatment, no characters, no text, no border, no saturated accent hue, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard, glossy bones."
10. `2026-09-08 · undisclosed · seed unknown · crypt/raw/candle_wax_4x_raw.png · accepted y` — "Pre-rendered painterly ground texture, Myth of Soma 2001 isometric MMORPG style, dark horror dark-fantasy, top-down orthographic, seamless tileable single 2:1 ground plate, even overcast light. Flagstone floor pooled and streaked with old wax, a few burnt candle stubs, warm-sickly but subdued, matte age and soot, rare material read. Palette remains muted and desaturated: ash grey, bone off-white #c9bfae, soot grey, dull ochre wax, wet slate #3c4048; no glow, no saturated accent hue, no objects beyond the floor surface, no characters, no text, no border, luma range 30-140. Negative: bright, saturated, neon, lens flare, bloom, 3D render, photo, blur, watermark, text, gradient sky, vignette, hard seams, checkerboard, candle flame, luminous bloom."
