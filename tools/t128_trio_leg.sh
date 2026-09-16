#!/bin/bash
# T-128 trio uniques gate leg: 5 bots 60s fighter, record epoch 24, replay mm=0.
# Epoch 23->24: trio unique rows draw per-row range(1,100) on 1013/1014/1009 kills.
# Profile is fighter (kills -> drops exercise the widened roll); valid profiles:
# wander|fighter|pilgrim|campaign|crypt|raider (there is no 'grinder').
set -e
PORT=${1:-7833}
SECS=${2:-60}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t128.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t128.bwj logs/t128_server.log logs/t128_bots.log
mkdir -p logs
# NOTE (T-126): older *_leg.sh scripts assume ./build/<target>; the
# linux-gcc preset nests binaries under build/linux-gcc/.
BUILD_DIR=build/linux-gcc

echo "[t128] server up, recording epoch 24..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 20)) --record-world logs/t128.bwj > logs/t128_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t128] 5 bots fighter ${SECS}s..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile fighter --prefix t128_ > logs/t128_bots.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t128] replay check:"
./$BUILD_DIR/server/bh_server --replay-world logs/t128.bwj 2>&1 | tail -4
echo "[t128] bot summary:"
grep SUMMARY logs/t128_bots.log || true
