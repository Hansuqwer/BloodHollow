# prompt.md — Gravecaller — male base (gravecaller/m)

**Status:** B6 keyframes 18/20 GENERATED (batches 1–2, 2026-09-14; 1774×887 keyed `#00FF00`). Remaining AI keyframes: cast_f2 E only (1). Offline contact-sheet QA passes (`docs/research-notes/qa/turn2026-09-14_batch{1,2}_montage.png`); in-engine UNVALIDATED. Final prompts are appended verbatim under *Runs*; the model/version goes to `assets/LICENSES.md` (write "undisclosed" if the provider hides it — never omit).

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

- walk_f0 S · 2026-09-14 · Arena Agent Mode image generation, model **undisclosed** (provider-abstracted), 1774×887 keyed, hardened pixel/green-screen prompt (Ravager-regen pattern) · seed n/a · SUBJECT + "walking mid-stride contact pose, left leg forward, facing the viewer front-on (SOUTH)" · accepted **y** (knife RIGHT, censer LEFT, robe stride, no painted shadow)
- walk_f0 SE · 2026-09-14 · same provider, model **undisclosed** · seed n/a · SUBJECT + walk contact pose, "3/4 view facing SOUTH-EAST" · accepted **y** (batch 1)
- walk_f0 E · 2026-09-14 · same provider, model **undisclosed** · seed n/a · SUBJECT + walk contact pose, "strict side profile facing EAST" · accepted **y** (batch 1)
- die_f3 S · 2026-09-14 · same provider, model **undisclosed** · seed n/a · SUBJECT + "final death frame: collapsed and crumpled face-down, robe pooled, censer fallen beside the left hand, knife dropped" · accepted **y** (batch 1)
- die_f3 SE · 2026-09-14 · same provider, model **undisclosed** · seed n/a · die prompt + "3/4 view oriented toward the SOUTH-EAST" · accepted **y** (batch 2)
- die_f3 E · 2026-09-14 · same provider, model **undisclosed** · seed n/a · die prompt + "strict side profile oriented toward the EAST" · accepted **y** (batch 2)
- attack_f1 SE · 2026-09-14 · same provider, model **undisclosed** · seed n/a · SUBJECT + "melee attack contact frame: lunging forward, short bone knife thrusting in the RIGHT hand, censer held in the LEFT" · accepted **y** (batch 2)
- attack_f1 E · 2026-09-14 · same provider, model **undisclosed** · seed n/a · attack prompt + "strict side profile facing EAST" · accepted **y** (batch 2)
- cast_f2 SE · 2026-09-14 · same provider, model **undisclosed** · seed n/a · SUBJECT + "spell release frame: censer swung raised aloft in the LEFT hand, ember glow at the mouth, knife held low in the RIGHT" · accepted **y** (batch 2)
- Batches 1–2: 10/10 distinct (md5), knife RIGHT verified on all attack/cast plates; contact sheets `docs/research-notes/qa/turn2026-09-14_batch{1,2}_montage.png`. Remaining: cast_f2 E (1 plate) — then Cultist starts. *(Batch-1 lines were lost to a concurrent-edit write race in commit 73de497 — re-recorded verbatim here.)*
- 2026-09-15 · flux (black-forest-labs/flux.1-schnell via gen.pollinations.ai, 512x768) · seeds 2001/2002/2003 (S/SE/E idle) + 2007 (attack_f1_S) + 2009 (cast_f2_S) · REJECTED all 5: 0.00 green bg (grey/white instead of #00FF00), painterly 3D-render not pixel-art, ground discs + shadows, wrong-hand gear; quarantined to /tmp/opencode/rejected-plates/turn1-gravecaller/ + MANIFEST.txt
- 2026-09-17 · flux (black-forest-labs/flux.1-schnell via gen.pollinations.ai, 512x768) · probe #1 gravecaller_m_S (hardened chroma wording: solid flat pure-green chroma-key RGB 0,255,0 backdrop, no ground, no shadow, 16-bit pixel sprite) · REJECTED: green_frac 0.018, corner #398634 not #00FF00, painterly still; quarantined to /tmp/probe_gravecaller_m_S.jpg (md5 24ddf6ee), not in plates/; 1/10 turn consumed
- 2026-09-17 · flux (black-forest-labs/flux.1-schnell via gen.pollinations.ai, 512x768) · probe #2 gravecaller_m_S (explicit flat green #00FF00 uniform, no shading/ground/shadow/terrain, empty green void, hard 1px outline #1a1214) · ACCEPTED y (green_frac 0.88, single figure, feet visible; JPEG md5 5f091dce → PNG 154791 md5 58d2d8cb): overwrote `plates/gravecaller_m_S_4x_raw.png`; 2/10 turn consumed, 8 remain — probe PASSED, batch may proceed next turn with hardened wording (right-hand + pixel-art manual check pending)
