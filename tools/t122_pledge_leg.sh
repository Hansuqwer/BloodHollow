#!/bin/bash
# T-122 B2 pledge-lite gate leg (epoch 22): 5 bots found/swear/leave a pledge
# at the registrar, relog -> membership persists (DB + g-sidecar), replay mm=0.
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
DB=/tmp/t122.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t122.bwj logs/t122_server.log logs/t122_bots1.log logs/t122_bots2.log logs/t122_bots3.log
mkdir -p logs
# Ensure maps exist (worktree may miss ignored assets)
if [ ! -d assets/maps ]; then mkdir -p assets/maps; cp /home/user/BloodHollow/assets/maps/*.bhmap assets/maps/ 2>/dev/null || true; fi

echo "[t122] Server up (record epoch 22)..."
./build/server/bh_server --db $DB --port $PORT --soak-secs 100 --record-world logs/t122.bwj > logs/t122_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t122] Wave 1: 5 wander bots 10s (create accounts)..."
./build/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t122_ > logs/t122_bots1.log 2>&1 || true
sleep 2

echo "[t122] Seed: founding gate (level 12, gold 15000)..."
python3 <<'PY'
import sqlite3
conn = sqlite3.connect("/tmp/t122.db")
cur = conn.cursor()
cur.execute("UPDATE characters SET level=12, gold=15000 WHERE name LIKE 't122_%'")
conn.commit()
cur.execute("SELECT name, level, gold FROM characters ORDER BY name")
for r in cur.fetchall():
    print("[t122] seeded", r)
PY

echo "[t122] Wave 2: 5 pledge bots 35s (oath ceremony)..."
./build/tools/bots/bh_bots --port $PORT --count 5 --secs 35 --profile pledge --prefix t122_ > logs/t122_bots2.log 2>&1 || true
sleep 2

echo "[t122] Wave 3: 5 wander bots 10s (relog, membership restore)..."
./build/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t122_ > logs/t122_bots3.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t122] DB state (pledges + membership after all waves):"
python3 <<'PY'
import sqlite3, sys
conn = sqlite3.connect("/tmp/t122.db")
cur = conn.cursor()
cur.execute("SELECT id, name, emblem, liege FROM pledges")
pledges = cur.fetchall()
for r in pledges:
    print("[t122] pledge:", r)
cur.execute("SELECT name, pledge_id, pledge_rank FROM characters ORDER BY name")
rows = cur.fetchall()
for r in rows:
    print("[t122] char:", r)
ok = True
if len(pledges) != 1 or pledges[0][1] != "t122clan":
    print("[t122] FAIL: expected exactly one pledge 't122clan'")
    ok = False
by_name = {r[0]: (r[1], r[2]) for r in rows}
expect = {"t122__00": (1, 3), "t122__01": (1, 2), "t122__04": (1, 1),
          "t122__02": (0, 0), "t122__03": (0, 0)}
for nm, (pid, rank) in expect.items():
    got = by_name.get(nm)
    if got != (pid, rank):
        print(f"[t122] FAIL: {nm} expected pledge={pid} rank={rank}, got {got}")
        ok = False
if not ok:
    sys.exit(1)
print("[t122] PASS: membership persisted (liege + bloodsworn + initiate, kicked/left unsworn)")
PY

echo "[t122] Journal: version line + g-sidecar + pledge c-lines:"
head -1 logs/t122.bwj
echo "-- g-lines (login sidecar: tick idx pledgeId rank):"
grep "^g " logs/t122.bwj
echo "-- pledge c-lines (kind 28=create 29=invite 30=accept 31=leave 32=kick 33=rank 34=disband):"
grep -cE "^c [0-9]+ [0-9]+ 28 " logs/t122.bwj | xargs echo "create c-lines:"
grep -E "^c [0-9]+ [0-9]+ (28|29|30|31|32|33|34) " logs/t122.bwj

echo "[t122] Replay check (epoch 22 leg of record):"
./build/server/bh_server --replay-world logs/t122.bwj 2>&1 | tail -3
