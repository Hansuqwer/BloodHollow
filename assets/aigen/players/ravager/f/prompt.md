# prompt.md — Ravager — female base (ravager/f)

**Status:** DRAFT prompt package (B0.5 A1). No plate generated. Final prompts are appended verbatim under *Runs* when generation happens; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

## Prompt

```
PREFIX   = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT  = "female human warrior with heavy shoulders, soot-grey padded gambeson with rusted iron plates nailed on, butcher-chain belt with iron hooks, oversized notched cleaver-axe held low in the right hand (15 percent oversized), pale scarred skin, close-cropped dark hair, grim, standing idle, full body"
PALETTE  = "soot gambeson #3a3634, rust plates #5a3a2a, iron chain #4a4e58, bone #c9bfae, skin (3 tones) #d6c6be / #b8a48a / #8e6e52, cleaver steel #6a6e78"
NEGATIVE = "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear, elf ears, sparkle, design-tool purple"
```

Full prompt = PREFIX + ", " + SUBJECT + ", palette: " + PALETTE + " --neg " + NEGATIVE

## Expansion
- native dirs: S, SE, E for the BASE BODY only (no gear); class overlay generated on the same body plate with the same seed
- skin tones: generate the pale tone only; sallow/weathered are index swaps of the 4 skin entries in the family strip
- frames: idle1 walk6 (contact f0/f3) attack3 (f1) cast4 (release f2) hurt2 die4 gib3 — key frames by AI (walk f0, attack f1, cast f2, die f3), the rest hand-inbetweened
- cell: 32×48, body ≤ 43 px (rows 0–42) — D2 ruled 32×48 (B0-GATE-DECISION); fit_to_cell(43), never 46 (clips)

## Hand-fix list (check in this order after quantize)
- outline: close every gap at claws / teeth / tail tip / weapon tip / staff head (AI breaks these first)
- weapon / attachment stays in the RIGHT hand in all 8 dirs — derive W-side dirs by img2img, never mirror
- feet: lowest opaque body row on the anchor row; no floating shadows painted into the plate (we add the alpha-70 ellipse)
- kill any pixel above luma 200 that is not a light source, eye glint, hit-flash or bone highlight
- no accent hue (violet/arterial/choir-gold) on skin, cloth or terrain-coloured gear
- shoulder width per class (Ravager 14 / Cultist 11 / Gravecaller 10 px) measured on the S frame after downscale — adjust by hand, not by re-prompt

## Cell / sheet
32×48 (body ≤ 43 px; D2 ruled) · idle1+walk6+attack3+cast4+hurt2+die4+gib3 → 23 cols × 8 rows = 736×384

## Runs

- 2026-09-10 · undisclosed · seed_auto · 10-plate batch (walk-f0 S/SE/E, die-f3 S/SE/E, attack-f1 SE/E, cast-f2 SE/E) · REJECTED, acceptance struck: S/SE/E byte-identical copies (8 unique images), 6–9 colors, placeholder grade — false "accepted y" lines removed, files quarantined to /tmp/opencode/rejected-plates/f/ + MANIFEST.txt, superseded by keyed regen below
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1011 · ravager_f_walk_f0_S_4x_raw.png · accepted y (spot-checked: walk pose, axe right, green bg + painted shadow → hand-fix)
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1012 · ravager_f_walk_f0_SE_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1013 · ravager_f_walk_f0_E_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1014 · ravager_f_die_f3_S_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1015 · ravager_f_die_f3_SE_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1016 · ravager_f_die_f3_E_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1017 · ravager_f_attack_f1_SE_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1018 · ravager_f_attack_f1_E_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1019 · ravager_f_cast_f2_SE_4x_raw.png · accepted y
- 2026-09-10 · flux (gen.pollinations.ai, keyed, 512x768, hardened pixel/green-screen prompt) · seed 1020 · ravager_f_cast_f2_E_4x_raw.png · accepted y
