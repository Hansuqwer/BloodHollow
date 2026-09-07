# prompt.md — Smugglers' Cove fence (kind 69, reserved)

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "hooded dockside fence, wax-seal ledger, grime on the hem, Marrowgate rope belt, only the mouth and a gold tooth lit under the hood, standing idle, full body"
PALETTE  = "bone-white robe #e6e0d4 greying to #a9a29a, incense-gold ink #d9b04a (≤8%), wax seal #8e101c (tiny), skin pale #d6c6be"
NEGATIVE = "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear, elf ears, sparkle, design-tool purple"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- native dirs: S, SE, E; idle 4f = one micro-motion (see BRIEF) — draw f1..f3 by hand from f0
- portrait: separate 96×96 painterly generation (see BRIEF), quantize ≤ 32, no frame baked in

## Hand-fix list (check in this order after quantize)
- outline: close every gap at claws / teeth / tail tip / weapon tip / staff head (AI breaks these first)
- weapon / attachment stays in the RIGHT hand in all 8 dirs — derive W-side dirs by img2img, never mirror
- feet: lowest opaque body row on the anchor row; no floating shadows painted into the plate (we add the alpha-70 ellipse)
- kill any pixel above luma 200 that is not a light source, eye glint, hit-flash or bone highlight
- no accent hue (violet/arterial/choir-gold) on skin, cloth or terrain-coloured gear
- NPCs never carry the accent ramp except the Registrar's ink (choir-gold ≤ 8 %) and the Bonesmith ember rim

## Cell / sheet
32×48 · idle4 × 8 dirs = 128×384 + portrait 96×96

## Runs
_(none yet — one line per generation: date · model or "undisclosed" · seed if known · which dirs · accepted y/n)_
