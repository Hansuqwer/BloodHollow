#!/bin/bash
# T-118 M3 gate: one pre-grind leg. Five campaign bots (mixed kits via
# --party-size 5, target L13) on a PERSISTENT db: same names across legs,
# so level/gear carry over (the wipe-proof the M2b chain used). Fresh server
# per leg; journal logs/m3_grind_leg<N>.bwj must replay bit-exact.
# usage: m3_grind_leg.sh <legN> <port> [secs]
set -u
LEG=${1:?leg number}
PORT=${2:?port}
SECS=${3:-540}
cd "$(dirname "$0")/.." || exit 1
BUILD=${BUILD_DIR:-build/verify}
DB=${DB:-"$PWD/build/m3_gate.db"}
pkill -x bh_server 2>/dev/null; sleep 1
"$BUILD/server/bh_server" --db "$DB" --port "$PORT" \
  --soak-secs $((SECS + 60)) --record-world "logs/m3_grind_leg${LEG}.bwj" \
  > "logs/m3_grind_leg${LEG}_server.log" 2>&1 &
SRV=$!
sleep 2
"$BUILD/tools/bots/bh_bots" --port "$PORT" --profile campaign --count 5 \
  --secs "$SECS" --party-size 5 --target-level 13 --prefix m3g_ \
  > "logs/m3_grind_leg${LEG}_bots.log" 2>&1
RC=$?
# T-118: the server saves each character on disconnect (dropSession) — but
# only AFTER it has seen the ENet peer drop. Killing it the instant the
# bots exit raced the save and lost the level (leg 2 restarted at L1).
# Wait for the DB to show every bot at its final reported level before
# stopping the server.
python3 - "$DB" "logs/m3_grind_leg${LEG}_bots.log" <<'PYEOF'
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
        pass  # writer holds the lock; retry
    time.sleep(1)
print("PERSIST_" + ("OK" if ok else f"TIMEOUT {final}"), file=sys.stderr)
sys.exit(0 if ok else 1)
PYEOF
PERSIST=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
echo "M3_GRIND_LEG${LEG}_DONE bots_rc=$RC persist=$PERSIST"
grep -E "reached L|^\[bots\] (OK|FAIL)" \
  "logs/m3_grind_leg${LEG}_bots.log" | tail -6
"$BUILD/server/bh_server" --replay-world "logs/m3_grind_leg${LEG}.bwj" \
  2>/dev/null | tail -1
