#!/usr/bin/env sh
# B2 reproducible build (Bonehowl Mine + Drowned Crypt + Thornwall Crypt):
# shared 4x raws -> plates -> D12 edges -> prism skins -> map/edge boards -> R-LUMA.
# Run from the repository root. Everything remains offline / UNVALIDATED until T-ART-12.
set -e
T=tools/atlaspack
QA=docs/research-notes/qa
rc=0
zone_dir() {
  case "$1" in
    mine) echo assets/aigen/terrain/mine ;;
    crypt_drowned) echo assets/aigen/terrain/crypt/drowned ;;
    crypt_thornwall) echo assets/aigen/terrain/crypt/thornwall ;;
  esac
}

python3 $T/bh_terrain.py mine             --variants 3 --preview >/dev/null || { echo "B2 mine FAIL"; rc=1; }
python3 $T/bh_terrain.py crypt_drowned   --variants 3 --preview >/dev/null || { echo "B2 drowned crypt FAIL"; rc=1; }
python3 $T/bh_terrain.py crypt_thornwall  --variants 3 --preview >/dev/null || { echo "B2 Thornwall crypt FAIL"; rc=1; }

python3 $T/b1_edge_board.py mine            --batch b2 >/dev/null || { echo "B2 mine edge board FAIL"; rc=1; }
python3 $T/b1_edge_board.py crypt_drowned  --batch b2 >/dev/null || { echo "B2 drowned crypt edge board FAIL"; rc=1; }
python3 $T/b1_edge_board.py crypt_thornwall --batch b2 >/dev/null || { echo "B2 Thornwall crypt edge board FAIL"; rc=1; }

# R-LUMA cross-check: every B2 zone witness stands on the actual B2 plate used by its map.
# The larger B4 sheets use their binding D5/D8 cells explicitly below.
for pair in \
  "mine 1_DIRT 1003_hollow_hound 32x48 42 0" \
  "mine 6_MUD 1004_plague_bat 32x48 42 12" \
  "mine 1_DIRT 1005_bonepicker_gnoll 32x48 42 0" \
  "mine 1_DIRT 1006_charnel_widow 32x48 42 0" \
  "crypt_drowned 1_DIRT 1007_gravecaller 32x48 42 0" \
  "crypt_drowned 1_DIRT 1008_revenant_sexton 32x48 42 0"; do
  set -- $pair; z=$1; p=$2; m=$3; cell=$4; anchor=$5; hover=$6; zd=$(zone_dir "$z")
  extra=""; [ "$hover" -gt 0 ] && extra="--hover $hover"
  python3 $T/bh_qa_sheet.py assets/aigen/mobs/$m/sheet.png --json assets/aigen/mobs/$m/sheet.json --kind sheet \
    --plate "$zd/plates/$p.png" --cell "$cell" --anchor-y "$anchor" \
    --out $QA/b2_xcheck_${z}_${p}_$m $extra >/dev/null || { echo "XCHECK FAIL $z $p $m"; rc=1; }
done

# B4 elite and boss sheet gates on the drowned crypt floor. Keeping them here makes the
# planned "try 1010/1009" explicit instead of silently dropping the larger cells.
python3 $T/bh_qa_sheet.py assets/aigen/mobs/1010_sepulcher_elite/sheet.png --json assets/aigen/mobs/1010_sepulcher_elite/sheet.json --kind sheet \
  --plate assets/aigen/terrain/crypt/drowned/plates/1_DIRT.png --cell 40x60 --anchor-y 52 \
  --out $QA/b2_xcheck_crypt_drowned_1_DIRT_1010_sepulcher_elite >/dev/null || { echo "XCHECK FAIL crypt_drowned 1010"; rc=1; }
python3 $T/bh_qa_sheet.py assets/aigen/mobs/1009_gravemother/sheet.png --json assets/aigen/mobs/1009_gravemother/sheet.json --kind sheet \
  --plate assets/aigen/terrain/crypt/drowned/plates/1_DIRT.png --cell 64x64 --anchor-y 58 \
  --out $QA/b2_xcheck_crypt_drowned_1_DIRT_1009_gravemother >/dev/null || { echo "XCHECK FAIL crypt_drowned 1009"; rc=1; }

for pair in \
  "crypt_thornwall 1_FLOOR 1007_gravecaller 32x48 42 0" \
  "crypt_thornwall 1_FLOOR 1002_feral_ghoul 32x48 42 0" \
  "crypt_thornwall 1_FLOOR 1006_charnel_widow 32x48 42 0"; do
  set -- $pair; z=$1; p=$2; m=$3; cell=$4; anchor=$5; hover=$6; zd=$(zone_dir "$z")
  extra=""; [ "$hover" -gt 0 ] && extra="--hover $hover"
  python3 $T/bh_qa_sheet.py assets/aigen/mobs/$m/sheet.png --json assets/aigen/mobs/$m/sheet.json --kind sheet \
    --plate "$zd/plates/$p.png" --cell "$cell" --anchor-y "$anchor" \
    --out $QA/b2_xcheck_${z}_${p}_$m $extra >/dev/null || { echo "XCHECK FAIL $z $p $m"; rc=1; }
done

exit $rc
