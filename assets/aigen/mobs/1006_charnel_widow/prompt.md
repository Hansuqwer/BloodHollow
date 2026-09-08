# prompt.md — Charnel Widow (1006)

**Status:** B4 first pass generated 2026-09-08 (see *Runs*); offline QA exit 0; **UNVALIDATED in engine**. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "Charnel Widow, bloated grave spider, its abdomen is a fused wooden coffin lid with brass nails, black chitin legs with grey-pink joints, trailing grey shroud-thread silk, eight bone-white eyes, low deliberate stance, 3/4 view from above, single creature"
PALETTE  = "coffin-lid wood #3a2e26, brass nails #8a6a3a, chitin black #141014, joint grey-pink #8e7776, shroud silk #a9a29a, venom #4a5a3a, eyes #cfc6b4"
NEGATIVE = "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear, elf ears, sparkle, design-tool purple"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- native dirs: S, SE, E (three generations, same seed family)
- img2img: SW/W/NW/N/NE from the S/E plates at denoise ≤ 0.35, pose guidance = the E plate flipped ONLY as guidance, never as output
- frames: walk 4 (contact feet on f0/f2), attack 3 (contact f1), die 3 (+ corpse frame = last die frame)

## Hand-fix list (check in this order after quantize)
- outline: close every gap at claws / teeth / tail tip / weapon tip / staff head (AI breaks these first)
- weapon / attachment stays in the RIGHT hand in all 8 dirs — derive W-side dirs by img2img, never mirror
- feet: lowest opaque body row on the anchor row; no floating shadows painted into the plate (we add the alpha-70 ellipse)
- kill any pixel above luma 200 that is not a light source, eye glint, hit-flash or bone highlight
- no accent hue (violet/arterial/choir-gold) on skin, cloth or terrain-coloured gear
- widow: legs are 1–2 px lines with a knee kink — re-draw legs by hand after quantize, AI merges them

## Cell / sheet
32×48 (legs span 32 px, body 22 px) · 10 cols × 8 rows = 320×384

## Runs
- S · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md SUBJECT + "3/4 view from above facing south, legs spread wide" → keyed 1083×671 (w/h 1.61) · accepted **y** (B4 first pass)
- E · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md SUBJECT + "strict side profile facing EAST" → keyed 1247×697 (w/h 1.79) · accepted **y** (B4 first pass)
- SE · not generated — derived from S (see `derivation.json`)
- Build: `sh tools/atlaspack/b4_build.sh` → `bh_mob_sheet.py 1006 --body-h 22 --family widow --kind spider --walk-style glide --shadow-rx 14 --lunge 3 --attack-fx line8 --gamma 0.65` → 80 cells → `sheet.png` 320×384 → `docs/research-notes/qa/b4_1006_charnel_widow_{qa_3x.png,audit.json}` exit 0. Legs span the full 32 px (width-limited fit, body 20/18 px instead of 22 — the legs are the silhouette). **UNVALIDATED in engine** (T-ART-01/05).
