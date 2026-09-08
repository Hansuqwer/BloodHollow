#!/usr/bin/env sh
# B3 reproducible build (mobs 1001–1005): plates → cells → sheets → QA. Run from repo root.
set -e
T=tools/atlaspack; PLATE=docs/research-notes/style-tile/plate_fields_mud_b0.png; QA=docs/research-notes/qa
python3 $T/bh_mob_sheet.py 1001 --body-h 20 --family vermin      --lunge 3 --shadow-rx 9  --match-to SE
python3 $T/bh_mob_sheet.py 1002 --body-h 40 --family ghoul-flesh --lunge 2 --shadow-rx 6
python3 $T/bh_mob_sheet.py 1003 --body-h 26 --family hound       --lunge 4 --shadow-rx 11 --gamma 0.72
python3 $T/bh_mob_sheet.py 1004 --body-h 18 --family vermin      --lunge 2 --shadow-rx 7  --gamma 0.62 --kind fly --hover 12
python3 $T/bh_mob_sheet.py 1005 --body-h 42 --family grave-goods --lunge 3 --shadow-rx 10 --asym-box 0,0,15,13
rc=0
for m in 1001_marsh_rat 1002_feral_ghoul 1003_hollow_hound 1004_plague_bat 1005_bonepicker_gnoll; do
  hv=""; [ "$m" = 1004_plague_bat ] && hv="--hover 12"
  python3 $T/bh_qa_sheet.py assets/aigen/mobs/$m/sheet.png --json assets/aigen/mobs/$m/sheet.json --kind sheet --plate $PLATE --out $QA/b3_$m $hv >/dev/null || { echo "QA FAIL $m"; rc=1; }
done
exit $rc
