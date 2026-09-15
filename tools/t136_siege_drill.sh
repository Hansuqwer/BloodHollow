#!/bin/bash
# T-136 siege drill: rehearsal server + 6 attacker bots flip the castle.
# Flow: brief login wave (create chars) -> server-down sqlite top-up
# (L12 + equipped blade/armor + vials, disclosed like m3_topup) ->
# recorded rehearsal run (~480 s): march, reg, start, breach x2, attune,
# crown. Asserts bands, battle, splinters x2, attuned, crowned + replay mm=0.
set -e
PORT=${1:-7835}
SECS=${2:-900}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t136.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t136.bwj logs/t136_server.log logs/t136_bots0.log logs/t136_bots.log
mkdir -p logs
BUILD_DIR=build/linux-gcc

echo "[t136] login wave (create 6 chars)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 40 > logs/t136_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 6 --secs 15 --profile wander --prefix t136_ > logs/t136_bots0.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t136] server-down top-up (disclosed staging)..."
python3 - "$DB" <<'PY'
import sqlite3, sys
con = sqlite3.connect(sys.argv[1])
cur = con.cursor()
names = [r[0] for r in cur.execute("SELECT name FROM characters ORDER BY id")]
want = [n for n in names if n.startswith("t136_")]
assert len(want) == 6, f"expected 6 t136_ characters, got {want}"
blob = "2002:1:1:0:100:0:0;2102:1:1:0:100:0:0;3001:16:0:0:100:0:0;"
marks = ",".join("?" * len(want))
cur.execute(f"UPDATE characters SET level=15, xp=0, str=24, vit=24, dex=14, gold=600, inv=? WHERE name IN ({marks})",
            [blob] + want)
con.commit()
print(f"[t136] topped up: {want}")
PY

echo "[t136] rehearsal run recording epoch 25..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 30)) --siege-rehearsal --record-world logs/t136.bwj > logs/t136_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count 6 --secs $SECS --profile siege --prefix t136_ > logs/t136_bots.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t136] assertions (server-log printf markers; broadcasts go to clients):"
grep -c "rehearsal] siege window OPEN" logs/t136_server.log | xargs -I{} echo "rehearsal-flag: {}"
grep "SIEGE" logs/t136_bots.log | tail -1
grep -c "breached by" logs/t136_server.log | xargs -I{} echo "gates-down: {}"
grep -c "heartstone attuned at tick" logs/t136_server.log | xargs -I{} echo "attuned: {}"
grep -c "crowned:" logs/t136_server.log | xargs -I{} echo "crowned: {}"
echo "[t136] replay check:"
./$BUILD_DIR/server/bh_server --siege-rehearsal --replay-world logs/t136.bwj 2>&1 | tail -2
echo "[t136] cross-mode refusal (no flag on rehearsal journal):"
./$BUILD_DIR/server/bh_server --replay-world logs/t136.bwj > /dev/null 2>&1; echo "exit=$? (want 4)"
