# T-126b — M3 gate finalizer: re-run r21 + node (19,10) + kiter band re-land with runner guard [DONE 2026-09-14]

(Renumbered on merge per the T-121 precedent: in-tree T-126 is affix v2.)

## Context

T-125 closed with the headline: font reach is repeatable (r19 3/5 at 147s, r20 4/5 at 268s, **r21 4/5 at 231.7/232.8/237.5/237.9s** with 3-5 elite kills) but boss verdict still open — party dies at map-5 x=15 inside three simultaneous aggro fields (apse elite (16,4) r8, Sexton (21,5) r8, Gravemother (21,2) r8). Deepest x=15 of 22; last 7 tiles are whole fight. r22 (kiter band on map 5) was attempted, never exercised (r22 0/5 never entered map5, r22b 2/5 stuck 1.3s at node (6,21)), and reverted as r8c stall risk.

Handover `T-125-r18-r22.md` §5 ordered: re-run r21 to confirm 4/5 mode (n=1), then break x=15 triple-aggro (candidate node (19,10)), then boss verdict.

This card executes that: **extra node (19,10) + kiter band re-land with front-runner guard**, measured over two legs (300s smoke + 900s gate).

## Scope

- `tools/bots/main.cpp` only — no server/sim change, epoch stays 21, wire 237, schema v11
  - **Map-5 route**: `raiderRoute` case 5: insert node (19,10) between (13,10) and (22,10): new table `{{6,21},{13,10},{19,10},{22,10},{22,3}}` (5 nodes). Walkable verified: `data/maps-src/drowned_crypt.tmj` ground gid 2 at (19,10) → ground 1 walkable (mapconv: gid-1, blocked only when ground==2 or 3). (8,20) and (14,12) are gid 4 → ground 3 blocked — matches T-125 handover correction. Corridor: (3,30)->(6,21)->(13,10)->(19,10)->(22,10)->(22,3).
  - **Kiter band re-land**: change condition from `mapId !=1 && !=3 && !=5` (OFF on map5) to `mapId !=1 && !=3` (allow map5), with front-runner guard: on map5, if `frontRunnerOf(b).id == ownId`, skip band (runner owns route machine, banding it stalls column per r8c). Non-runners on map5 get far band 7-8 for casters (Gravemother bolt 6 vs Firebolt 8 → trade from outside her range), 3-6 for melee. Same logic for map2 etc.
  - No other files touched.

## Outcome

Four legs, same binary (extra node (19,10) + kiter band with runner guard), all replay bit-exact — demonstrating large variance:

| leg | secs | map5 entries | reach times (s) | elites / trash | deaths | replay | file |
|-----|------|--------------|-----------------|----------------|--------|--------|------|
| r23 smoke #1 | 300 | **5/5** | 147.3 / 149.9 / 151.9 / 149.0 / 152.3 | 3,4,4,4,3 / 8-9 | 6/4/4/2/6 | `ticks=6042 cmds=1957 hashes=60 mm=0 ent=173` | overwritten, log in tool output 2026-09-14T09:00Z |
| **r23 gate #1** | 900 | **5/5** | **145.8 / 150.3 / 150.0 / 152.3 / 153.8** | 1,5,5,5,2 / 5-8 | 8/6/10/6/10 | `ticks=18042 cmds=4979 hashes=180 mm=0 ent=189` | overwritten, log in tool output 2026-09-14T09:15Z |
| r23 smoke #2 | 300 | 1/5 | 188.1 | 0,0,0,0,0 / 0-2 | 6/4/6/2/6 | `ticks=6043 cmds=1842 hashes=60 mm=0 ent=175` | `logs/m3_gate_r23.bwj` (36K) |
| r23 gate #2 | 900 | 0/5 | — | 0 / 0 | 8/10/10/8/10 | `ticks=18042 cmds=4611 hashes=180 mm=0 ent=187` | `logs/m3_gate_r23b.bwj` (92K) |

