#!/bin/bash
# T-161b.4 Corpse-blast gate leg: L14 mixed-kit crypt party, 300 s.
# Fires ch14 (Corpse Explosion) through the journaled kSkill lane and replays
# bit-exact. Pattern: t161b_raise_leg.sh (register -> server-down top-up ->
# recorded crypt run). The yard packs of the crypt march lay the corpses the
# blast spends. Staging disclosed.
# usage: t161b_blast_leg.sh [port] [secs]
set -e
PORT=${1:-7853}
SECS=${2:-300}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t161b_blast.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t161b_blast.bwj logs/t161b_blast_server.log logs/t161b_blast_bots.log
mkdir -p logs
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[T-161b.4] Phase 1: registering 5 accounts..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 40 > logs/t161b_blast_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 12 --profile wander --prefix t161b_ > logs/t161b_blast_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[T-161b.4] Phase 2: server-down top-up to L14, kits 1/3/3/2/1..."
python3 - "$DB" <<'PY'
import sqlite3, sys
con = sqlite3.connect(sys.argv[1])
cur = con.cursor()
cur.execute("SELECT id, name FROM characters ORDER BY name")
rows = cur.fetchall()
names = [n for (_, n) in rows if n.startswith("t161b_")]
assert len(names) == 5, f"expected 5 t161b_ characters, got {len(names)}"
kits = [1, 3, 3, 2, 1]
inv_blob = "2002:1:1:0:100:0:0;2101:1:1:0:100:0:0;3001:16:0:0:100:0:0;"
for nm, kit in zip(sorted(names), kits):
    cur.execute("UPDATE characters SET level=14, xp=0, str=20, vit=24, dex=13, stat_points=0, gold=500, inv=?, karma=0, class_id=?, map_id=1, x=0, y=0 WHERE name=?",
                (inv_blob, kit, nm))
    print(f"[T-161b.4] staged {nm} L16 kit={kit}")
con.commit()
con.close()
PY

echo "[T-161b.4] Phase 3: recorded crypt run (${SECS}s)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 60)) --record-world logs/t161b_blast.bwj > logs/t161b_blast_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile crypt --party-size 5 --prefix t161b_ > logs/t161b_blast_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[T-161b.4] assertions:"
grep "SUMMARY" logs/t161b_blast_bots.log | tail -1
C14=$(grep "SUMMARY" logs/t161b_blast_bots.log | grep -oE "c14=[0-9]+" | cut -d= -f2)
echo "blast casts (c14 SUMMARY): $C14"
echo "[T-161b.4] replay check:"
./$BUILD_DIR/server/bh_server --replay-world logs/t161b_blast.bwj 2>&1 | tail -2
