#!/bin/bash
# T-034d smoke: campaign re-gear trip — 2-bot campaign leg must walk home to
# buy gear (regear counter > 0 in SUMMARY) and record + replay must stay
# 0 mismatches.
set -u
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null; sleep 1
rm -f /tmp/t034d_smoke.db
BH_HASH_CADENCE=25 ./build/server/bh_server --db /tmp/t034d_smoke.db --port 7823 \
  --soak-secs 300 --record-world logs/t034d_smoke.bwj \
  > logs/t034d_smoke_server.log 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port 7823 --profile campaign --count 2 \
  --secs 240 --target-level 8 --prefix t34d_ > logs/t034d_smoke_bots.log 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "T034D_SMOKE_DONE bots_rc=$RC"
grep -E 'SUMMARY|maxLevel|regear' logs/t034d_smoke_bots.log | tail -4
./build/server/bh_server --replay-world logs/t034d_smoke.bwj 2>/dev/null | tail -1
