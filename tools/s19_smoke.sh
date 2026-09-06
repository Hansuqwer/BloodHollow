#!/bin/bash
# S15 smoke: epoch-3 journal + replay on a short campaign leg (alignment wire
# live: karmaBand in EntitySpawn, ch-255 crossing lines, duel party traffic).
set -u
cd /home/user/bloodhollow || exit 1
pkill -x bh_server 2>/dev/null; sleep 1
rm -f /tmp/s19_smoke.db
./build/server/bh_server --db /tmp/s19_smoke.db --port 7819 \
  --soak-secs 150 --record-world logs/s19_smoke.bwj \
  > logs/s19_smoke_server.log 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port 7819 --profile campaign --count 2 \
  --secs 120 --target-level 3 --prefix s19s_ > logs/s19_smoke_bots.log 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "S15_SMOKE_DONE bots_rc=$RC"
./build/server/bh_server --replay-world logs/s19_smoke.bwj 2>/dev/null | tail -1
