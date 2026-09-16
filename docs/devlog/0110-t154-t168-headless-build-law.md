# 0110 — T-154/T-168 headless gate + build-dir law

2026-09-16. Combined branch (overlap). Legs honour `BH_BUILD_DIR` (default unchanged); 4 mapgens gain `--out` (byte-identical diffs); CI gains 6/6 mapgen gate + `headless` job (no X11) + t159 replay guard; `tools/clean_clone_check.sh` → PASS on fresh clone (replay mm=0). T-154 core (raylib-gated TUs) was already in tree — verified, not redone. Caught: unquoted `:` broke ci.yml (yaml.safe_load now OK). Card `docs/tasks/T-154-headless-build-law.md`. Next: T-156 T-ART-12..15.
