#!/bin/bash
# Merge-repair gate leg (epoch 27): the H1/H2 mine+steward lane merged under
# epoch-26 journals shifts the entity set (ore nodes map 4, steward map 6;
# T-068 precedent) — t138/t140 re-replay mismatched. Fresh leg proves the
# merged tree replays bit-exact at 27; old journals must refuse exit 4.
#   wave 1: 5 wander bots 10s (accounts created)
set -e
PORT=${1:-7831}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t146.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t146.bwj logs/t146_server.log logs/t146_bots.log
mkdir -p logs
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[t146] Server up (record epoch 27)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 40 --record-world logs/t146.bwj > logs/t146_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t146] Wave 1: 5 wander bots 10s..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t146_ > logs/t146_bots.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t146] Replay (expect mm=0)..."
./$BUILD_DIR/server/bh_server --replay-world logs/t146.bwj 2>&1 | tail -n 3

echo "[t146] Guard (old epoch-26 journal must refuse exit 4)..."
if ./$BUILD_DIR/server/bh_server --replay-world logs/t140.bwj > /dev/null 2>&1; then
  echo "[t146] FAIL: t140 (epoch 26) replayed under epoch 27 (expected refusal)"
  exit 1
else
  rc=$?
  if [ "$rc" -eq 4 ]; then
    echo "[t146] guard OK: t140 refused exit 4"
  else
    echo "[t146] FAIL: t140 exited $rc (expected 4)"
    exit 1
  fi
fi
echo "[t146] DONE"
