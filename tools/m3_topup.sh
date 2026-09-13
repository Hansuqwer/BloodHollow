#!/bin/bash
# T-118: disclosed top-up to L13. The real grind legs (their journals) are
# the in-world-mechanism evidence of record; this sets the five m3g_
# characters to level 13 / 0 xp in the PERSISTENT db so the gate leg
# starts at the level the boss fight is designed for. Disclosed in the
# devlog — the server is STOPPED when this runs (no save races).
# usage: m3_topup.sh
set -u
cd "$(dirname "$0")/.." || exit 1
DB=${DB:-"$PWD/build/m3_gate.db"}
[ -f "$DB" ] || { echo "no db at $DB"; exit 1; }
pgrep -x bh_server > /dev/null && { echo "bh_server is RUNNING — stop it first"; exit 1; }
python3 - "$DB" <<'EOF'
import sqlite3, sys
db = sys.argv[1]
con = sqlite3.connect(db)
cur = con.cursor()
cur.execute("SELECT name, level, xp FROM characters ORDER BY id")
before = cur.fetchall()
names = [n for (n, l, x) in before if n.startswith("m3g_")]
if len(names) != 5:
    print(f"expected 5 m3g_ characters, found {len(names)}: {names}")
    sys.exit(1)
# gold=600: the vendor belt is 16 vials x 30g = 480g; 600 leaves coin for
# the anvil. The gauntlet runs on the flask belt (the grind's 240g gate
# meant the party arrived with empty belts).
marks = ",".join("?" * len(names))
cur.execute(f"UPDATE characters SET level=13, xp=0, stat_points=0, gold=600 WHERE name IN ({marks})", names)
# The T-114 flask belt: 16 Blood Vials (stackMax 16). The gate's party
# cannot wait on a town vendor mid-gauntlet, so the belt is stocked here
# (blob grammar "iid:qty:equipped:aura:durability:affix:refine;").
cur.execute(f"SELECT id, inv FROM characters WHERE name IN ({marks})", names)
for (cid, inv) in cur.fetchall():
    if "3001:" not in (inv or ""):
        con.execute("UPDATE characters SET inv = inv || ? WHERE id = ?",
                    ("3001:16:0:0:100:0:0;", cid))
con.commit()
cur.execute(f"SELECT name, level, xp FROM characters WHERE name IN ({marks}) ORDER BY name", names)
print("before:", before)
print("after :", cur.fetchall())
con.close()
EOF
