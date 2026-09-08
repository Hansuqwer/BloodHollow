# prompt.md — Plague Bat (1004)

**Status:** B3 first-pass GENERATED 2026-09-08 (native S + E plates; SE/SW/W/NW/N/NE derived). Offline QA pass; in-engine UNVALIDATED. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "Plague Bat, fat diseased bat in flight, ragged dark membrane wings spread in a W, swollen greenish plague belly, tiny bone claws, 3/4 view from slightly above, single creature, hovering"
PALETTE  = "wet slate #4a4a50, mud #5a4a3a, grey-pink belly #9a7f7e, bone teeth #cfc6b4, black ichor #141014, rust eye #7a2a1e, plague-green bloat #5a6a48"
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
- bat: body bottom ≈ y30, shadow ellipse at y45 (hover read); wing blur allowed on 2 of 4 walk frames

## Cell / sheet
32×48 (body 18 px, hovers; shadow at y=45) · 10 cols × 8 rows = 320×384

## Runs
- S · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md, hovering, wings W · accepted **y** (B3 first pass)
- E · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md, side profile flying EAST · accepted **y** (B3 first pass)
- SE (except 1001) · not generated — derived from S/E by `bh_mob_sheet.py` (see `derivation.json`); a native SE plate is the first upgrade when generation budget allows
- Build: `sh tools/atlaspack/b3_build.sh` → `cells/` (80) → `sheet.png` + `sheet.json` → `docs/research-notes/qa/b3_<slug>_qa_3x.png` + `_audit.json` (exit 0, 2026-09-08). **UNVALIDATED in engine** (T-ART-01/05).
