#!/bin/bash
# T-118 M3 gate leg: the party of five (same DB + names as the grind legs)
# marches town -> crypt hatch (10,10) -> depths stairs (44,6) -> Drowned
# Crypt Depths, fights to the Gravemother, attempts the kill. Journal of
# record: logs/m3_gate.bwj (must replay bit-exact); per-bot [raid] lines
# carry the entry/TTK/deaths/xp numbers for the verdict.
# usage: m3_gate_leg.sh <port> [secs]
set -u
PORT=${1:?port}
SECS=${2:-600}
cd "$(dirname "$0")/.." || exit 1
BUILD=${BUILD_DIR:-build/verify}
DB=${DB:-"$PWD/build/m3_gate.db"}
pkill -x bh_server 2>/dev/null; sleep 1
"$BUILD/server/bh_server" --db "$DB" --port "$PORT" \
  --soak-secs $((SECS + 60)) --record-world "logs/m3_gate.bwj" \
  > "logs/m3_gate_server.log" 2>&1 &
SRV=$!
sleep 2
"$BUILD/tools/bots/bh_bots" --port "$PORT" --profile raider --count 5 \
  --secs "$SECS" --party-size 5 --prefix m3g_ \
  > "logs/m3_gate_bots.log" 2>&1
RC=$?
# Same disconnect-save race as the grind legs: wait for the DB to show the
# final levels before stopping the server (keeps re-runs from restarting
# at a stale position/level).
python3 - "$DB" "logs/m3_gate_bots.log" <<'PYEOF'
import re, sqlite3, sys, time
db, blog = sys.argv[1], sys.argv[2]
final = {}
for line in open(blog, errors="replace"):
    m = re.match(r"\[bots\] (\S+)\s+kit=\d+ lvl=(\d+)", line)
    if m:
        final[m.group(1)] = int(m.group(2))
def persisted():
    con = sqlite3.connect(db, timeout=5)
    rows = dict(con.execute("SELECT name, level FROM characters").fetchall())
    con.close()
    return all(rows.get(n, 0) >= lvl for n, lvl in final.items())
ok = False
for _ in range(30):
    try:
        if persisted():
            ok = True
            break
    except sqlite3.OperationalError:
        pass
    time.sleep(1)
print("PERSIST_" + ("OK" if ok else "TIMEOUT"), file=sys.stderr)
PYEOF
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "M3_GATE_DONE bots_rc=$RC"
grep -E "^\[raid\]|^\[bots\] (OK|FAIL)" logs/m3_gate_bots.log || true
"$BUILD/server/bh_server" --replay-world "logs/m3_gate.bwj" \
  2>/dev/null | tail -1
