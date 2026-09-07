# prompt.md — Gravecaller — male base (gravecaller/m)

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "male narrow-shouldered scholar in a long black ichor-stained robe, pale wax half-mask over the upper face, bronze bell censer on a short chain in the left hand, short bone knife in the right hand, sallow skin, standing idle, full body"
PALETTE  = "ichor robe #1e1a20 with #2e2a30 folds, wax half-mask #d9cdb4, bronze censer #6a5a3a, ember #c8622a, bone knife #cfc6b4, skin (3 tones)"
NEGATIVE = "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear, elf ears, sparkle, design-tool purple"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- native dirs: S, SE, E for the BASE BODY only (no gear); class overlay generated on the same body plate with the same seed
- skin tones: generate the pale tone only; sallow/weathered are index swaps of the 4 skin entries in the family strip
- frames: idle1 walk6 (contact f0/f3) attack3 (f1) cast4 (release f2) hurt2 die4 gib3 — key frames by AI (walk f0, attack f1, cast f2, die f3), the rest hand-inbetweened
- cell: 32×48 body 46 (D2a) — if D2b, regenerate nothing: re-run fit_to_cell at 52 px from the same plate

## Hand-fix list (check in this order after quantize)
- outline: close every gap at claws / teeth / tail tip / weapon tip / staff head (AI breaks these first)
- weapon / attachment stays in the RIGHT hand in all 8 dirs — derive W-side dirs by img2img, never mirror
- feet: lowest opaque body row on the anchor row; no floating shadows painted into the plate (we add the alpha-70 ellipse)
- kill any pixel above luma 200 that is not a light source, eye glint, hit-flash or bone highlight
- no accent hue (violet/arterial/choir-gold) on skin, cloth or terrain-coloured gear
- shoulder width per class (Ravager 14 / Cultist 11 / Gravecaller 10 px) measured on the S frame after downscale — adjust by hand, not by re-prompt

## Cell / sheet
32×48 (body 46 px; 32×56 if director picks the taller cell) · idle1+walk6+attack3+cast4+hurt2+die4+gib3 → 23 cols × 8 rows = 736×384

## Runs
_(none yet — one line per generation: date · model or "undisclosed" · seed if known · which dirs · accepted y/n)_
