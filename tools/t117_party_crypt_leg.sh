#!/bin/bash
# T-117 M3 party-crypt gate leg: 5-person mixed-kit party clears Crypt to Gravemother
# Usage: t117_party_crypt_leg.sh [port] [secs]
set -e
PORT=${1:-7817}
SECS=${2:-300}
ROOT=/home/user/BloodHollow/build/wt/117
cd $ROOT
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t117.db
rm -f $DB
rm -f logs/t117.bwj
mkdir -p logs

# Phase 1: create accounts by short wander leg (10s, 5 bots)
echo "[t117] Phase 1: creating 5 accounts in fresh DB..."
./build/manual/bh_server --db $DB --port $PORT --soak-secs 30 > logs/t117_phase1_server.log 2>&1 &
SRV=$!
sleep 2
./build/manual/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t117_ > logs/t117_phase1_bots.log 2>&1 || true
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true
echo "[t117] Phase 1 done, DB should have 5 chars"

# Phase 2: pre-level DB to L12 with gear and kits
echo "[t117] Phase 2: pre-seeding DB to L12..."
python3 <<'PY'
import sqlite3
import os
db_path = "/tmp/t117.db"
conn = sqlite3.connect(db_path)
cur = conn.cursor()
# Show existing chars
cur.execute("SELECT id, name, level, class_id FROM characters")
rows = cur.fetchall()
print(f"Found {len(rows)} characters:")
for r in rows:
    print(r)

# Desired kits: idx 0 Ravager(1), 1 Cultist(3), 2 Cultist(3), 3 Gravecaller(2), 4 Ravager(1)
kits = [1,3,3,2,1]
# Sort rows by name to match prefix order t117_00..04
rows_sorted = sorted(rows, key=lambda x: x[1])
for idx, (char_id, name, lvl, class_id) in enumerate(rows_sorted):
    if idx >= len(kits):
        break
    kit = kits[idx]
    # stats: allocate for survivability at L12
    # L12: 11 levels *3 =33 points over base 8
    # For Ravager: str 18, vit 16, dex 11 (8+10, 8+8, 8+3) =33
    # For Cultist: str 5 base? Actually kit base is 5,8,5 — but DB stores str/vit/dex raw, kitChoose will adjust
    # So set DB str/vit/dex to desired final values (kitChoose preserves extra over 8)
    # Let's set for all: str 16, vit 16, dex 12 => extra = (8+8+4)=20? Need 33, so we need more.
    # Actually base 8, we want 8+33 distribution. Let's do: str 18 (extra10), vit 18 (extra10), dex 14 (extra6) + remaining 7 -> put into vit => vit 20? Let's just set str 20, vit 20, dex 11 = extra 12+12+3=27, need 6 more -> str 22, vit 22, dex 12 => extra 14+14+4=32, need 1 more -> vit 23 => extra 14+15+4=33. So str 22, vit 23, dex 12.
    # For simplicity set str 20, vit 20, dex 13 => extra 12+12+5=29, need 4 more -> vit 24 => extra 12+16+5=33 => str 20, vit 24, dex 13
    # Let's use str 20, vit 24, dex 13 for all (good HP)
    str_v = 20
    vit_v = 24
    dex_v = 13
    # For Gravecaller, maybe more int? But int/mag come from kit, not DB
    # Inventory blob: Pit Blade 2002 equipped, Hide Armor 2101 equipped, 16 vials 3001
    inv_blob = "2002:1:1:0:100:0:0;2101:1:1:0:100:0:0;3001:16:0:0:100:0:0;"
    gold = 500
    level = 12
    xp = 0
    stat_points = 0
    anvil_mercy = 0
    karma = 0
    # Update
    cur.execute("UPDATE characters SET level=?, xp=?, str=?, vit=?, dex=?, stat_points=?, gold=?, inv=?, anvil_mercy=?, karma=?, class_id=?, map_id=1, x=0, y=0 WHERE id=?",
                (level, xp, str_v, vit_v, dex_v, stat_points, gold, inv_blob, anvil_mercy, karma, kit, char_id))
    print(f"Updated {name} id={char_id} to L{level} kit={kit} str={str_v} vit={vit_v} dex={dex_v} inv={inv_blob}")

conn.commit()
# Verify
cur.execute("SELECT name, level, class_id, str, vit, dex, gold, inv FROM characters")
for r in cur.fetchall():
    print("VERIFIED:", r)
conn.close()
PY

# Phase 3: crypt leg with recording
echo "[t117] Phase 3: running crypt party leg ${SECS}s with recording..."
./build/manual/bh_server --db $DB --port $PORT --soak-secs $((SECS + 60)) --record-world logs/t117.bwj > logs/t117_server.log 2>&1 &
SRV=$!
sleep 2
./build/manual/bh_bots --port $PORT --count 5 --secs $SECS --profile crypt --prefix t117_ > logs/t117_bots.log 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true
echo "[t117] Bots RC=$RC"
echo "[t117] Replay check:"
./build/manual/bh_server --replay-world logs/t117.bwj 2>&1 | tail -5
echo "[t117] Bot summary:"
cat logs/t117_bots.log | grep -E "SUMMARY|CRYPT|boss|campaign" | tail -20
