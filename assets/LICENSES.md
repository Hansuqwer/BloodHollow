# Asset provenance

Every asset must carry a license or be generated in-repo. Alpha gate (docs/05
section 7) requires this file to cover everything under assets/.

| Asset | Source | License |
|---|---|---|
| Placeholder hero atlas | Procedurally generated at runtime by `engine/assets/placeholder.cpp` | Project code (MIT) |
| Thornwall map | `tools/mapgen/make_thornwall.py` (deterministic generator) | Project code (MIT) |
| B0 style-tile plates ×4 (`docs/research-notes/style-tile/plates/{marsh_rat,ravager,fields_ground,dead_tree}_4x_raw.png`) — **research/approval only, not loaded by the engine** | AI-generated 2026-09-07 via Arena Agent Mode image generation (model family: diffusion, provider-abstracted; **exact model/version undisclosed at generation time**); prompts recorded verbatim in `docs/research-notes/style-tile/plates/PROMPTS.md`; post-processed by `tools/atlaspack/make_style_tile.py` | Project-owned generated output; no third-party art referenced or traced |
| B0 style-tile composites (`docs/research-notes/style-tile/style_tile_*.png`, `cell_*.png`, `tile_*.png`, `palette_*.png`) | Derived deterministically from the plates above by `tools/atlaspack/make_style_tile.py` | Project code (MIT) |
| Ancestor reference frames (`docs/research-notes/{dark-eden,lineage1,helbreath}/*.jpg`; `mir2/mir2-web-*.jpg`, `soma/soma-web-*.jpg`) | Low-res storyboard frames / thumbnails of public YouTube videos linked by the director (IDs `-UAVwmseC8o`, `ocDVnwvBOvI`, `tJU0SoQo0-c`, `HbcNadHYQ8o`, `PUK3za0cBag`) and reduced web press screenshots (sources named in each dossier) | **Research citation only** — never shipped, never traced (bible §16) |
| `assets/aigen/**/BRIEF.md`, `**/prompt.md`, `**/atlas.draft*.json`, `REGISTRY.md`, `terrain/*/MAPS_*` | Design briefs, prompt packages, atlas layout drafts, registry, map manifests (read from `data/maps-src/*.tmj`) — text/JSON, no imagery except `MAPS_*_preview.png` (1 px/tile schematic) | Project docs (MIT) |
| `assets/aigen/palettes/*.png`, `families.json`, `families_board.png` | Hand-authored ≤32-colour family ramps (B0.5 A3, 2026-09-07); no AI generation | Project code (MIT) |
| `docs/research-notes/style-tile/export/decision_board_3x.png` (+ `_audit.json`) | **PROVISIONAL** decision-input board, 2026-09-07; derived from the four B0 plates above (model **undisclosed**, see that row) by `tools/atlaspack/make_decision_board.py`; no new generation | Project code (MIT); research only |
| `docs/research-notes/style-tile/export/pools/*` | Procedural warm light-pool decals (`numpy` radial + Bayer), 2026-09-07; no AI | Project code (MIT); provisional |
| `docs/research-notes/style-tile/export/edges/*` | **PROVISIONAL** A12 edge-mask preview (`tools/atlaspack/make_edge_preview.py`), 2026-09-07: 8 procedural masks + board; material A = `plate_fields_mud_b0.png` (derived from the B0 plates, model **undisclosed**, see that row), material B = flat client placeholder colour; no new AI generation | Masks/board: project code (MIT); plate-derived pixels inherit the B0 plate row |
| `docs/research-notes/style-tile/export/font/*` | Hand-authored 5×7 / 7×11 red-caps bitmap font drafts (`tools/atlaspack/bhfont.py`), 2026-09-07; no AI | Project code (MIT) |
| `docs/research-notes/style-tile/plate_fields_mud_b0.png`, `docs/research-notes/qa/*` | QA composites derived from the B0 plates by `bh_qa_sheet.py` / `make_crowd_test.py`; `luma-audit.json` = numbers only from the reference frames | research only; nothing shipped |
| Everything else (Sprint 2+) | not yet shipped | — |

Rules: free/placeholder packs require an entry with upstream URL + license.
AI-generated assets go to `assets/aigen/` with a note on model + date.
Human-commissioned art goes to `assets/final/` with contract reference.
