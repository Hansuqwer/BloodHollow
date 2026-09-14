# Continue — M3 gate follow-on #2: confirm the r21 mode, break the x=15 wall, attempt the boss verdict (issued 2026-09-14)

This prompt is the execution order for the next M3-gate session. It starts
from the standing law (`AGENTS.md`, `docs/prompts/continue-building-2026-09-12.md`,
`docs/prompts/continue-pr-pipeline-2026-09-12.md`) and the latest handover
(`docs/handover/T-125-r18-r22.md`). Where they conflict, the board
(`docs/tasks/README.md`) wins.

---

## 1. Repo analysis — where the tree stands

**Ground truth (verified 2026-09-14 in the execution sandbox, not copied from
the prompt):**

- HEAD: `2845a3d` — "T-124 + T-125: PR #25 merge wave, then the M3 gate
  follow-on r18-r22 (font reach 4/5 at 232 s; boss verdict still open)".
- Journal epoch **21** · wire **237** (`shared/protocol/gen/messages_gen.h:14`)
  · persist schema **v11** · duel pin **`b273be661b54673a`** (win=1 ticks=41
  hpEnd=82) · suite **209/209 · 329,058 assertions** (headless preset; the 3
  client-law TUs are CI-only when X11 is unavailable) · `logs/t120.bwj` replay
  `ticks=1500 hashes=15 mismatches=0` · r17 journal of record
  `logs/m3_gate.bwj` md5 `b4c0cf3f3255e38071c172cc3a3336bd`.
- M3 gate state (T-125, six legs r18→r22b, all 900 s, all replay bit-exact):
  - **The font reach is repeatable and fast** — r19 3/5 (147 s), r20 4/5 (268 s),
    **r21 4/5 at 231.7–237.9 s with 3–5 elite + 12–14 trash kills per bot** —
    where the r9–r17 series produced one reach in ten legs. r21 is the
    best-known configuration; **its n=1 mode is the first thing to confirm.**
  - **The boss verdict is STILL NOT ESTABLISHED.** No leg has ever killed the
    Gravemother (mob 1009: L14, hp 700, dmg 40, aggro 8, leash 14, boss
    channel 26 s). `bossSeen` is routine (20–26/leg); kills zero.
  - The wall moved from map 3 to **map 5 x=15**: the party clears (6,21),
    holds (13,10), and dies between (13,10) and (22,10) — inside overlapping
    aggro fields of the apse elites (mob 1010 "Sepulcher Elite" L12 hp 380
    dmg 36 aggro 8, rects (16,4,4,3) and (25,4,4,3)), the Revenant Sexton
    (mob 1008 L12 hp 420 dmg 34 aggro 8, rect (21,5,3,3)) and, at the x=21
    boundary, the Gravemother (rect (21,2,4,3)).
  - r22 (kiter band on map 5) was **attempted, never exercised, reverted** —
    not tested; do not retry it as an independent lever (re-land only behind
    a reaching leg, front-runner guard intact).
- Untried lever named by the handover (§5), in order: **(1) re-run r21** (n=1
  today); **(2) break the x=15 triple-aggro — option (a): an extra node at
  (19,10)**; (3) then the boss verdict with the assembled configuration.
- Next counter: **card T-126 · devlog 0087** (per the post-T-125 amendment in
  the PR-pipeline prompt §5). T-122/T-123 were claimed in flight by PRs
  #22/#23 — do not reuse.
- Level question (depths are L18–25 content; the gate party is L13 by
  top-up design) **stays a director call**. This session does not raise the
  level and says so in the PR if the verdict depends on it.

## 2. The work — T-126: M3 gate follow-on #2

Scope is **bot/tooling + logs + docs only** (`tools/bots/main.cpp` route
table, `logs/`, `docs/`). No sim/server/protocol/schema/epoch change. The
r21 route on map 5 (verified against the shipped `assets/maps/drowned_crypt.bhmap`
v2 binary, all 1497 bytes parsed: ground/zone/blocked/spawners/portals) is
(6,21) → (13,10) → (22,10) → (22,3); the party enters map 5 at (3,30) via the
map-3 portal (44,6,2,1)→(3,30) and the return portal is (2,30,1,2)→(44,6)
map 3.

**r23 — confirm the mode.** Re-run the r21 configuration UNCHANGED (same
binary, same 900 s, same staged DB). Expectation: 4/5 reach in the 230–240 s
band. A 3/5 or worse result is itself the finding — re-run once more (r23b)
before touching anything else, and record the leg variance per handover §4
(one leg cannot promote or retire a lever).

