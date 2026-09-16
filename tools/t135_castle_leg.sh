#!/bin/bash
# T-135 castle-zone gate leg: 5 bots 60s fighter, record epoch 25, replay mm=0.
# Epoch 24->25: Weeping Castle boots as zone 6 (entity set shifts).
# Profile is fighter; valid profiles:
# wander|fighter|pilgrim|campaign|crypt|raider (there is no 'grinder').
set -e
PORT=${1:-7834}
SECS=${2:-60}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t135.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t135.bwj logs/t135_server.log logs/t135_bots.log
mkdir -p logs
# NOTE (T-126): older *_leg.sh scripts assume ./build/<target>; the
# linux-gcc preset nests binaries under build/linux-gcc/.
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[t135] server up, recording epoch 25..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 20)) --record-world logs/t135.bwj > logs/t135_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t135] 5 bots fighter ${SECS}s..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile fighter --prefix t135_ > logs/t135_bots.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t135] replay check:"
./$BUILD_DIR/server/bh_server --replay-world logs/t135.bwj 2>&1 | tail -4
echo "[t135] bot summary:"
grep SUMMARY logs/t135_bots.log || true
