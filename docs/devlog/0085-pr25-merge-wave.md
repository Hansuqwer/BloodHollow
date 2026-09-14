# 0085 — The gate series lands, and the boss verdict stays open (T-124)

The director said merge, so the wave ran: **PR #25 squash-merged to master as
`2a813ec`**, subject kept as the PR title. PR #24 — the older r8 lineage,
CONFLICTING since its four r8 commits were cherry-picked into #25's series —
was closed unmerged with a comment pointing at the successor. One branch's
work landed; the other's is already inside it.

What landed is ten gate legs at the Thornwall Crypt → Drowned Crypt Depths
gate, and the honest half of the result is the part worth repeating: the
hard gate (*reach the font*) **passed once**, in leg r9 — three of five bots
into map 5 at 692.9 / 692.5 / 617.1 s, Gravemother sighted, clock dead ten
seconds later. The deliverable, the boss verdict, is **not established**. No
leg ever engaged her. r14 restored r9's exact combat posture and did not
reproduce the reach, which is the measurement that matters: the entry was
timing and luck, not posture. The sustainable blocker is content-side — the
map-3 respawn-swarm attrition wall (racks respawning on a 30 s cycle, the
cocoon widows, the barrow ring converging on the depths stairs). Bot posture
has been stable since r14; doubling the budget to 1800 s in r17 changed
nothing.

That is a *finding*, not a failure, and it is filed where the next session
can act on it: `docs/handover/T-118-r17.md` §5 lists four untried
content-side fixes and the r18 experiment (restore r9's map-1 posture —
individual return march, cap-cross at q≥2 — while keeping r16b's election
and guarded re-adoption). §4 is the do-not-retry list. The M3 exit criterion
the board recorded as "MET" in T-118 was the earlier, pre-seeded staging
leg; this series is the unassisted measurement, and it says the gate is
reachable but not repeatably. Both numbers are now on master, and they are
not in conflict — one was a proof of killability, the other a measurement of
the march.

## The wave itself

No renumber was needed — the first clean wave in three. Master was still at
`818d661`, PR #25 was MERGEABLE/CLEAN with both CI matrices green, and its
14 commits sat directly on that base. The diff touches only `tools/`,
`docs/` and `logs/`: **no server, shared, client, engine or test code**, so
there was no sim-semantics question to settle. Journal epoch stays **21**,
schema stays **v11**, wire stays **237**.

Post-merge verification on `2a813ec` (full local battery, headless preset):

| check | result |
|---|---|
| build (`ninja -C build/verify bh_bots bh_server bh_tests bh_duel`) | 74/74, clean |
| `bh_tests` | **209/209 · 329,052 assertions · 0 failed** |
| `ctest` | 2/2 |
| `bh_duel --selftest` | `win=1 ticks=41 hpEnd=82 hash=b273be661b54673a` — pin holds |
| `logs/m3_gate.bwj` replay | `ticks=36042 sessionCmds=5933 hashes=360 mismatches=0 entities=189` |
| `logs/t120.bwj` replay | `ticks=1500 sessionCmds=529 hashes=15 mismatches=0` |
| epoch guard | refuses `t118.bwj` (journal 20 vs build 21), exit 4 — by contract |

The journal of record replays bit-exact on master at epoch 21 — the guard did
not refuse it, because the series never moved the epoch. The r17 numbers in
the replay line are the same numbers the PR body claimed, which is the point
of re-running it rather than trusting the branch.

## Environment taxes, third occurrence

Devlog 0081 recorded two of these; this wave hit all three plus two new ones,
so they belong in one place:

- **The sandbox came back shallow.** `git log master` showed a single commit
  and `7b2e9b8` was unreachable. `git fetch --unshallow` fixes it.
- **No cmake, no ninja.** `pip install --user --break-system-packages cmake
  ninja` — the PEP 668 flag is mandatory on this image.
- **`apt-get` is blocked** (deb.debian.org unreachable), so the X11/GL dev
  bits `tools/bootstrap.sh` fetches with `apt-get download` cannot be had and
  the client cannot build. The battery therefore ran under the repo's own
  **headless preset** (`-DBH_BUILD_CLIENT=OFF`), which by the T-108 gate
  excludes the three client-law TUs (`test_zoom`, `test_animstate`,
  `test_clientlaw` — 7 `TEST_CASE`s between them). Those seven are covered by
  CI's client-ON matrix, which is green. The 209/329,052 figures quoted above
  are the headless numbers and match the board's pin exactly.
- **`bh_maps` is an `ALL` target, and building only the four battery targets
  skips it.** Without `assets/maps/*.bhmap` (gitignored, data-derived) four
  tests fail at `loadZone` and replay dies on `cannot open:
  assets/maps/thornwall.bhmap`. `ninja -C build/verify bh_maps` — not a code
  fault, and it looks exactly like one.
- **There is no `bh_replay` binary.** Replay is
  `bh_server --replay-world <journal>`, which is what `tools/m3_gate_leg.sh`
  calls. Worth writing down because the name is what everyone reaches for.

## Queue state after the wave

T-122 (pledge-lite) and T-123 (Weeping Castle, stacked on #22) are in flight
as PRs **#22** and **#23** and are claimed; this wave therefore took
**T-124**. Two collisions are now waiting for whoever merges those, and both
are the documented rule — master keeps its numbers, the incoming card
renumbers:

- **Devlog numbers.** #22 carries `docs/devlog/0084-pledge-lite.md` and #23
  carries `0085-weeping-castle.md`; master now holds `0084` (the gate series)
  and `0085` (this entry). Per the collision rule those two take whatever is
  free at their own merge time — 0086/0087 if nothing else lands first.
- **A real content conflict.** Both PRs edit `tools/bots/main.cpp`, which #25
  rewrote (+1009/−46). `git merge-tree` against the new master reports
  `CONFLICT (content): tools/bots/main.cpp` for both heads (`c0de5634`,
  `e0718d7d`). They need a sync before they can merge — and #23 is stacked on
  #22, so #22 goes first.

Next card **T-125**, next devlog **0086**, journal epoch **21**. The queue's
most valuable open question is still the Gravemother: the wall is content,
and content is ours to move.
