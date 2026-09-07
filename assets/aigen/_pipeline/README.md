# assets/aigen/_pipeline

Working files for the AI-assisted pipeline (plates, keyed PNGs, QA sheets)
before they are packed into a delivery folder. Nothing in here is loaded by
the engine. The code lives in `tools/atlaspack/`:

- `bhpix.py` — chroma-key, alpha hardening, NEAREST fit, ≤32-colour median-cut
  palette + Bayer ordered dither, 1 px outline, 32×48 cell placement (feet
  y=42, alpha-70 contact ellipse), `pack_atlas`/`validate_atlas` (anim.json v1,
  8 dir rows E,SE,S,SW,W,NW,N,NE), `night_overlay`/`night_floor` (port of
  `engine/render/daynight.cpp` keyframes), `cut_diamond`-style plate cutting,
  palette strips, colour counts, greyscale.
- `make_style_tile.py` — the B0 style-lock scene (reference implementation of
  the whole chain; copy its steps for every batch).

Conventions: plates at 4× named `<slug>_4x_raw.png`; keyed intermediates
`<slug>_keyed.png`; per-family palettes `palette_<family>.png`; QA triptychs
`<slug>_qa_3x.png` (day | night 02:00 | greyscale) archived under
`docs/research-notes/qa/`.