**r24 — the (19,10) node.** Add a fifth map-5 node between (13,10) and
(22,10): **(19,10)** — verified walkable in the shipped .bhmap (blocked=0,
ground=1), on the y=10 corridor, 5 Chebyshev from the Sexton rect. Rest 0
(waypoint: the existing map-5 quorum hold — 3+ living within 6 tiles, 45 s
cap — already applies, so the column assembles there before pushing the last
7 tiles). Intent: the apse elite (16,4) is killed as a stacked fight at the
node instead of while the column is strung out mid-march at x=15. Hand-format
the route-table line(s); do NOT clang-format the file.

**Then the battery and the verdict.**

- Full battery after the code change: suite, duel pin, `logs/t120.bwj`
  replay mm=0 (epoch-guard refusals of t107/t104/t112 are correct
  behavior).
- Both new legs replay bit-exact (`bh_server --replay-world`); soak p99
  within the 10 ms budget.
- Archive: force-add `logs/m3_gate_r23.bwj` (+ `r23b` if run) and
  `logs/m3_gate_r24.bwj`; **restore `logs/m3_gate.bwj` byte-identical**
  (the harness overwrites it; md5 must read
  `b4c0cf3f3255e38071c172cc3a3336bd` at close-out).
- **Boss verdict:** if r24 (or its follow leg) reaches the font and the
  party engages, record kill / TTK / unkillable explicitly with per-bot
  numbers from the `[raid]` lines. If the party still dies before the font,
  the verdict stays "not established" — say exactly where (x, y, killer
  composition) and which lever is left. Do not report a direction the legs
  did not produce.

## 3. Execution procedure

1. **Step 0 — sync and claim** (per the PR-pipeline prompt): `git fetch
   origin`, confirm master position and open PRs, confirm T-126/0087 are
   free on the board. (In this sandbox: work stays on the Arena session
   branch; the PR goes from the session branch to `master` — note that in
   the PR body instead of the usual `task/T-NNN-slug` branch.)
2. **Bootstrap** (fresh sandbox): `pip install --user
   --break-system-packages cmake ninja`; configure headless into
   `build/verify` (`-G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
   -DBH_BUILD_CLIENT=OFF`); `cmake --build build/verify -j$(nproc)`;
   `ninja -C build/verify bh_maps` before any test/replay.
3. **Rebuild the staged DB** (it dies with the sandbox): short warm leg to
   create the five `m3g_` characters, stop the server cleanly (save race —
   wait for the DB to show final levels), then `tools/m3_topup.sh`
   (idempotent: L13/0xp/600g, mixed-kit class_id 1/3/2/3/1, stage (1,22)
   map 3, 16-vial belt, Pit Blade + Hide Armor, 7/11/6).
4. **r23 leg** (`tools/m3_gate_leg.sh <port> 900`, port 7701): record the
   `[raid]` lines, deaths, map-5 entries, reach times, bossSeen, p99.
   Archive the journal, restore `m3_gate.bwj`.
5. **Code change** (the (19,10) node) → rebuild → quick battery sanity
   (duel pin + one test) → **r24 leg** (port 7702) → archive + restore.
6. **Battery** (full) → **docs**: card `T-126-m3-gate-followon-2.md`
   written first, ticked with evidence at close; devlog `0087-...`; board
   row at the done-junction (interleave, never drop); handover
   `T-126-*.md` if the verdict is still open (the next session starts from
   the handover, not from scratch).
7. **Commit** (identity `bloodhollow-dev <dev@bloodhollow.local>`),
   **push**, **open the PR** to `master` with the standard body: what/why,
   full evidence (suite line, duel pin, replay lines, per-leg table, soak
   p99, epoch disposition = *unchanged 21*), deviations, merge-order notes.
   **Leave it open — the director merges.**

## 4. Law (unchanged, restated for the executor)

- C++20, no exceptions across module boundaries, `-Wall -Wextra -Werror`
  clean; hand-format the lines touched.
- No new third-party dependencies. No sim-semantics change ⇒ no epoch bump;
  if anything in this session touches sim semantics, **stop and flag** —
  the epoch/leg discipline is director-owned.
- The server never trusts the client; all verification evidence comes from
  the server replay, never the client.
- Diffs < ~400 lines; bots-only means `tools/` — a `server/` file in the
  diff is a stop-and-flag.
- Never touch `assets/final/`; journals are force-added; the r17 journal of
  record stays byte-identical.
- Budget two legs per change before a verdict (handover §4); say which leg
  you ran.
- Push early, push often (sandbox restarts wipe `.local/` and `build/` and
  can roll `.git` back); the journal of record is the recovery anchor.

*Filed 2026-09-14. Execute T-126, run the legs, record honestly, open the
PR and leave it open.*
