# prompt.md — Bonepicker Gnoll (1005)

**Status:** B3 first-pass GENERATED 2026-09-08 (native S + E plates; SE/SW/W/NW/N/NE derived). Offline QA pass; in-engine UNVALIDATED. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "Bonepicker Gnoll, hyena-headed man wearing grave goods, a human rib cage strapped on as armour with rusted wire, a femur club over the right shoulder, hunched knuckle-walking stance, matted soot mane, laughing snout, 3/4 south-facing, single creature"
PALETTE  = "hyena tan-grey #6a5e4e, soot mane #2b2628, rib armour bone #cfc6b4, grave-wire rust #6a3a26, rust plate #5a3a2a, iron #4a4e58, brass keys #8a6a3a, grave-gold #a88a4a"
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
- gnoll: club over the RIGHT shoulder; the laugh tell is frame 0 of attack (jaw open) — draw jaw as a separate 2-frame overlay

## Cell / sheet
32×48 (body 42 px) · 10 cols × 8 rows = 320×384

## Runs
- S (rev 1) · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md, club raised over the RIGHT shoulder — the club above the head ate the 42-px height budget (body fit 34) · accepted **n** → superseded by rev 2 (plate overwritten)
- S (rev 2) · 2026-09-08 · same provider, model **undisclosed** · seed n/a · prompt.md + pose line "club held **horizontal across the shoulders in the RIGHT hand**, hunched" → keyed 558×641 (w/h 0.87), body fit 37 (width-limited by the club length) · accepted **y** (B3 rev 2)
- E · 2026-09-08 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted) · seed n/a · prompt.md, side profile facing EAST, club on near shoulder · accepted **y** (B3 first pass)
- SE (except 1001) · not generated — derived from S/E by `bh_mob_sheet.py` (see `derivation.json`); a native SE plate is the first upgrade when generation budget allows
- Build: `sh tools/atlaspack/b3_build.sh` → `cells/` (80) → `sheet.png` + `sheet.json` → `docs/research-notes/qa/b3_<slug>_qa_3x.png` + `_audit.json` (exit 0, 2026-09-08). **UNVALIDATED in engine** (T-ART-01/05).
