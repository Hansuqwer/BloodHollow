#!/bin/bash
# T-049 reproduction harness: cadence-25 hashing, N 6-bot 300s soaks, each
# recorded and replayed. Prints one verdict line per run and a final verdict.
# usage: t49_repro.sh <runs> — writes logs/t49_run<i>.* + logs/t49_verdict.log
set -u
cd /home/user/bloodhollow || exit 1
RUNS=${1:-3}
: > logs/t49_verdict.log
FAIL=0
for i in $(seq 1 "$RUNS"); do
  pkill -x bh_server 2>/dev/null; sleep 1
  BH_HASH_CADENCE=25 ./build/server/bh_server --db /tmp/t49_run${i}.db --port 7749 \
    --soak-secs 360 --record-world logs/t49_run${i}.bwj \
    > logs/t49_run${i}_server.log 2>&1 &
  SRV=$!
  sleep 2
  ./build/tools/bots/bh_bots --port 7749 --profile campaign --count 6 \
    --secs 300 --target-level 8 --prefix t49r${i}_ > logs/t49_run${i}_bots.log 2>&1
  RC=$?
  kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
  R=$(./build/server/bh_server --replay-world logs/t49_run${i}.bwj 2>/dev/null | tail -1)
  # reader determinism: replay twice, demand identical output
  R2=$(./build/server/bh_server --replay-world logs/t49_run${i}.bwj 2>/dev/null | tail -1)
  echo "run$i bots_rc=$RC" >> logs/t49_verdict.log
  echo "run$i replay1: $R" >> logs/t49_verdict.log
  echo "run$i replay2: $R2" >> logs/t49_verdict.log
  case "$R" in *"OK"*) : ;; *) FAIL=1;; esac
  if [ "$R" != "$R2" ]; then echo "run$i READER UNSTABLE" >> logs/t49_verdict.log; FAIL=1; fi
done
echo "T49_VERDICT fail=$FAIL" >> logs/t49_verdict.log
