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
- `bhscene.py` — offline compositor replicating the client's draw order
  (ground → decals → y-sorted entities → FX → text → night overlay).
- `bh_qa_sheet.py <png> [--json] [--plate] [--kind cell|sheet|plate]` — §15
  gates for one asset: R-LUMA (body excl. outline) day + night, colour count,
  loader-rule replica, feet-on-anchor; writes `<name>_qa_3x.png` + `_audit.json`
  to `docs/research-notes/qa/`; exit 1 on any failed gate.
- `bh_pack_sheet.py <cells_dir> <out_dir>` — `<anim>_<DIR>_<frame>.png` cells →
  `sheet.png` + `sheet.json` (v1 + `anchorY`) + `pack_report.json`;
  `--pad-missing` for WIP sheets (never ship padded).
- `bhfont.py` — 5×7 / 7×11 red-caps bitmap font (callouts); `python3 bhfont.py`
  re-exports the strips + JSON + specimen into `style-tile/export/font/`.
- `make_edge_preview.py` — A12: 8 transition masks per adjacency pair
  (`bhpix.edge_masks`, faces NE/SE/SW/NW + points N/E/S/W, iso-correct per
  `iso.cpp`), autotile union (`edge_mask_for`), first-cut composer
  (`blend_edge`); board + masks → `style-tile/export/edges/`.
- `map_manifest.py [map]` — read-only `.tmj` → `assets/aigen/terrain/<zone>/MAPS_*`.
- `make_decision_board.py`, `make_crowd_test.py` — provisional QA boards from
  the B0 plates (director inputs; regenerate with real sheets later).

Batch flow: plates → `bhpix` chain → cells dir → `bh_pack_sheet.py` →
`bh_qa_sheet.py --json` → triptych into `docs/research-notes/qa/` →
`assets/LICENSES.md` row → drop.

Conventions: plates at 4× named `<slug>_4x_raw.png`; keyed intermediates
`<slug>_keyed.png`; per-family palettes `palette_<family>.png`; QA triptychs
`<slug>_qa_3x.png` (day | night 02:00 | greyscale) archived under
`docs/research-notes/qa/`.
