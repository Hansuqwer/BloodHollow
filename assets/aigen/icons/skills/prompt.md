# prompt.md — Skill icons (8 shipped channels + GDD set)

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "see BRIEF.md / spec"
PALETTE  = "backing: iron/red #8e101c→#d8302a · ash-blue #6e7a8a · bone-white #e6e0d4 · choir-gold #d9b04a · era-violet #6b4a8a/#a884c4"
NEGATIVE = "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear, elf ears, sparkle, design-tool purple"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- generate at 8× (256×256) per icon, fit_to_cell NEAREST to 32 and 24; the 8 shipped skill icons get a hand redraw at 2×
- backing plate #151013 + 2 px bevel in the family colour is composited by the pipeline, not generated

## Hand-fix list (check in this order after quantize)
- one silhouette per icon family (greyscale check at 24 px)
- no text in icons
- refine states +5/+9/+10 are paint edits of the +0 icon, never regenerations

## Cell / sheet
see BRIEF.md

## Runs
_(none yet — one line per generation: date · model or "undisclosed" · seed if known · which dirs · accepted y/n)_