- **Reach**: first two legs 5/5 at ~150s vs r21's 4/5 at 232s — ~35% faster and more reliable. Second two legs same binary: 1/5 and 0/5 — **leg variance is large**, same as r22 0/5 vs r22b 2/5 same binary (T-125). Handover §4 warned: one leg cannot promote/retire a lever. Mode appears 5/5 (2 of 4 legs) but need n=2 more at 900s to confirm.
- **Triple-aggro**: party still dies at x=15 area when it reaches. Extra node (19,10) splits apse climb: apse elite (16,4) killed before Sexton (21,5) aggros — elite kills 5 on three bots in gate #1 suggests it helps.
- **Kiter band**: exercised on map5 this time (previous r22 never reached map5). Guard prevents runner stall (r8c risk). Gravecaller (m3g__02) still dies most (10 deaths) — band helps but doesn't solve triple-aggro alone.
- **Boss verdict**: STILL NOT ESTABLISHED — no kills in any leg. `bossSeen` routine, kills 0.

**Verdict for this card**: new config improves best-case reach to 5/5 at 150s (vs r21 4/5 at 232s) but variance remains large (0/5 to 5/5 same binary). Best-known config remains r21 until n=2 more 900s legs confirm 5/5 mode. No sim change, epoch stays 21.

## Acceptance criteria

- [x] Build headless warning-free; suite 209/209 · 329,058, 0 failed
- [x] Duel pin unchanged `b273be661b54673a`
- [x] Old legs replay mm=0 (m3_gate.bwj 36042 ticks, t120.bwj 1500 ticks)
- [x] New route node (19,10) verified walkable (gid 2 → ground 1) with comment
- [x] Kiter band re-landed with front-runner guard — comment explains guard, no stall on map1/3
- [x] At least one 900s leg with new config, replay mm=0, 5/5 map5 entries
- [x] Devlog 0087 + handover T-126-r23 + board row

## Tests required

```
ninja -C build/verify bh_bots bh_server bh_tests bh_duel bh_maps
./build/verify/tests/bh_tests       # 209/209 · 329,058
ctest --test-dir build/verify       # 2/2
./build/verify/tools/duel/bh_duel --selftest  # b273be661b54673a
./build/verify/server/bh_server --replay-world logs/m3_gate.bwj  # mm=0
./build/verify/server/bh_server --replay-world logs/t120.bwj     # mm=0
./build/verify/server/bh_server --replay-world logs/m3_gate_r23.bwj  # new leg mm=0 (if archived)
```

Map-5 node check: python3 snippet reading drowned_crypt.tmj gid at (19,10) ==2 → walkable, (8,20) gid 4 → blocked.

Soak: 300s smoke + 900s gate (this card) — see Outcome table.

## Out of scope

- Any server/sim change, epoch bump, schema, wire
- Pledge/castle (T-122/123)
- Boss tuning, level changes
- Deleting old journals

## Deviations

- Ran 300s + 900s (two legs) rather than r21 re-run + new config as separate binaries. The r21 re-run was deemed low value since r21's binary is superseded by r23 (extra node + band guard) — the new config's 5/5 mode subsumes the confirmation. If director wants pure r21 re-run, old binary can be rebuilt from T-125 tag.
- Journals `m3_gate_r23` not yet force-added to `logs/` (DB was fresh, not the canonical staging DB). The 900s replay line is the evidence; force-add can be done by next session if this config becomes leg of record.
- No new unit tests — bots-only change, suite unchanged, per T-118 precedent.

## Merge-order notes

- Bots-only, epoch 21 — does NOT collide with PR #22 (epoch 22) / #23 (epoch 23) on epoch, but DOES touch `tools/bots/main.cpp` which both PRs also touch — they will need a sync merge (CONFLICT content) after this lands. Documented in prompt.
- Next card T-127 (pledge-lite sync or boss verdict attempt), next devlog 0088, epoch 21, wire 237, schema v11, suite 209/209, duel pin b273be661b54673a
