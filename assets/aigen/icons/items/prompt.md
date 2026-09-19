# prompt.md — Item icons (top-10 batch, B7.5)

## Prompt

```
PREFIX   = "1999-era isometric MMORPG inventory icon, hand-drawn 2D pixel-art look, muddy desaturated earth tones, hard 1px dark outline, visible dither shading, no anti-aliasing, no gradients, single centered item"
SUBJECT  = "see Runs (one line per icon)"
PALETTE  = "per-item family (iron-red / ash-blue / bone-white / gold)"
NEGATIVE = "solid flat pure-green chroma-key RGB 0,255,0 backdrop, no ground, absolutely no shadow, no text, no watermark, 16-bit pixel sprite"
```

Full prompt = PREFIX + ", " + SUBJECT + ", " + PALETTE + ", " + NEGATIVE.
Model: black-forest-labs/flux.1-schnell (Pollinations gateway, repo key),
256x256, one generation per icon (no regens).

## Expansion
- border-median chroma key + 1px fringe erode, NEAREST fit to 28px on a
  32 cell, MEDIANCUT ≤32, pipeline `#151013` plate + 2px family bevel,
  NEAREST 24px, palette strip, greyscale silhouette check.

## Runs (2026-09-19, 10/10 fresh-cap batch on the repo key)

Deviations (all accepted, read as icon bases at 32px): extra props on
2002 (brick), 2003 (ore pile + coin), 2501 (stone pedestal), gold (grassy
mound + stone base); 2001 subject renders thin (10px wide); 2102 pauldron
micro-glyphs vanish at 32px. No regens (cap reached).
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990201 · 2001_rusty_shank (pitted short iron blade with rag grip, iron-red) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990202 · 2002_pit_blade (heavy notched cleaver with teeth on spine, iron-red) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990203 · 2003_mine_pick (worn miner pickaxe with wooden haft, ash-blue grey) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990204 · 2101_hide_armor (stitched hide vest, grey-pink leather) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990205 · 2102_bone_plate (rib-and-scapula bone plate with wax seals, bone-white) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990206 · 2501_scrap_helm (dented scrap-metal helmet, ash-blue grey) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990207 · 3001_blood_vial (small glass vial of dark red blood, bone-white and arterial) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990208 · 3003_torch (burning wooden torch with ember flame, gold and ember) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990209 · 3004_lantern (brass lantern with candle glow, gold) · accepted y
- 2026-09-19 · black-forest-labs/flux.1-schnell · seed 19990210 · gold_coin (black-iron coin with skull stamp, gold) · accepted y
