#!/usr/bin/env bash
# b5_build.sh — regenerate the B5 procedural NPC placeholder sheets + QA.
#
# Usage: bash tools/atlaspack/b5_build.sh
#
# Output:
#   assets/aigen/npcs/<slug>/{sheet.png,sheet.json,palette.png,portrait.png}
#   docs/research-notes/qa/b5_*_{qa_3x.png,_audit.json}
#
# NOTE: these are procedural PLACEHOLDERS (drawn by b5_npc_proxy.py using
# Pillow + bhpix primitives). When final B5 AI plates ship they overwrite
# these PNGs in place (same folder contract / sheet.json schema) — no loader
# changes required.
set -euo pipefail
cd "$(dirname "$0")/../.."

echo "[b5] generating procedural NPC sheets"
python3 tools/atlaspack/b5_npc_proxy.py

echo "[b5] running bh_qa_sheet on every sheet"
for spec in \
    "bonesmith_twins:64x48" \
    "confessor:32x48" \
    "cove_fence:32x48" \
    "guard_ashen:32x48" \
    "guard_synod:32x48" \
    "pledge_registrar:32x48" \
    "castle_steward:32x48"; do
  name="${spec%:*}"
  cell="${spec#*:}"
  python3 tools/atlaspack/bh_qa_sheet.py \
    "assets/aigen/npcs/${name}/sheet.png" \
    --json "assets/aigen/npcs/${name}/sheet.json" \
    --kind sheet --cell "$cell" --terrain-luma 51 \
    --out "docs/research-notes/qa/b5_${name}" >/dev/null \
    && echo "  [PASS] ${name}"
done

echo "[b5] done."
