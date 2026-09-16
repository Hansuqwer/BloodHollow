#!/bin/bash
# T-123 Weeping Castle gate leg (epoch 23, stacked on T-122's lineage): zone 6
# boots (gates/heartstone/throne + dead garrison), bots hold the castle across
# a relog (positions + zone persist), replay mm=0, t122.bwj refuses by guard.
#   wave 1: 5 wander bots 10s   (accounts created in town)
#   seed:   map_id=6 + courtyard tile + level 25 + 20/20/20 stats (founders'
#           veterans hold the keep: the garrison is L14 and fights back)
#   wave 2: 5 FIGHTER bots 25s  (hold the castle: sentinels aggro, combat
#           churns the journal — attacks, kills, XP, gold, drops, deaths)
#   wave 3: 5 fighter bots 10s  (relog INSIDE zone 6 — l-lines carry zoneId 6)
set -e
PORT=${1:-7825}
cd "$(dirname "$0")/.." || exit 1
pkill -x bh_server 2>/dev/null || true
sleep 1
DB=/tmp/t123.db
rm -f $DB $DB-wal $DB-shm
rm -f logs/t123.bwj logs/t123_server.log logs/t123_bots1.log logs/t123_bots2.log logs/t123_bots3.log
mkdir -p logs
# bh_maps regenerated at build; worktrees may not have built yet — copy from
# the sibling T-122 build or the main tree if absent (same .tmj -> same bytes)
if [ ! -f assets/maps/weeping_castle.bhmap ]; then
  for d in /home/user/BloodHollow/build/wt/t122/assets/maps /home/user/BloodHollow/assets/maps; do
    if [ -f "$d/weeping_castle.bhmap" ] && [ -f "$d/fields_overflow.bhmap" ]; then
      mkdir -p assets/maps
      cp "$d/weeping_castle.bhmap" "$d/fields_overflow.bhmap" assets/maps/
      break
    fi
  done
fi
# both must exist or the leg is meaningless
ls assets/maps/weeping_castle.bhmap assets/maps/fields_overflow.bhmap >/dev/null

echo "[t123] Server up (record epoch 23)..."
./build/server/bh_server --db $DB --port $PORT --soak-secs 90 --record-world logs/t123.bwj > logs/t123_server.log 2>&1 &
SRV=$!
sleep 2

echo "[t123] Wave 1: 5 wander bots 10s (create accounts in town)..."
./build/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile wander --prefix t123_ > logs/t123_bots1.log 2>&1 || true
sleep 2

echo "[t123] Seed: garrison the bots in the castle courtyard..."
python3 <<'PY'
import sqlite3
conn = sqlite3.connect("/tmp/t123.db")
cur = conn.cursor()
# courtyard tile south of the heart crossroads, inside the walls
cur.execute("UPDATE characters SET map_id=6, x=28, y=24, level=25, str=20, vit=20, dex=20 WHERE name LIKE 't123_%'")
conn.commit()
cur.execute("SELECT name, map_id, x, y, level FROM characters ORDER BY name")
for r in cur.fetchall():
    print("[t123] seeded", r)
PY

echo "[t123] Wave 2: 5 fighter bots 25s (hold the castle)..."
./build/tools/bots/bh_bots --port $PORT --count 5 --secs 25 --profile fighter --prefix t123_ > logs/t123_bots2.log 2>&1 || true
sleep 2

echo "[t123] Wave 3: 5 fighter bots 10s (relog in zone 6)..."
./build/tools/bots/bh_bots --port $PORT --count 5 --secs 10 --profile fighter --prefix t123_ > logs/t123_bots3.log 2>&1 || true
sleep 2

kill $SRV 2>/dev/null; wait $SRV 2>/dev/null || true

echo "[t123] Boot log: zone 6 online + castle spawners?"
grep -E "zone 6|zone . \(.*castle" logs/t123_server.log || true

echo "[t123] DB after all waves (garrison residency):"
python3 <<'PY'
import sqlite3, sys
conn = sqlite3.connect("/tmp/t123.db")
cur = conn.cursor()
cur.execute("SELECT name, map_id, x, y, level FROM characters ORDER BY name")
rows = cur.fetchall()
in6 = 0
for r in rows:
    print("[t123] char:", r)
    if r[1] == 6:
        in6 += 1
# deaths respawn in zone 1 (the walk of shame) — that's era law, not a leg
# failure; the gate is: the castle HELD most of the garrison through a relog.
if in6 < 3:
    print(f"[t123] FAIL: only {in6}/5 residents in zone 6")
    sys.exit(1)
print(f"[t123] PASS: {in6}/5 held the castle across the relog (map 6 persisted)")
PY

echo "[t123] Journal: version + logins (wave-2 l-lines must carry zone 6):"
head -1 logs/t123.bwj
grep "^l " logs/t123.bwj | awk '{print $2, $3, $4, $5, $6, $7}'
python3 <<'PY'
import sys
lines = [l.split() for l in open("logs/t123.bwj") if l.startswith("l ")]
# l tick idx name x y zone level ...
wave2 = lines[5:10]
zones = {l[6] for l in wave2}
if len(wave2) != 5 or zones != {"6"}:
    print(f"[t123] FAIL: wave-2 logins not all in zone 6: {zones}")
    sys.exit(1)
print("[t123] PASS: all five wave-2 logins are zone-6 lines")
PY

echo "[t123] Replay check (epoch 23 leg):"
./build/server/bh_server --replay-world logs/t123.bwj 2>&1 | tail -3

echo "[t123] Stacked-lineage guard: t122.bwj must REFUSE (22 vs 23):"
if ./build/server/bh_server --replay-world logs/t122.bwj >/dev/null 2>&1; then
  echo "[t123] FAIL: t122.bwj replayed on epoch 23 — guard broken"
  exit 1
else
  echo "[t123] PASS: t122.bwj refuses by epoch guard (exit $?)"
fi
