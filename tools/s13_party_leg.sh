#!/bin/bash
# S13 gate leg: 2 auto-partied campaign bots L1->5, fresh DB, journal + replay.
# Party pair-up fires at campaign t0 (bots/main.cpp): /invite + /accept via
# chat slash verbs -> id-resolved party Commands in the journal.
# usage: s13_party_leg.sh [port] [secs]
set -u
PORT=${1:-7813}
SECS=${2:-600}
cd /home/user/bloodhollow || exit 1
pkill -x bh_server 2>/dev/null; sleep 1
rm -f /tmp/s13_party.db
./build/server/bh_server --db /tmp/s13_party.db --port "$PORT" \
  --soak-secs $((SECS + 60)) --record-world "logs/s13_party_leg.bwj" \
  > "logs/s13_party_leg_server.log" 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port "$PORT" --profile campaign --count 2 \
  --secs "$SECS" --target-level 5 --prefix s13p_ \
  > "logs/s13_party_leg_bots.log" 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "S13_LEG_DONE bots_rc=$RC"
./build/server/bh_server --replay-world "logs/s13_party_leg.bwj" 2>/dev/null | tail -1
