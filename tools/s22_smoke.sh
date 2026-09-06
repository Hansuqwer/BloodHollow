#!/bin/bash
# S22 smoke: choir-bot v2 profile (ch6/7/8 usage, potion priority, safe-chase
# band) on a fresh campaign leg — record + replay must stay 0 mismatches.
# Party pair-up exercises the chorus formation path; kit-v2 channels fire only
# at their chUnlock levels (server-enforced), so a short leg mostly proves the
# profile is replay-clean and the cast gates never misfire early.
set -u
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null; sleep 1
rm -f /tmp/s22_smoke.db
BH_HASH_CADENCE=25 ./build/server/bh_server --db /tmp/s22_smoke.db --port 7822 \
  --soak-secs 300 --record-world logs/s22_smoke.bwj \
  > logs/s22_smoke_server.log 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port 7822 --profile campaign --count 6 \
  --secs 240 --target-level 8 --prefix s22s_ > logs/s22_smoke_bots.log 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "S22_SMOKE_DONE bots_rc=$RC"
grep -E 'SUMMARY|maxLevel' logs/s22_smoke_bots.log | tail -2
./build/server/bh_server --replay-world logs/s22_smoke.bwj 2>/dev/null | tail -1
