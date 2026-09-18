#!/bin/bash
# T-161b.3 Raise gate leg: L16 mixed-kit crypt party, 300 s.
# Fires ch13 (Raise Skeleton) through the journaled kSkill lane and replays
# bit-exact. Pattern: t161b_sanct_leg.sh (register -> server-down top-up ->
# recorded crypt run). The crypt march crosses zone portals, so thrall
# gate-following is exercised wherever a thrall lives. Staging disclosed.
# usage: t161b_raise_leg.sh [port] [secs]
set -e
PORT=${1:-7852}
SECS=${2:-300}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t161b_raise.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t161b_raise.bwj logs/t161b_raise_server.log logs/t161b_raise_bots.log
mkdir -p logs
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[t161b.3] Phase 1: registering 5 accounts..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 40 > logs/t161b_raise_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 12 --profile wander --prefix t161r_ > logs/t161b_raise_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t161b.3] Phase 2: server-down top-up to L16, kits 1/3/3/2/1..."
python3 - "$DB" <<'PY'
import sqlite3, sys
con = sqlite3.connect(sys.argv[1])
cur = con.cursor()
cur.execute("SELECT id, name FROM characters ORDER BY name")
rows = cur.fetchall()
names = [n for (_, n) in rows if n.startswith("t161r_")]
assert len(names) == 5, f"expected 5 t161r_ characters, got {len(names)}"
kits = [1, 3, 3, 2, 1]
inv_blob = "2002:1:1:0:100:0:0;2101:1:1:0:100:0:0;3001:16:0:0:100:0:0;"
for nm, kit in zip(sorted(names), kits):
    cur.execute("UPDATE characters SET level=16, xp=0, str=20, vit=24, dex=13, stat_points=0, gold=500, inv=?, karma=0, class_id=?, map_id=1, x=0, y=0 WHERE name=?",
                (inv_blob, kit, nm))
    print(f"[t161b.3] staged {nm} L16 kit={kit}")
con.commit()
con.close()
PY

echo "[t161b.3] Phase 3: recorded crypt run (${SECS}s)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 60)) --record-world logs/t161b_raise.bwj > logs/t161b_raise_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile crypt --party-size 5 --prefix t161r_ > logs/t161b_raise_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t161b.3] assertions:"
grep "SUMMARY" logs/t161b_raise_bots.log | tail -1
C13=$(grep "SUMMARY" logs/t161b_raise_bots.log | grep -oE "c13=[0-9]+" | cut -d= -f2)
echo "raise casts (c13 SUMMARY): $C13"
echo "[t161b.3] replay check:"
./$BUILD_DIR/server/bh_server --replay-world logs/t161b_raise.bwj 2>&1 | tail -2
