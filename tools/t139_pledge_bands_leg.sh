#!/bin/bash
# T-139 pledge-bands gate leg (epoch 26, no bump): sworn war-hosts muster as
# ONE band per pledge on a rehearsal server, then the journal replays mm=0.
#   wave 1: 5 wander bots 10s    (accounts created)
#   seed:   sqlite level=12 + gold=15000 (pledge founding gate)
#   wave 2: 5 pledge bots 40s    (oath ceremony: t139clan formed)
#   wave 3: 5 siege bots 300s    (SAME accounts -> pledge restored at login ->
#                                 /siege-reg enlists the whole sworn war-host)
set -e
PORT=${1:-7824}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t139.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t139.bwj logs/t139_server.log logs/t139_bots1.log logs/t139_bots2.log logs/t139_bots3.log
mkdir -p logs
BUILD_DIR=build/linux-gcc

echo "[t139] Rehearsal server up (record epoch 26)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 480 --siege-rehearsal --record-world logs/t139.bwj > logs/t139_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t139] Wave 1: 5 wander bots 10s (create accounts)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t139_ > logs/t139_bots1.log 2>&1 || true
sleep 2

echo "[t139] Seed: founding gate (level 12, gold 15000)..."
python3 <<'PY'
import sqlite3
conn = sqlite3.connect("/tmp/t139.db")
cur = conn.cursor()
cur.execute("UPDATE characters SET level=12, gold=15000 WHERE name LIKE 't139_%'")
conn.commit()
print("[t139] seeded", cur.execute("SELECT COUNT(*) FROM characters").fetchone())
PY

echo "[t139] Wave 2: 5 pledge bots 40s (oath ceremony)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 40 --profile pledge --prefix t139_ > logs/t139_bots2.log 2>&1 || true
sleep 2

echo "[t139] Wave 3: 5 siege bots 300s (march, register, ram)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 300 --profile siege --prefix t139_ > logs/t139_bots3.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t139] Muster evidence (pledge band = multi-member single enlistment):"
grep -E 'band enlisted|pledge band' logs/t139_server.log || echo "[t139] NOTE: no enlistment lines"
python3 <<'PY'
import re, sys
multi = 0
for line in open("logs/t139_server.log"):
    m = re.search(r"band enlisted \((\d+) members", line)
    if m and int(m.group(1)) > 1:
        multi += 1
        print("[t139] multi-member band:", line.strip())
print("[t139] multi-member enlistments:", multi)
sys.exit(0 if multi >= 1 else 1)
PY

echo "[t139] Replay check (epoch 26 leg of record):"
./$BUILD_DIR/server/bh_server --replay-world logs/t139.bwj 2>&1 | tail -2
