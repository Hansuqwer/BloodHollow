# Asset provenance

Every asset must carry a license or be generated in-repo. Alpha gate (docs/05
section 7) requires this file to cover everything under assets/.

| Asset | Source | License |
|---|---|---|
| Placeholder hero atlas | Procedurally generated at runtime by `engine/assets/placeholder.cpp` | Project code (MIT) |
| Thornwall map | `tools/mapgen/make_thornwall.py` (deterministic generator) | Project code (MIT) |
| Everything else (Sprint 2) | not yet shipped | — |

Rules: free/placeholder packs require an entry with upstream URL + license.
AI-generated assets go to `assets/aigen/` with a note on model + date.
Human-commissioned art goes to `assets/final/` with contract reference.
