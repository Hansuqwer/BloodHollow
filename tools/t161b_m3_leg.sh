#!/bin/bash
# T-161b M3 re-measure: L13 mixed-kit raider party to the Gravemother, 900 s.
# M3 party-vs-solo re-measure with the fuller kit (T-161b acceptance close):
# L13 raider party marches town -> crypt -> depths -> Gravemother. Per-bot
# [raid]/CRYPT lines carry reach/TTK/deaths; the fuller kit (ch11-23) rides
# along. Pattern: t161b_control_leg.sh. Staging disclosed.
# usage: t161b_m3_$TAG_leg.sh [port] [secs]
set -e
PORT=${1:-7857}
SECS=${2:-300}
PROF=${3:-crypt}
TAG=${4:-$PROF}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t161b_m3_$TAG.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t161b_m3_$TAG.bwj logs/t161b_m3_$TAG_server.log logs/t161b_m3_$TAG_bots.log
mkdir -p logs
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[T-161b.M3] Phase 1: registering 5 accounts..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 40 > logs/t161b_m3_$TAG_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 12 --profile wander --prefix m3k_ > logs/t161b_m3_$TAG_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[T-161b.M3] Phase 2: server-down top-up to L13, kits 1/3/3/2/1..."
python3 - "$DB" <<'PY'
import sqlite3, sys
con = sqlite3.connect(sys.argv[1])
cur = con.cursor()
cur.execute("SELECT id, name FROM characters ORDER BY name")
rows = cur.fetchall()
names = [n for (_, n) in rows if n.startswith("m3k_")]
assert len(names) == 5, f"expected 5 m3k_ characters, got {len(names)}"
kits = [1, 3, 3, 2, 1]
inv_blob = "2002:1:1:0:100:0:0;2101:1:1:0:100:0:0;3001:16:0:0:100:0:0;"
for nm, kit in zip(sorted(names), kits):
    cur.execute("UPDATE characters SET level=13, xp=0, str=20, vit=24, dex=13, stat_points=0, gold=500, inv=?, karma=0, class_id=?, map_id=1, x=0, y=0 WHERE name=?",
                (inv_blob, kit, nm))
    print(f"[T-161b.M3] staged {nm} L13 kit={kit}")
con.commit()
con.close()
PY

echo "[T-161b.M3] Phase 3: recorded crypt run (${SECS}s)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 60)) --record-world logs/t161b_m3_$TAG.bwj > logs/t161b_m3_$TAG_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile raider --party-size 5 --prefix m3k_ > logs/t161b_m3_$TAG_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[T-161b.M3/$PROF] assertions:"
grep "SUMMARY" logs/t161b_m3_$TAG_bots.log | tail -1
CXXX=$(grep "SUMMARY" logs/t161b_m3_$TAG_bots.log | grep -oE "c1[5-8]=[0-9]+" | cut -d= -f2 | awk '{s+=$1} END {print s+0}')
echo "control casts (c15-c18 SUMMARY total): $CXXX"
echo "[T-161b.M3] replay check:"
./$BUILD_DIR/server/bh_server --replay-world logs/t161b_m3_$TAG.bwj 2>&1 | tail -2
