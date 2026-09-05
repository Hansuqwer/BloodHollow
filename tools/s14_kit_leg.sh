#!/bin/bash
# S14 dual-kit gate leg (T-055): Ravager + Pale Choir cultist pair, L1->5,
# fresh DB, journal + replay. Bencharmark = S13 two-Ravager leg
# (556/583 s to L5, 36 deaths). The Cultist claim to prove: fewer deaths,
# equal-or-better pace — "a party WITH one out-survives one without".
# usage: s14_kit_leg.sh [port] [secs]
set -u
PORT=${1:-7814}
SECS=${2:-600}
cd /home/user/bloodhollow || exit 1
pkill -x bh_server 2>/dev/null; sleep 1
rm -f /tmp/s14_kit.db
./build/server/bh_server --db /tmp/s14_kit.db --port "$PORT" \
  --soak-secs $((SECS + 60)) --record-world "logs/s14_kit_leg.bwj" \
  > "logs/s14_kit_leg_server.log" 2>&1 &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port "$PORT" --profile campaign --count 2 \
  --secs "$SECS" --target-level 5 --prefix s14k_ \
  > "logs/s14_kit_leg_bots.log" 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "S14_LEG_DONE bots_rc=$RC"
./build/server/bh_server --replay-world "logs/s14_kit_leg.bwj" 2>/dev/null | tail -1
