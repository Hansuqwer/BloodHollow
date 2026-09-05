#!/bin/bash
# M2b gate chain (resumable): N legs x ~10 min, DB persists characters across
# server restarts; every leg's journal must replay bit-exact.  Writes markers
# into logs/m2b_final_gate.log so an interrupted chain resumes where it left off.
set -u
cd /home/user/bloodhollow || exit 1
GATE=logs/m2b_final_gate.log
touch "$GATE"
target_hit() { grep -q 'TARGET L8 DONE' logs/m2b_final_leg*_bots.log 2>/dev/null; }

for LEG in 3 4 5 6 7 8 9 10 11 12; do
  if target_hit; then echo "gate: L8 already reached before leg $LEG" >>"$GATE"; break; fi
  if grep -q "leg$LEG verified" "$GATE"; then echo "gate: leg $LEG already verified, skip" >>"$GATE"; continue; fi
  echo "gate: leg $LEG start $(date -u +%H:%M:%S)" >>"$GATE"
  bash tools/m2b_final_leg.sh "$LEG" 7840 540 >>"$GATE" 2>&1
  R=$(./build/server/bh_server --replay-world "logs/m2b_final_leg${LEG}.bwj" 2>/dev/null | tail -1)
  echo "gate: leg $LEG replay: $R" >>"$GATE"
  case "$R" in
    *"OK"*) echo "leg$LEG verified" >>"$GATE" ;;
    *) echo "gate: leg $LEG REPLAY FAILED -> $R" >>"$GATE"; echo LEG_FAILED; exit 3 ;;
  esac
  LVL=$(grep -o 'reached L[0-9]*' logs/m2b_final_leg${LEG}_bots.log 2>/dev/null | tail -1)
  echo "gate: leg $LEG end state: $LVL" >>"$GATE"
done
echo "gate: chain done $(date -u +%H:%M:%S)" >>"$GATE"
grep -h 'TARGET L8 DONE' logs/m2b_final_leg*_bots.log >>"$GATE" 2>/dev/null
echo CHAIN_COMPLETE
