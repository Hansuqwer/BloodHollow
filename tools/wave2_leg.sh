#!/bin/bash
# Wave-2 epoch-30 gate leg: creation flow + new roster + relog restore, replay mm=0.
# Epoch 29->30: T-161 rebate stamps + T-162 night premium/roster + T-163 EK law +
# T-166 bounty persistence + T-167 creation/sex + worldHash widening + wire 244.
#   wave 1: 8 fighter bots 60s (FRESH accounts -> CharCreatePrompt answered ->
#             spawn with class/sex; kills exercise the new roster + rarity roll)
#   wave 2: same accounts wander 10s (relog restores v15 fields via y-sidecar)
set -e
PORT=${1:-7843}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/wave2.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/wave2.bwj logs/wave2_server.log logs/wave2_bots1.log logs/wave2_bots2.log
mkdir -p logs
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[wave2] Server up (record epoch 30)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 120 --record-world logs/wave2.bwj > logs/wave2_server.log 2>&1 &
SRV=$!
sleep 2

echo "[wave2] Wave 1: 8 fighters 60s (creation + roster)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 8 --secs 60 --profile fighter --prefix w2_ > logs/wave2_bots1.log 2>&1 || true
sleep 2

echo "[wave2] Wave 2: relog wander 10s (y-sidecar restore)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 8 --secs 10 --profile wander --prefix w2_ > logs/wave2_bots2.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[wave2] Replay (expect mm=0)..."
./$BUILD_DIR/server/bh_server --replay-world logs/wave2.bwj 2>&1 | tail -n 3

echo "[wave2] Guard (old epoch-29 journal must refuse exit 4)..."
if ./$BUILD_DIR/server/bh_server --replay-world logs/t159.bwj > /dev/null 2>&1; then
  echo "[wave2] FAIL: t159 (epoch 29) replayed under epoch 30 (expected refusal)"
  exit 1
else
  rc=$?
  if [ "$rc" -eq 4 ]; then
    echo "[wave2] guard OK: t159 refused exit 4"
  else
    echo "[wave2] FAIL: t159 exited $rc (expected 4)"
    exit 1
  fi
fi
echo "[wave2] DONE"
