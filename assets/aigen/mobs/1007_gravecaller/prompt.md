# prompt.md — Gravecaller mob (1007)

**Status:** B4 first pass generated 2026-09-08 (see *Runs*); offline QA exit 0; **UNVALIDATED in engine**. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

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
- S · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md SUBJECT (D9: **Waxen Celebrant**, folder name kept `1007_gravecaller` for `shared/content/mobs.h`) + "3/4 facing south, censer in the RIGHT hand" → 1456×720 output, keyed 269×412 (w/h 0.65) · accepted **y** (B4 first pass)
- E · **not generated — turn generation limit reached** (10/10). E/W/NE/NW/SW are derived from S (`derivation.json`: E = fallback S). ⟨DIRECTOR⟩ one E generation is the first upgrade.
- Build: `bh_mob_sheet.py 1007 --body-h 44 --family choir-wax --walk-style glide --shadow-rx 7 --lunge 2 --asym-box 0,18,11,44 --attack-fx dots5 --n-hide-face 0.3 --gamma 0.42 --pin ember=c8622a` → 80 cells → 320×384 → `qa/b4_1007_gravecaller_*` exit 0. Gamma 0.42 because the ichor-black robe sat at Δday −7.6 unlifted; the censer ember is pinned to `#c8622a` (3 px) so it survives quantize in all 8 dirs; the wax mask is hidden on N/NE/NW (hood). **UNVALIDATED in engine.**
