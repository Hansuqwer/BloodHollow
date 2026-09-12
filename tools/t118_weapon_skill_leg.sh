#!/bin/bash
# T-118 B1 weapon-skill persistence gate leg: 5 bots 120s fighter -> skill up, relog -> skill persists, replay mm=0, epoch 21
set -e
PORT=${1:-7821}
SECS=${2:-120}
ROOT=/home/user/BloodHollow/build/wt/118
cd $ROOT
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t118.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t118.bwj logs/t118_server.log logs/t118_bots1.log logs/t118_bots2.log logs/t118_db_after1.txt logs/t118_db_after2.txt
mkdir -p logs
# Ensure maps exist (worktree may miss ignored assets)
if [ ! -d assets/maps ]; then mkdir -p assets/maps; cp /home/user/BloodHollow/assets/maps/*.bhmap assets/maps/ 2>/dev/null || true; fi

echo "[t118] Phase 1+2: single server run, two bot waves, recording epoch 21..."
./build/headless/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 40)) --record-world logs/t118.bwj > logs/t118_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t118] Wave 1: 5 bots fighter ${SECS}s (gain skill)..."
./build/headless/tools/bots/bh_bots --port $PORT --count 5 --secs $SECS --profile fighter --prefix t118_ > logs/t118_bots1.log 2>&1 || true
echo "[t118] Wave 1 done, waiting 2s for saves..."
sleep 2

# Check DB after wave1
python3 <<'PY'
import sqlite3
conn=sqlite3.connect("/tmp/t118.db")
cur=conn.cursor()
cur.execute("SELECT name, level, sword_skill, swing_lands FROM characters ORDER BY name")
rows=cur.fetchall()
print("[t118] DB after wave1:")
for r in rows:
    print(r)
open("logs/t118_db_after1.txt","w").write("\n".join(str(x) for x in rows))
PY

echo "[t118] Wave 2: same 5 bots wander 10s (relog, should carry skill)..."
./build/headless/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t118_ > logs/t118_bots2.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t118] Server stopped, checking DB after wave2..."
python3 <<'PY'
import sqlite3
conn=sqlite3.connect("/tmp/t118.db")
cur=conn.cursor()
cur.execute("SELECT name, level, sword_skill, swing_lands FROM characters ORDER BY name")
rows=cur.fetchall()
print("[t118] DB after wave2:")
for r in rows:
    print(r)
open("logs/t118_db_after2.txt","w").write("\n".join(str(x) for x in rows))
PY

echo "[t118] Journal l-lines:"
grep "^l " logs/t118.bwj | head -20
echo "[t118] Second wave l-lines (should have skill>0 or lands>0):"
grep "^l " logs/t118.bwj | tail -10

echo "[t118] Replay check:"
./build/headless/server/bh_server --replay-world logs/t118.bwj 2>&1 | tail -10

echo "[t118] Bot summaries:"
cat logs/t118_bots1.log | grep SUMMARY
cat logs/t118_bots2.log | grep SUMMARY || true

# Verify persistence: at least one char has lands>0 in DB and journal second login has lands>0
python3 <<'PY'
import sqlite3, sys, re
conn=sqlite3.connect("/tmp/t118.db")
cur=conn.cursor()
cur.execute("SELECT MAX(swing_lands), MAX(sword_skill) FROM characters")
max_lands, max_skill = cur.fetchone()
print(f"[t118] max_lands={max_lands} max_skill={max_skill}")
if max_lands == 0:
    print("[t118] FAIL: no lands persisted")
    sys.exit(1)
# Check journal second wave has skill/lands
with open("logs/t118.bwj") as f:
    lines=[l for l in f if l.startswith("l ")]
# first 5 are wave1 (skill 0), next 5 wave2 should have lands>0
if len(lines) < 10:
    print(f"[t118] FAIL: expected >=10 l-lines, got {len(lines)}")
    sys.exit(1)
# parse second wave
def parse_l(line):
    # l tick idx name x y zone level xp str vit dex sp gold mercy karma swordSkill swingLands inv
    parts=line.split()
    if len(parts) < 18:
        return None
    try:
        skill=int(parts[15])
        lands=int(parts[16])
        return skill, lands
    except:
        return None

second_wave=lines[5:10]
has_persist=False
for l in second_wave:
    p=parse_l(l)
    if p and (p[0]>0 or p[1]>0):
        has_persist=True
        print(f"[t118] persist found in journal: {l.strip()} -> skill={p[0]} lands={p[1]}")
if not has_persist:
    print("[t118] FAIL: second wave journal shows no persisted skill/lands")
    sys.exit(1)
print("[t118] PASS: skill persistence verified")
PY
