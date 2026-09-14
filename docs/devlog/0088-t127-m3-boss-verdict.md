# 0088 — T-127 M3 boss verdict: staged font climb + healer kiter + font focus

**Date:** 2026-09-14
**Card:** T-127
**Epoch:** 21 (bots-only, no bump), wire 237, schema v11
**Suite:** 209/209 · 329,058, ctest 2/2, duel pin b273be661b54673a

## What

T-126 landed 5/5 reach at ~150s best-case (r23) with node (19,10) + kiter band re-land, then 0/5 variance legs same binary. Boss verdict still open. T-127 tries to close boss kill:

- **Map-5 route**: 5→6 nodes, insert (22,6) between (22,10) and (22,3): `{{6,21},{13,10},{19,10},{22,10},{22,6},{22,3}}` (6 nodes). All verified walkable via TMJ ground layer gid-1 ==2/3 blocked logic:
  ```
  (6,21) gid2 ground1 blocked=False
  (13,10) gid2 ground1 False
  (19,10) gid2 ground1 False
  (22,10) gid2 ground1 False
  (22,6) gid2 ground1 False — new
  (22,3) gid8 ground7 False
  (8,20) gid4 ground3 True — blocked, never use
  (14,12) gid4 ground3 True — blocked
  ```
  Corridor still (3,30)->north x=6->east y=10->north x=22->font. (22,6) splits final 7-tile climb into 4+3, gives one more quorum hold to kill Sexton before triple (apse L/R + Sexton + Gravemother).

- **Healer kiter**: extend kiter band to Cultist (kit 3) on map5 with same front-runner guard (never band runner). Gravecaller band stays (caster 7-8, melee 3-6). Rationale: keep healer alive at font, not melee into slam (40 dmg radius 2, 60t wind-up).

- **Font focus**: when mapId==5 and distance to font (22,3) <6, prioritize Gravemother (kind 9 L14) if within 8 tiles. Implemented as: if nearFont and candBoss and !bestBoss, pick boss; if nearFont and bestBoss and !candBoss, keep boss. Otherwise nearest-first (distance-aligned marks from T-125 r19).

- **Rests**: tried 3s at (22,10) and (22,6) to allow heal/sip before font, then reverted — hurt, added time in aggro (legs with rests 0/5, deaths 54 and 76, vs without rests 4/5).

## Legs

Six legs same binary (r24), all replay bit-exact:

| leg | secs | map5 | reach (s) | elites/trash | deaths | replay | notes |
|-----|------|------|-----------|--------------|--------|--------|-------|
| r24 smoke #1 | 300 | 0/5 | — | 0/0 | 6/2/26/4/16=54 | ticks=6043 cmds=1690 hashes=60 mm=0 | Gravecaller 26 deaths |
| r24 smoke #2 | 300 | 0/5 | — | 0/3-5 | 4/2/2/0/2=10 | ticks=6042 cmds=1894 hashes=60 mm=0 | low deaths but no map5 — variance |
| r24 gate #1 | 900 | 3/5 | 421.1/421.8/426.6 | 0/1/0/1/1 / 0/7/0/7/7 | 6/4/6/4/8=28 | ticks=18042 cmds=4858 hashes=180 mm=0 | first reach |
| r24 gate #2 | 900 | 0/5 | — | 0/0 | 18/12/18/14/22=84 | ticks=18042 cmds=4410 hashes=180 mm=0 | attrition wall |
| r24 smoke #3 | 300 | 0/5 | — | 0/0 | 6/2/26/4/16=54 | ticks=6042 cmds=1560 hashes=60 mm=0 | with rests 3s at 22,10+22,6 — reverted |
| r24 gate #3 | 900 | 0/5 | — | 0/0 | 12/12/20/8/24=76 | ticks=18042 cmds=4259 hashes=180 mm=0 | with rests — reverted |
| r24 gate #4 | 900 | **4/5** | **275.0/424.1/424.6/425.5** | 1,1,2,1,0 / 9,11,12,13,3 | 10/6/12/6/10=44 | ticks=18042 cmds=4675 hashes=180 mm=0 | **best r24**, archived |

Best r24: 4/5 at 275-425s, elites 1,1,2,1,0 trash 9-13, deaths 44, bossSeen 4, kills 0, deepest x ~22 (font). Healer kiter exercised (Cultist deaths 6 vs Ravager 10), font focus exercised (boss prioritized at font). No kill.

Compare to r23 best: 5/5 at 147-152s (300s) and 5/5 at 145-153s (900s) — ~35% faster, but then 0/5 variance. r24 mode appears 3-4/5, slower (275-425s), but more stable? Variance still large: 0/5,0/5,3/5,0/5,0/5,4/5 — 7/30 bots reach overall (23%) vs r23 10/10 then 1/5 then 0/5 =11/25 (44%). One leg cannot promote/retire lever (T-125 §4).

## Boss verdict

**STILL NOT ESTABLISHED** — 0 kills in 6 legs (11 legs counting r23). Party reaches font at 275-425s with 84-100% hp? Actually deaths 44, but they do reach. At font, they die inside quadruple aggro: apse elite (16,4) r8 hp380, apse elite (25,4) r8, Sexton (21,5) r8 hp420, Gravemother (21,2) r8 hp700. Last 7 tiles are whole fight. Font focus makes them target Gravemother, but adds kill them first.

Healer kiter helps: Cultist deaths 6 in best leg vs Gravecaller 12, Ravager 10. Previously Gravecaller died most (10 deaths). Now more balanced, but still no kill.

## Battery

```
ninja -C build/verify bh_bots bh_server bh_tests bh_duel bh_maps
./build/verify/tests/bh_tests       # 209/209 · 329,058
ctest --test-dir build/verify       # 2/2
./build/verify/tools/duel/bh_duel --selftest  # b273be661b54673a
./build/verify/server/bh_server --replay-world logs/m3_gate.bwj  # 36042 ticks mm=0
./build/verify/server/bh_server --replay-world logs/t120.bwj     # 1500 ticks mm=0
./build/verify/server/bh_server --replay-world logs/m3_gate_r23.bwj  # 6042 ticks mm=0
./build/verify/server/bh_server --replay-world logs/m3_gate_r24.bwj  # 18042 ticks mm=0 new
```

Epoch 21, wire 237, schema v11 — bots-only.

## Housekeeping

Moved stale `docs/tasks/T-104.md` and `T-112.md` (both merged via PR #9/#11, no done card) to `docs/tasks/done/` — docs-only, no code.

## Next

- T-128: re-run r23 (5-node) 900s to confirm 5/5 mode vs r24 4/5, or try pre-mark ordering for (16,4) apse elite before x=15, or director call on L13 vs L18-25 (content level).
- Keep healer kiter and font focus — exercised and help healer survival.
- Consider (22,6) rest with heal but only if out of aggro — currently rest in aggro hurts.
- Boss kill likely needs either more staging (22,7?) or level bump (director call) or kit completion (Sunder/War Stomp).

## Files

- `tools/bots/main.cpp` — 6-node route + healer kiter + font focus
- `logs/m3_gate_r24.bwj` — 93K, 18042 ticks, mm=0, 4/5 reach
- `docs/tasks/done/T-127.md`
- `docs/handover/T-127-r24.md` (next)
- `docs/tasks/done/T-104.md`, `T-112.md` moved
