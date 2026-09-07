# Charnel Widow (1006)

**Status:** design brief only — no art generated (volume art is gated on the B0 style-tile approval).
**Spec:** `docs/art/20-mobs.md` (single source of truth for silhouette/palette/anims; do not fork it here).
**Cell / sheet:** 32×48 (legs span 32 px, body 22 px) · 10 cols × 8 rows = 320×384
**Palette family:** `widow` (shared strip; build with `bhpix.build_palette` over the whole family first)

## Generation prompt (bible §13; record the *final* prompt here when the plate is made)

```
PREFIX  = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT = "Charnel Widow, bloated grave spider, its abdomen is a fused wooden coffin lid with brass nails, black chitin legs with grey-pink joints, trailing grey shroud-thread silk, eight bone-white eyes, low deliberate stance, 3/4 view from above, single creature"
NEGATIVE= "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear"
```

## Delivery checklist (bible §14–§15)

- [ ] 4× plate(s) → `plates/` (S, SE, E native; SW/W/NW/N/NE via img2img ≤ 0.35 denoise)
- [ ] `bhpix.key_out_green` → `harden_alpha` → `fit_to_cell` (NEAREST) → family `quantize` (Bayer-2) → `outline`
- [ ] hand-fix: outline gaps at claws/teeth/staff tips; weapon hand identical across 8 dirs
- [ ] `pack_atlas` → `sheet.png` + `sheet.json` (v1 schema) → `validate_atlas` → load via `bh::loadAtlas`
- [ ] QA triptych (day / engine night 02:00 / greyscale) → `docs/research-notes/qa/`
- [ ] R-LUMA Δ ≥ 25 vs its zone's plate; ≤ 32 opaque colours; palette strip saved
- [ ] `assets/LICENSES.md` row (model + date + this file's prompt)
