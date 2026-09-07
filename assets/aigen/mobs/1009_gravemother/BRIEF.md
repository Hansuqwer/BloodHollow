# The Gravemother — boss (1009)

**Status:** design brief only — no art generated (volume art is gated on the B0 style-tile approval).
**Spec:** `docs/art/20-mobs.md` (single source of truth for silhouette/palette/anims; do not fork it here).
**Cell / sheet:** 64×64 (anchorY 58) · walk4+attack3+cast4+hurt2+die4+summon4 → 21 cols × 8 rows = 1344×512
**Palette family:** `choir-wax (+ arterial licence)` (shared strip; build with `bhpix.build_palette` over the whole family first)

## Generation prompt (bible §13; record the *final* prompt here when the plate is made)

```
PREFIX  = "1999-era isometric MMORPG sprite, hand-drawn 2D pixel-art look, dark horror dark-fantasy, muddy desaturated earth palette (mud brown, rust red, bone off-white, soot grey, stagnant green), hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, matte dark blood, candle-light warmth only as accent, plain flat #00FF00 background, orthographic 2:1 isometric view"
SUBJECT = "The Gravemother, drowned undead matriarch fused into a great cracked verdigris bronze bell, her torso rising from the bell mouth, long arms, weed-like black hair, grave lace, black ichor veins, clapper chain hanging, massive and slow, 3/4 south-facing, single creature, boss"
NEGATIVE= "anime, chibi, cute, bright, saturated, neon on terrain, smooth gradient, 3D render, painterly character, bloom, lens flare, modern UI, text, watermark, extra limbs, asymmetry errors in gear"
```

Also delivers: rot-ring telegraph decals (3 stages), bell shockwave FX, P2/P3 palette states (3-colour swaps).

## Delivery checklist (bible §14–§15)

- [ ] 4× plate(s) → `plates/` (S, SE, E native; SW/W/NW/N/NE via img2img ≤ 0.35 denoise)
- [ ] `bhpix.key_out_green` → `harden_alpha` → `fit_to_cell` (NEAREST) → family `quantize` (Bayer-2) → `outline`
- [ ] hand-fix: outline gaps at claws/teeth/staff tips; weapon hand identical across 8 dirs
- [ ] `pack_atlas` → `sheet.png` + `sheet.json` (v1 schema) → `validate_atlas` → load via `bh::loadAtlas`
- [ ] QA triptych (day / engine night 02:00 / greyscale) → `docs/research-notes/qa/`
- [ ] R-LUMA Δ ≥ 25 vs its zone's plate; ≤ 32 opaque colours; palette strip saved
- [ ] `assets/LICENSES.md` row (model + date + this file's prompt)
