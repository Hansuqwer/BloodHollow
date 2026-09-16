#!/bin/bash
# T-137 M4 gate leg: attackers + defenders flip the castle, p99 metered.
# usage: t137_m4_leg.sh [port] [attackers] [defenders] [secs]
# Rehearsal server, disclosed L15 top-up both sides (server-down, like
# m3_topup). M4 PASS: flips >= 1 (crowned lines) + tick p99 < 25 ms.
set -e
PORT=${1:-7836}
ATK=${2:-12}
DEF=${3:-6}
SECS=${4:-750}
TAG=${5:-m4}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t137_${TAG}.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t137_${TAG}.bwj logs/t137_${TAG}_server.log logs/t137_${TAG}_bots*.log
mkdir -p logs
# T-154/T-168 one-build-dir law: BH_BUILD_DIR overrides (headless CI).
BUILD_DIR=${BH_BUILD_DIR:-build/linux-gcc}

echo "[t137] login wave A ($ATK attackers)..."
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs 120 > logs/t137_${TAG}_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count $ATK --secs 20 --profile wander --prefix t137a_ > logs/t137_${TAG}_bots0a.log 2>&1 || true
# T-109 limiter: 30 new accounts per 60 s per IP — stagger the waves.
echo "[t137] limiter window (65 s)..."
sleep 65
echo "[t137] login wave D ($DEF defenders)..."
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count $DEF --secs 20 --profile wander --prefix t137d_ > logs/t137_${TAG}_bots0d.log 2>&1 || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t137] server-down top-up (disclosed staging)..."
T137_WANT=$((ATK + DEF)) python3 - "$DB" <<'PY'
import sqlite3, sys, os
con = sqlite3.connect(sys.argv[1])
cur = con.cursor()
names = [r[0] for r in cur.execute("SELECT name FROM characters ORDER BY id")]
want = [n for n in names if n.startswith("t137a_") or n.startswith("t137d_")]
need = int(os.environ.get("T137_WANT", "0"))
assert len(want) == need, f"expected {need} t137 characters, got {len(want)}"
blob = "2002:1:1:0:100:0:0;2102:1:1:0:100:0:0;3001:16:0:0:100:0:0;"
marks = ",".join("?" * len(want))
cur.execute(f"UPDATE characters SET level=15, xp=0, str=24, vit=24, dex=14, gold=600, inv=? WHERE name IN ({marks})",
            [blob] + want)
con.commit()
print(f"[t137] topped up {len(want)}: {want[:3]}...")
PY

echo "[t137] rehearsal run recording epoch 29 ($ATK atk + $DEF def, ${SECS}s)..."
# T-157-F1(a): the drill horns via gm siege-start (bot0), which T-152 gates on
# the operator allowlist — seat bot0 as drill GM (disclosed staging, same class
# as the server-down top-up above). BH_GM_NAMES is allowlist additive.
export BH_GM_NAMES=t137a__00
./$BUILD_DIR/server/bh_server --db $DB --port $PORT --soak-secs $((SECS + 30)) --siege-rehearsal --record-world logs/t137_${TAG}.bwj > logs/t137_${TAG}_server.log 2>&1 &
SRV=$!
sleep 2
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count $ATK --secs $SECS --profile siege --prefix t137a_ > logs/t137_${TAG}_botsA.log 2>&1 &
BOTA=$!
./$BUILD_DIR/tools/bots/bh_bots --port $PORT --count $DEF --secs $SECS --profile siege --prefix t137d_ --defenders $DEF > logs/t137_${TAG}_botsD.log 2>&1 || true
# T-157-F1(b) abort-fast: the horn must land shortly after muster (~60s) + march;
# a missing "battle joined" at 240s means another quiet refusal — fail in 4 min.
sleep 240
if ! grep -q "battle joined" logs/t137_${TAG}_server.log; then
  echo "[t137:$TAG] ABORT: no battle joined at 240s (horn refused again?)"
  kill $BOTA 2>/dev/null; kill $SRV 2>/dev/null; wait 2>/dev/null || true
  exit 1
fi
echo "[t137:$TAG] horn OK: battle joined, running out the clock..."
wait $BOTA 2>/dev/null || true
sleep 2
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true  # || true: soak may beat us here (set -e)

echo "[t137:$TAG] assertions:"
grep -c "battle joined" logs/t137_${TAG}_server.log | xargs -I{} echo "battles: {}"
grep -c "breached by" logs/t137_${TAG}_server.log | xargs -I{} echo "gates-down: {}"
grep -c "heartstone attuned at tick" logs/t137_${TAG}_server.log | xargs -I{} echo "attuned: {}"
grep -c "crowned:" logs/t137_${TAG}_server.log | xargs -I{} echo "flips: {}"
grep "SIEGE" logs/t137_${TAG}_botsA.log | tail -1
echo -n "p99us-max: "
grep -oE "p99=[0-9]+" logs/t137_${TAG}_server.log | cut -d= -f2 | sort -n | tail -1
echo "[t137:$TAG] replay check:"
./$BUILD_DIR/server/bh_server --siege-rehearsal --replay-world logs/t137_${TAG}.bwj 2>&1 | tail -2
