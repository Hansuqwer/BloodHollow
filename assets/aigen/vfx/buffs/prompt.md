# prompt.md — VFX — Buffs & heals (#17–#22)

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "see BRIEF.md / spec"
PALETTE  = "reserved ramp only: era-violet #6b4a8a/#a884c4 (curse) · arterial #8e101c→#d8302a (blood) · choir-gold #d9b04a (holy) · iron #6a6e78 · ash-blue #6e7a8a · bone-white #f4ece0 · ember #c8622a"
NEGATIVE = "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear, elf ears, sparkle, design-tool purple"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- generate the PEAK frame only per effect (see docs/art/50-vfx.md sizes); in/out frames hand-drawn from it
- deliver strip.png (1 row) AND sheet.png (8 identical rows) until a dirs:1 loader exists (D8)

## Hand-fix list (check in this order after quantize)
- ≤ 16 colours per effect
- no additive-glow assumptions: dithered falloff to transparent
- FX may cover ≤ 40 % of caster body pixels at any frame

## Cell / sheet
see BRIEF.md

## Runs
_(none yet — one line per generation: date · model or "undisclosed" · seed if known · which dirs · accepted y/n)_
