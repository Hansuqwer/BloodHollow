# Asset provenance

Every asset must carry a license or be generated in-repo. Alpha gate (docs/05
section 7) requires this file to cover everything under assets/.

| Asset | Source | License |
|---|---|---|
| Placeholder hero atlas | Procedurally generated at runtime by `engine/assets/placeholder.cpp` | Project code (MIT) |
| Thornwall map | `tools/mapgen/make_thornwall.py` (deterministic generator) | Project code (MIT) |
| B0 style-tile plates ×4 (`docs/research-notes/style-tile/plates/{marsh_rat,ravager,fields_ground,dead_tree}_4x_raw.png`) — **research/approval only, not loaded by the engine** | AI-generated 2026-09-07 via Arena Agent Mode image generation (diffusion model, provider-abstracted); prompts recorded verbatim in `docs/research-notes/style-tile/plates/PROMPTS.md`; post-processed by `tools/atlaspack/make_style_tile.py` | Project-owned generated output; no third-party art referenced or traced |
| B0 style-tile composites (`docs/research-notes/style-tile/style_tile_*.png`, `cell_*.png`, `tile_*.png`, `palette_*.png`) | Derived deterministically from the plates above by `tools/atlaspack/make_style_tile.py` | Project code (MIT) |
| Ancestor reference frames (`docs/research-notes/{dark-eden,lineage1,helbreath}/*.jpg`; `mir2/mir2-web-*.jpg`, `soma/soma-web-*.jpg`) | Low-res storyboard frames / thumbnails of public YouTube videos linked by the director (IDs `-UAVwmseC8o`, `ocDVnwvBOvI`, `tJU0SoQo0-c`, `HbcNadHYQ8o`, `PUK3za0cBag`) and reduced web press screenshots (sources named in each dossier) | **Research citation only** — never shipped, never traced (bible §16) |
| `assets/aigen/**/BRIEF.md` | Design briefs + prompts, no imagery | Project docs (MIT) |
| Everything else (Sprint 2+) | not yet shipped | — |

Rules: free/placeholder packs require an entry with upstream URL + license.
AI-generated assets go to `assets/aigen/` with a note on model + date.
Human-commissioned art goes to `assets/final/` with contract reference.
