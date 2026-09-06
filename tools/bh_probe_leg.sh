#!/bin/bash
# T-049x evidence leg: BH_DUMP_ENTS live-vs-replay fingerprint diff.
# Runs a fresh 6-bot campaign session at BH_HASH_CADENCE=1 (so live emits the
# [live-rng]/[live-ply] probes EVERY tick, same phase as the replay probes),
# records the world, then replays it and diffs the whole probe stream:
#   [live-rng]/[live-ply]/[live-login]  vs  [replay-rng]/[replay-ply]/[replay-login]
# Writes logs/probe_leg.* and a one-line verdict to logs/probe_leg_verdict.txt.
# exit 0 = streams equal (relog-launderer class of divergence absent).
# NOTE: pkills bh_server like the other harnesses — never run concurrently.
set -u
cd "$(dirname "$0")/.." || exit 1
PORT=7799
DB=/tmp/bh_probe.db
J=logs/probe_leg.bwj
rm -f "$DB" "$J" logs/probe_leg_verdict.txt
pkill -x bh_server 2>/dev/null; sleep 1
BH_HASH_CADENCE=1 BH_DUMP_ENTS=1 ./build/server/bh_server --db "$DB" --port $PORT \
  --record-world "$J" > logs/probe_leg_srv.txt 2> logs/probe_leg_live.dump &
SRV=$!
sleep 2
./build/tools/bots/bh_bots --port $PORT --profile campaign --count 6 \
  --secs 45 --target-level 8 --prefix probe_ > logs/probe_leg_bots.txt 2>&1
RC=$?
kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
R=$(BH_DUMP_ENTS=1 ./build/server/bh_server --replay-world "$J" 2> logs/probe_leg_replay.dump | tail -1)
echo "probe leg bots_rc=$RC replay: $R" > logs/probe_leg_verdict.txt
# normalize tags and extract the replay subset for the same sessions/ticks
# (live emits ticks 1..N incl. final marker; replay runs to N+20 — filter
# replay to live's exact tick set before diffing)
MAXT=$(awk '/^\[live-rng\]/ { if (n < +substr($2,3)) n=+substr($2,3) } END { print n }' logs/probe_leg_live.dump)
awk "/^\\[live-(rng|ply)/ { sub(/^\\[live-/, \"[probe-\"); print }" logs/probe_leg_live.dump \
  | sort > /tmp/probe_live.txt
awk -v maxt="$MAXT" '/^\[replay-(rng|ply)/ { if (+substr($2,3) <= maxt) { sub(/^\[replay-/, "[probe-"); print } }' logs/probe_leg_replay.dump \
  | sort > /tmp/probe_replay.txt
awk '/^\[live-login/ { sub(/^\[live-/, "[probe-"); print }' logs/probe_leg_live.dump \
  | sort -k3 > /tmp/probe_live_login.txt
awk '/^\[replay-login/ { sub(/^\[replay-/, "[probe-"); print }' logs/probe_leg_replay.dump \
  | sort -k3 > /tmp/probe_replay_login.txt
if diff -q /tmp/probe_live.txt /tmp/probe_replay.txt > /dev/null && \
   diff -q /tmp/probe_live_login.txt /tmp/probe_replay_login.txt > /dev/null; then
  echo "PROBE_VERDICT equal" >> logs/probe_leg_verdict.txt
  echo "PROBE_VERDICT equal (rng=$(wc -l < /tmp/probe_live.txt) ply=$(wc -l < /tmp/probe_live.txt) login=$(wc -l < /tmp/probe_live_login.txt))"
  exit 0
fi
echo "PROBE_VERDICT diverge" >> logs/probe_leg_verdict.txt
echo "PROBE_VERDICT diverge — see /tmp/probe_live*.txt vs /tmp/probe_replay*.txt"
exit 1