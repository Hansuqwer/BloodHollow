# prompt.md — Gravecaller mob (1007)

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "Gravecaller plague priest, hollowed corpse in black ichor-stained scholar robes, featureless pale wax mask, swinging a bronze bell censer on an iron chain, ember glow in the censer, robe hem trailing grave mud, 3/4 south-facing, single figure"
PALETTE  = "ichor-black robe #1e1a20, wax mask #d9cdb4, chain iron #4a4e58, bell bronze #6a5a3a, censer ember #c8622a, grave-mud hem #3a2f26, verdigris #4a5a4e, drowned flesh #8a8290, weed hair #2e3a2e"
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

## Cell / sheet
32×48 (body 44 px) · 10 cols × 8 rows = 320×384

## Runs
_(none yet — one line per generation: date · model or "undisclosed" · seed if known · which dirs · accepted y/n)_
