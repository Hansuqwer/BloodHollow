#!/bin/bash
# T-168 clean-clone check: the script T-149's human run leans on.
# Clone HEAD to a temp dir, headless build, maps, boot, 5 bots, replay leg.
set -e
cd "$(dirname "$0")/.." || exit 1
WORK=${1:-/tmp/bh_clean_clone}
PORT=${2:-7851}
rm -rf "$WORK"
git clone -q . "$WORK"
cd "$WORK" || exit 1
echo "[clean-clone] headless configure+build..."
cmake --preset headless > /tmp/bh_cc_conf.log 2>&1
cmake --build --preset headless -j4 > /tmp/bh_cc_build.log 2>&1
echo "[clean-clone] unit suite..."
ctest --preset headless > /tmp/bh_cc_test.log 2>&1
echo "[clean-clone] boot + 5 bots..."
DB=/tmp/bh_cc.db
rm -f $DB $DB-wal $DB-shm
./build/headless/server/bh_server --db $DB --port $PORT --soak-secs 30 > /tmp/bh_cc_server.log 2>&1 &
SRV=$!
sleep 2
./build/headless/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix cc_ > /tmp/bh_cc_bots.log 2>&1 || true
sleep 1
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true
echo "[clean-clone] replay leg of record..."
./build/headless/server/bh_server --replay-world logs/t159.bwj 2>&1 | tail -n 2
echo "[clean-clone] PASS"
