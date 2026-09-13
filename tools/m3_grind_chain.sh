#!/bin/bash
# T-118 M3 gate: the pre-grind chain. Runs grind legs 1..8 (540 s each,
# persistent db) until all five bots hit the target level, then stops.
# A bot already at the level at login re-announces "reached L13", so
# counting dedupes on the bot NAME, not the line. Every leg's journal must
# replay mismatches=0 — a mismatch aborts the chain (exit 3): a
# non-deterministic leg poisons the gate's premise.
# usage: m3_grind_chain.sh [port]
set -u
PORT=${1:-7903}
cd "$(dirname "$0")/.." || exit 1
count_l13() {
  grep -h "reached L13" logs/m3_grind_leg*_bots.log 2>/dev/null \
    | awk '{print $2}' | sort -u | wc -l
}
for leg in 1 2 3 4 5 6 7 8; do
  at13=$(count_l13)
  echo "chain: leg $leg start $(date +%H:%M:%S) (at-L13: $at13/5)"
  OUT=$(bash tools/m3_grind_leg.sh "$leg" "$PORT" 540)
  echo "$OUT"
  if ! echo "$OUT" | grep -q "mismatches=0"; then
    echo "chain: leg $leg replay FAILED — aborting (exit 3)"
    exit 3
  fi
  at13=$(count_l13)
  if [ "$at13" -ge 5 ]; then
    echo "chain: all 5 at L13 after leg $leg — done"
    break
  fi
done
echo "CHAIN_COMPLETE"
