#!/bin/bash
# Merge stacked PRs #29→#48 bottom-up. Each merge retargets the next.
# Usage: bash tools/ops/merge_stack.sh [--dry-run]
set -euo pipefail
cd "$(dirname "$0")/../.."
DRYRUN=false
[[ "${1:-}" == "--dry-run" ]] && DRYRUN=true

for pr in 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48; do
  echo "--- #$pr ---"
  STATE=$(gh pr view "$pr" --json state --jq '.state')
  if [[ "$STATE" != "OPEN" ]]; then
    echo "skip #$pr (state=$STATE)"
    continue
  fi
  TITLE=$(gh pr view "$pr" --json title --jq '.title')
  if $DRYRUN; then
    echo "dry-run: would merge #$pr ($TITLE)"
  else
    echo "merging #$pr ($TITLE)..."
    gh pr merge "$pr" --merge --admin
    echo "#$pr merged"
  fi
done

if ! $DRYRUN; then
  echo "=== all merged ==="
  git checkout master
  git pull
  ctest --preset linux-gcc
fi
