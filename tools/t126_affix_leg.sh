#!/bin/bash
# T-126 affix v2 gate leg: 5 bots 60s fighter, record epoch 22, replay mm=0.
# Epoch 21->22: affix roll widens 1..3 -> 1..10 (stream shift in gear-drop journals).
# Profile is fighter (kills -> drops exercise the widened roll); valid profiles:
# wander|fighter|pilgrim|campaign|crypt|raider (there is no 'grinder').
set -e
PORT=${1:-7831}
SECS=${2:-60}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t126.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t126.bwj logs/t126_server.log logs/t126_bots.log
mkdir -p logs
# NOTE (T-126): older *_leg.sh scripts assume ./build/<target>; the
# linux-gcc preset nests binaries under build/linux-gcc/.
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[t126] server up, recording epoch 22..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 20)) --record-world logs/t126.bwj > logs/t126_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t126] 5 bots fighter ${SECS}s..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile fighter --prefix t126_ > logs/t126_bots.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t126] replay check:"
./$BUILD_DIR/server/bh_server --replay-world logs/t126.bwj 2>&1 | tail -4
echo "[t126] bot summary:"
grep SUMMARY logs/t126_bots.log || true
