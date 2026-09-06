#!/bin/bash
# M2b rerun chain (post-T-034b retune): fresh L1->8 cross-zone pacing run to
# measure whether the retune moves the L4-L6 plateau.  Resumable: skips legs
# already verified in the gate log; stops early if L8 is reached.
# Same cadence as the original chain (2 bots, campaign, ~9 min/leg) for an
# apples-to-apples comparison against legs 3-12 of the closed chain.
set -u
cd "$(dirname "$0")/.." || exit 1
GATE=logs/m2b_rerun_gate.log
DB=/tmp/m2b_rerun_campaign.db
PORT=7851
LEGS=12
SECS=540
: > "$GATE"
rm -f "$DB"   # fresh characters for a clean pacing measurement
target_hit() { grep -q 'TARGET L8 DONE' logs/m2b_rerun_leg*_bots.log 2>/dev/null; }
for LEG in $(seq 1 "$LEGS"); do
  if target_hit; then echo "gate: L8 reached before leg $LEG" >>"$GATE"; break; fi
  if grep -q "leg$LEG verified" "$GATE"; then echo "gate: leg $LEG already verified, skip" >>"$GATE"; continue; fi
  echo "gate: leg $LEG start $(date -u +%H:%M:%S)" >>"$GATE"
  pkill -x bh_server 2>/dev/null; sleep 1
  ./build/server/bh_server --db "$DB" --port "$PORT" \
    --soak-secs $((SECS + 60)) --record-world "logs/m2b_rerun_leg${LEG}.bwj" \
    > "logs/m2b_rerun_leg${LEG}_server.log" 2>&1 &
  SRV=$!
  sleep 2
  ./build/tools/bots/bh_bots --port "$PORT" --profile campaign --count 2 \
    --secs "$SECS" --target-level 8 --prefix m2br_ > "logs/m2b_rerun_leg${LEG}_bots.log" 2>&1
  RC=$?
  echo "FINAL_RERUN_LEG${LEG}_DONE bots_rc=$RC" >>"$GATE"
  kill $SRV 2>/dev/null; wait $SRV 2>/dev/null
  R=$(./build/server/bh_server --replay-world "logs/m2b_rerun_leg${LEG}.bwj" 2>/dev/null | tail -1)
  echo "gate: leg $LEG replay: $R" >>"$GATE"
  case "$R" in
    *"OK"*) echo "leg$LEG verified" >>"$GATE" ;;
    *) echo "gate: leg $LEG REPLAY FAILED -> $R" >>"$GATE"; echo LEG_FAILED; exit 3 ;;
  esac
  LVL=$(grep -o 'reached L[0-9]*' logs/m2b_rerun_leg${LEG}_bots.log 2>/dev/null | tail -1)
  echo "gate: leg $LEG end state: $LVL" >>"$GATE"
done
echo "gate: chain done $(date -u +%H:%M:%S)" >>"$GATE"
grep -h 'TARGET L8 DONE' logs/m2b_rerun_leg*_bots.log >>"$GATE" 2>/dev/null
echo CHAIN_COMPLETE
