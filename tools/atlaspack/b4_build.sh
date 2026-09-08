#!/usr/bin/env sh
# B4 reproducible build (mobs 1006–1010): plates → cells → sheets → QA → boss occupancy. Run from repo root.
set -e
T=tools/atlaspack; PLATE=docs/research-notes/style-tile/plate_fields_mud_b0.png; QA=docs/research-notes/qa
python3 $T/bh_mob_sheet.py 1006 --body-h 22 --family widow       --kind spider --walk-style glide --shadow-rx 14 --lunge 3 --attack-fx line8 --gamma 0.65
python3 $T/bh_mob_sheet.py 1007 --body-h 44 --family choir-wax   --walk-style glide --shadow-rx 7  --lunge 2 --asym-box 0,18,11,44 --attack-fx dots5 --n-hide-face 0.3 --gamma 0.42 --pin ember=c8622a
python3 $T/bh_mob_sheet.py 1008 --body-h 46 --family grave-goods --walk-style drag  --shadow-rx 9  --lunge 3 --asym-box 0,0,10,46 --n-hide-face 0.22
python3 $T/bh_mob_sheet.py 1010 --body-h 58 --family grave-goods --cell 40x60 --anims walk4,attack3,hurt2,die3 --shadow-rx 11 --lunge 3 --asym-box 0,0,12,58 --n-hide-face 0.2 --pin "lum>150=e6e0d4"
python3 $T/bh_mob_sheet.py 1009 --body-h 58 --family choir-wax   --cell 64x64 --kind boss --anims walk4,attack3,cast4,hurt2,die4,summon4 --walk-style bell --shadow-rx 26 --lunge 4 --quiet-band 16 --quiet-ramp 3,17,18,21
rc=0
for m in 1006_charnel_widow 1007_gravecaller 1008_revenant_sexton; do
  python3 $T/bh_qa_sheet.py assets/aigen/mobs/$m/sheet.png --json assets/aigen/mobs/$m/sheet.json --kind sheet --plate $PLATE --out $QA/b4_$m >/dev/null || { echo "QA FAIL $m"; rc=1; }
done
python3 $T/bh_qa_sheet.py assets/aigen/mobs/1010_sepulcher_elite/sheet.png --json assets/aigen/mobs/1010_sepulcher_elite/sheet.json --kind sheet --plate $PLATE --cell 40x60 --anchor-y 52 --out $QA/b4_1010_sepulcher_elite >/dev/null || { echo "QA FAIL 1010"; rc=1; }
python3 $T/bh_qa_sheet.py assets/aigen/mobs/1009_gravemother/sheet.png     --json assets/aigen/mobs/1009_gravemother/sheet.json     --kind sheet --plate $PLATE --cell 64x64 --anchor-y 58 --out $QA/b4_1009_gravemother >/dev/null || { echo "QA FAIL 1009"; rc=1; }
python3 $T/b4_boss_occupancy.py >/dev/null || { echo "BOSS OCCUPANCY FAIL"; rc=1; }
exit $rc
