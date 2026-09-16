#!/bin/bash
# bh_backup.sh — hot backup + rotation + restore drill for the alpha world DB.
# usage:
#   bash tools/ops/bh_backup.sh backup [db] [backup-dir]   # sqlite .backup + manifest
#   bash tools/ops/bh_backup.sh restore <backup> <dest>    # restore + integrity check
#   bash tools/ops/bh_backup.sh drill [db]                 # backup+restore+verify, prints verdict
# The server keeps running: sqlite3 .backup takes a live consistent snapshot
# (WAL-safe). Journals (*.bwj) are append-only history — copy them alongside.
set -euo pipefail
cd "$(dirname "$0")/../.."

CMD=${1:-drill}
DB=${2:-/var/lib/bloodhollow/world.bhdb}
BACKUP_DIR=${3:-/var/backups/bloodhollow}
STAMP=$(date -u +%Y%m%dT%H%M%SZ)

db_backup() {
  local db=$1 dir=$2
  mkdir -p "$dir"
  local out="$dir/world-$STAMP.bhdb"
  if [ ! -f "$db" ]; then
    echo "[backup] FAIL: no DB at $db" >&2
    return 1
  fi
  sqlite3 "$db" ".backup '$out'"
  sha256sum "$out" > "$out.sha256"
  sqlite3 "$out" "PRAGMA integrity_check;" | grep -q '^ok$' \
    && echo "[backup] $out (integrity ok)" >&2 \
    || { echo "[backup] FAIL: integrity_check on $out" >&2; return 1; }
  # rotation: keep newest 14 dailies, thin the rest (alpha policy, runbook §4)
  ls -t "$dir"/world-*.bhdb 2>/dev/null | tail -n +15 | xargs -r rm -f
  echo "$out"
}

db_restore() {
  local backup=$1 dest=$2
  if [ ! -f "$backup" ]; then
    echo "[restore] FAIL: no backup at $backup" >&2
    return 1
  fi
  cp "$backup" "$dest"
  rm -f "$dest-wal" "$dest-shm"
  local verdict
  verdict=$(sqlite3 "$dest" "PRAGMA integrity_check;")
  if [ "$verdict" != "ok" ]; then
    echo "[restore] FAIL: integrity_check = $verdict" >&2
    return 1
  fi
  local counts
  counts=$(sqlite3 "$dest" "SELECT 'accounts='||(SELECT COUNT(*) FROM accounts)||' chars='||(SELECT COUNT(*) FROM characters)||' pledges='||(SELECT COUNT(*) FROM pledges)||' uv='||(SELECT user_version FROM pragma_user_version);" 2>/dev/null || echo "counts-unavailable")
  echo "[restore] $dest ok ($counts)"
}

case "$CMD" in
  backup) db_backup "$DB" "$BACKUP_DIR" ;;
  restore) db_restore "${3:?usage: restore <backup> <dest>}" "${4:?usage: restore <backup> <dest>}" ;;
  drill)
    echo "[drill] source: $DB"
    OUT=$(db_backup "$DB" "${3:-/tmp/bh_drill}")
    db_restore "$OUT" "${3:-/tmp/bh_drill}/world-restored.bhdb"
    echo "[drill] PASS: backup + restore + integrity verified"
    ;;
  *) echo "usage: $0 {backup|restore|drill} ..." >&2; exit 2 ;;
esac
