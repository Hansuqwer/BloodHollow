#!/usr/bin/env sh
# B1 reproducible build (terrain town + fields): raw 4x paints → plates → D12 edges → prism skins → QA. Run from repo root.
set -e
T=tools/atlaspack
rc=0
python3 $T/bh_terrain.py town   --variants 3 --preview >/dev/null || { echo "B1 town FAIL"; rc=1; }
python3 $T/bh_terrain.py fields --variants 3 --preview >/dev/null || { echo "B1 fields FAIL"; rc=1; }
python3 $T/b1_edge_board.py town   >/dev/null
python3 $T/b1_edge_board.py fields >/dev/null
# R-LUMA cross-check: every mob that spawns in the zone vs the zone's main plates (sheet gate must still exit 0)
PL=docs/research-notes/qa
for pair in "town 0_GRASS 1002_feral_ghoul" "town 5_PATH 1001_marsh_rat" "town 6_MUD 1004_plague_bat --hover 12" \
            "fields 0_GRASS 1007_gravecaller" "fields 0_GRASS 1005_bonepicker_gnoll" "fields 6_MUD 1003_hollow_hound" "fields 5_PATH 1006_charnel_widow"; do
  set -- $pair; z=$1; p=$2; m=$3; shift 3
  python3 $T/bh_qa_sheet.py assets/aigen/mobs/$m/sheet.png --json assets/aigen/mobs/$m/sheet.json --kind sheet \
    --plate assets/aigen/terrain/$z/plates/$p.png --out $PL/b1_xcheck_${z}_${p}_$m "$@" >/dev/null || { echo "XCHECK FAIL $z $p $m"; rc=1; }
done
exit $rc
