#!/bin/bash
# M2b campaign leg runner (S12): restart-safe leg of the L1->8 cross-zone gate.
# usage: m2b_leg.sh <legN> <port> [secs]
# - same DB across legs (persistence = wipe proof of character state)
# - same bot names -> same characters resume at their persisted levels
# - journal per leg; replay must be bit-exact
set -u
LEG=${1:?leg number}
PORT=${2:?port}
SECS=${3:-1720}
cd /home/user/bloodhollow || exit 1
pkill -x bh_server 2>/dev/null; sleep 1
./build/server/bh_server --db /tmp/m2b_campaign.db --port "$PORT" \
  --soak-secs $((SECS + 60)) --record-world "logs/m2b_leg${LEG}.bwj" \
  > "logs/m2b_leg${LEG}_server.log" 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port "$PORT" --profile campaign --count 2 \
  --secs "$SECS" --target-level 8 --prefix m2b_ > "logs/m2b_leg${LEG}_bots.log" 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "LEG${LEG}_DONE bots_rc=$RC"
./build/server/bh_server --replay-world "logs/m2b_leg${LEG}.bwj" 2>/dev/null | tail -1
