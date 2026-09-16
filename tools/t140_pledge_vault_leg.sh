#!/bin/bash
# T-140 pledge-vault gate leg (epoch 26, schema v14, no epoch bump): the oath
# ceremony now tithes — the vault accrues in the DB and the journal replays
# mm=0. Drip routing is pinned by doctest twins (live crown is a follow-up).
#   wave 1: 5 wander bots 10s   (accounts created)
#   seed:   sqlite level=12 + gold=15000 (founding gate)
#   wave 2: 5 pledge bots 45s   (ceremony + liege `/pledge tithe 500`)
#   wave 3: 5 wander bots 10s   (relog: membership + vault persist)
set -e
PORT=${1:-7825}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t140.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t140.bwj logs/t140_server.log logs/t140_bots1.log logs/t140_bots2.log logs/t140_bots3.log
mkdir -p logs
BUILD_DIR=build/linux-gcc

echo "[t140] Server up (record epoch 26)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 140 --record-world logs/t140.bwj > logs/t140_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t140] Wave 1: 5 wander bots 10s (create accounts)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t140_ > logs/t140_bots1.log 2>&1 || true
sleep 2

echo "[t140] Seed: founding gate (level 12, gold 15000)..."
python3 <<'PY'
import sqlite3
conn = sqlite3.connect("/tmp/t140.db")
cur = conn.cursor()
cur.execute("UPDATE characters SET level=12, gold=15000 WHERE name LIKE 't140_%'")
conn.commit()
print("[t140] seeded", cur.execute("SELECT COUNT(*) FROM characters").fetchone())
PY

echo "[t140] Wave 2: 5 pledge bots 45s (ceremony + tithe)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 45 --profile pledge --prefix t140_ > logs/t140_bots2.log 2>&1 || true
sleep 2

echo "[t140] Wave 3: 5 wander bots 10s (relog, vault persists)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t140_ > logs/t140_bots3.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t140] DB state (pledge vault after ceremony + relog):"
python3 <<'PY'
import sqlite3, sys
conn = sqlite3.connect("/tmp/t140.db")
cur = conn.cursor()
rows = cur.execute("SELECT id, name, emblem, liege, vault_gold FROM pledges").fetchall()
for r in rows:
    print("[t140] pledge:", r)
ok = True
if len(rows) != 1 or rows[0][4] < 500:
    print("[t140] FAIL: expected one pledge with vault >= 500 (liege tithe)")
    ok = False
sys.exit(0 if ok else 1)
PY

echo "[t140] Journal: kind-41 tithe c-lines:"
grep -E "^c [0-9]+ [0-9]+ 41 " logs/t140.bwj | head -n 5
grep -cE "^c [0-9]+ [0-9]+ 41 " logs/t140.bwj | xargs echo "tithe c-lines:"

echo "[t140] Replay check (epoch 26 leg of record):"
./$BUILD_DIR/server/bh_server --replay-world logs/t140.bwj 2>&1 | tail -2
