#!/bin/bash
# ADR-0016 epoch-31 gate leg: multi-affix rolls + 45-row band tables.
# Epoch 30->31: 10-field blob + roll stream + worldHash widening + wire 242.
#   wave 1: 8 fighter bots 60s (fresh accounts -> creation -> kills exercise
#             the multi-affix roll + band tables)
#   wave 2: same accounts wander 10s (relog restores 10-field blobs)
set -e
PORT=${1:-7845}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t159f1.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t159f1.bwj logs/t159f1_server.log logs/t159f1_bots1.log logs/t159f1_bots2.log
mkdir -p logs
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[t159f1] Server up (record epoch 31)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 120 --record-world logs/t159f1.bwj > logs/t159f1_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t159f1] Wave 1: 8 fighters 60s (creation + multi-affix rolls)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 8 --secs 60 --profile fighter --prefix f1_ > logs/t159f1_bots1.log 2>&1 || true
sleep 2

echo "[t159f1] Wave 2: relog wander 10s (10-field blob restore)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 8 --secs 10 --profile wander --prefix f1_ > logs/t159f1_bots2.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t159f1] Replay (expect mm=0)..."
./$BUILD_DIR/server/bh_server --replay-world logs/t159f1.bwj 2>&1 | tail -n 3

echo "[t159f1] Guard (old epoch-30 journal must refuse exit 4)..."
if ./$BUILD_DIR/server/bh_server --replay-world logs/wave2.bwj > /dev/null 2>&1; then
  echo "[t159f1] FAIL: wave2 (epoch 30) replayed under epoch 31 (expected refusal)"
  exit 1
else
  rc=$?
  if [ "$rc" -eq 4 ]; then
    echo "[t159f1] guard OK: wave2 refused exit 4"
  else
    echo "[t159f1] FAIL: wave2 exited $rc (expected 4)"
    exit 1
  fi
fi
