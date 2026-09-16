#!/bin/bash
# T-138 pledge-lite rebase gate leg (epoch 26): 5 bots found/swear/leave a pledge
# at the registrar, relog -> membership persists (DB + g-sidecar), replay mm=0.
# Adapted from tools/t122_pledge_leg.sh (epoch 22, kinds 28-34); rebased onto the
# Phase-S stack: epoch 26, schema v13, journal kinds 34-40.
# Phases on ONE server + ONE journal:
#   wave 1: 5 wander bots 10s   (accounts created, level 1, saved at logout)
#   seed:   sqlite sets level=12 + gold=15000 (founding gate: L>=10 + 10,000g)
#   wave 2: 5 pledge bots 35s   (ceremony: create/invite/accept/chat/promote/
#                                kick/leave; journal c-lines + g-lines)
#   wave 3: 5 wander bots 10s   (relog: g-lines must carry restored membership)
set -e
PORT=${1:-7823}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t138.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t138.bwj logs/t138_server.log logs/t138_bots1.log logs/t138_bots2.log logs/t138_bots3.log
mkdir -p logs
BUILD_DIR=build/linux-gcc

echo "[t138] Server up (record epoch 26)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 100 --record-world logs/t138.bwj > logs/t138_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t138] Wave 1: 5 wander bots 10s (create accounts)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t138_ > logs/t138_bots1.log 2>&1 || true
sleep 2

echo "[t138] Seed: founding gate (level 12, gold 15000)..."
python3 <<'PY'
import sqlite3
conn = sqlite3.connect("/tmp/t138.db")
cur = conn.cursor()
cur.execute("UPDATE characters SET level=12, gold=15000 WHERE name LIKE 't138_%'")
conn.commit()
cur.execute("SELECT name, level, gold FROM characters ORDER BY name")
for r in cur.fetchall():
    print("[t138] seeded", r)
PY

echo "[t138] Wave 2: 5 pledge bots 35s (oath ceremony)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 35 --profile pledge --prefix t138_ > logs/t138_bots2.log 2>&1 || true
sleep 2

echo "[t138] Wave 3: 5 wander bots 10s (relog, membership restore)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t138_ > logs/t138_bots3.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t138] DB state (pledges + membership after all waves):"
python3 <<'PY'
import sqlite3, sys
conn = sqlite3.connect("/tmp/t138.db")
cur = conn.cursor()
cur.execute("SELECT id, name, emblem, liege FROM pledges")
pledges = cur.fetchall()
for r in pledges:
    print("[t138] pledge:", r)
cur.execute("SELECT name, pledge_id, pledge_rank FROM characters ORDER BY name")
rows = cur.fetchall()
for r in rows:
    print("[t138] char:", r)
ok = True
if len(pledges) != 1 or pledges[0][1] != "t122clan":
    print("[t138] FAIL: expected exactly one pledge 't122clan'")
    ok = False
by_name = {r[0]: (r[1], r[2]) for r in rows}
expect = {"t138__00": (1, 3), "t138__01": (1, 2), "t138__04": (1, 1),
          "t138__02": (0, 0), "t138__03": (0, 0)}
for nm, (pid, rank) in expect.items():
    got = by_name.get(nm)
    if got != (pid, rank):
        print(f"[t138] FAIL: {nm} expected pledge={pid} rank={rank}, got {got}")
        ok = False
if not ok:
    sys.exit(1)
print("[t138] PASS: membership persisted (liege + bloodsworn + initiate, kicked/left unsworn)")
PY

echo "[t138] Journal: version line + g-sidecar + pledge c-lines:"
head -1 logs/t138.bwj
echo "-- g-lines (login sidecar: tick idx pledgeId rank):"
grep "^g " logs/t138.bwj
echo "-- pledge c-lines (kinds 34-40: create/invite/accept/leave/kick/rank/disband):"
grep -cE "^c [0-9]+ [0-9]+ 34 " logs/t138.bwj | xargs echo "create c-lines:"
grep -E "^c [0-9]+ [0-9]+ (34|35|36|37|38|39|40) " logs/t138.bwj

echo "[t138] Replay check (epoch 26 leg of record):"
./$BUILD_DIR/server/bh_server --replay-world logs/t138.bwj 2>&1 | tail -3
