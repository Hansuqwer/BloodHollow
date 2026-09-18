#!/bin/bash
# R5 M1+M2 refresh @ epoch 31: 20-bot 30-min soak (M1: p99 <10ms, 0 desyncs)
# + L6-8 XP pace (M2: ~1.2 levels/h). Bots ride disclosed L6 top-up
# (server-down sqlite, t137-class staging) so the pace window is L6-8.
set -e
PORT=${1:-7847}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/m1e31.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/m1e31.bwj logs/m1e31_server.log logs/m1e31_bots.log
mkdir -p logs
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[m1e31] login wave (20 fighters)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 120 > logs/m1e31_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 20 --secs 20 --profile wander --prefix m1_ > logs/m1e31_bots0.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[m1e31] server-down top-up to L6 (disclosed staging)..."
python3 - "$DB" <<'PY'
import sqlite3, sys
con = sqlite3.connect(sys.argv[1])
cur = con.cursor()
names = [r[0] for r in cur.execute("SELECT name FROM characters ORDER BY id")]
want = [n for n in names if n.startswith("m1_")]
assert len(want) == 20, f"expected 20 m1_ characters, got {len(want)}"
# legacy 6-field blob (proves 10-field parser compat live) + 600g belt stake
blob = "2002:1:1:0:100:0;2101:1:1:0:100:0;3001:16:0:0:100:0;"
marks = ",".join("?" * len(want))
cur.execute(f"UPDATE characters SET level=6, xp=0, str=14, vit=14, dex=10, gold=600, inv=? WHERE name IN ({marks})",
            [blob] + want)
con.commit()
print(f"[m1e31] topped up {len(want)} to L6")
PY

echo "[m1e31] soak: 20 fighters x 1800s, recording epoch 31..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 1830 --record-world logs/m1e31.bwj > logs/m1e31_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 20 --secs 1800 --profile fighter --prefix m1_ > logs/m1e31_bots.log 2>&1 &
BOTS=$!
wait $BOTS 2>/dev/null || true
sleep 5
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[m1e31] pace (DB levels/xp)..."
python3 - "$DB" <<'PY'
import sqlite3, sys
con = sqlite3.connect(sys.argv[1])
rows = con.execute("SELECT name, level, xp, gold FROM characters WHERE name LIKE 'm1\\_%' ESCAPE '\\' ORDER BY name").fetchall()
assert len(rows) == 20, f"expected 20 rows, got {len(rows)}"
gain = [r[1] - 6 for r in rows]
print(f"[m1e31] n=20 start=L6/xp0 end_mean_L={sum(r[1] for r in rows)/20:.2f} levels_gained_mean={sum(gain)/20:.2f} (x2 = levels/h)")
print(f"[m1e31] mean_xp_banked={sum(r[2] for r in rows)/20:.0f} mean_gold={sum(r[3] for r in rows)/20:.0f}")
for r in rows:
    print(f"  {r[0]} L{r[1]} xp={r[2]} g={r[3]}")
PY

echo "[m1e31] p99..."
grep -E "FINISH|OK p99" logs/m1e31_server.log | tail -n 2

echo "[m1e31] Replay (expect mm=0)..."
./$BUILD_DIR/server/bh_server --replay-world logs/m1e31.bwj 2>&1 | tail -n 2
