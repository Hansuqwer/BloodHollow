# Next-session prompt — 10-task recommended todolist, round 2 (2026-09-09)

*Follows `docs/prompts/session-handoff-1to10-2026-09-09.md` (HEAD `7f512c5`, suite 167/167, 328,566 assertions, epoch 12, wire 237, replay `logs/t084.bwj` OK). The 1–10 run closed every old watch (wander-drift, TTK, anvil, graze) and shipped T-ART-09/11 + T-ART-06 engine sides. Every item below is new work stemming from that run. One card = one sprint = one devlog = one commit (<~400 lines).*

## Ground state (verified 2026-09-09)

- HEAD `7f512c5` in sync with `origin/master`. Leave another workstream's tree alone: `M tools/atlaspack/bh_mob_sheet.py`, `?? docs/prompts/arena-parallel-eval-brief.md`, `?? docs/prompts/art-b1-b8-execution.md`, `?? tools/atlaspack/b5_build.sh` — never stage.
- Build warning-free (`-Wall -Wextra -Werror`), `ctest --test-dir build` 2/2, `./build/tests/bh_tests` 167/167.
- Git identity: `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`; always `git pull --rebase` before push.
- Standard validation per task: fresh DB `/tmp/tNN.db`, server `--soak-secs 640 --record-world logs/tNN.bwj`, 14-bot grinder mix (wander×5/fighter×4/pilgrim×3/campaign×2, `--secs 540`), then `--replay-world` must read `mismatches=0`. Long runs detached: `(setsid ./build/<thing> ... > logs/<run>.log 2>&1 < /dev/null &)` then poll. 30 s per-command timeout.
- Epoch law: bump `kJournalEpoch` only on content-sim change. Wire law: trailing field appends don't bump 237 (S16/T-070/T-071 precedent). GDD-vs-shipped → shipped wins + fix the GDD line.
- Determinism: `sim/rng.h` only; 20 Hz; integers in ticks/tiles in sim+server; C++20, no exceptions/RTTI across boundaries, hand-format touched lines (no whole-file `clang-format` — HEAD is not clean under v22.1.8).
- Val pattern (resumed-DB): copy chain DB + sqlite-set state (level/xp/map/pos) — never resume as-is (false death-loops); `/tmp/*.db` is reboot-volatile. XP bars: `xpNext(L)` in `shared/sim/combat.h` (L7→8 = 3660, L8→9 = 4690).
- Bot A/B discipline (T-074): same start state, same regime, one lever, before/after tables (deaths, killerByLvl, lastDeath, maxLevel, mend). Revert-on-red.

## How to use this list in opencode

Load these 10 items into the opencode todolist. Work top-down. Do NOT batch unrelated code changes into one commit. After each item: card in `docs/tasks/done/`, board update, GDD update if numbers changed, devlog, replay evidence, push.

---

### 1. L8→L9 climb verdict (HIGH)
- **Context:** T-084 opened the widow camp (60,41) but the 240 s val held L8 (86 kills / 0 deaths) — the climb 8→9 is unproven. This is the ladder's next gate.
- **Scope:** 540 s climb leg: `/tmp/t083b.db` → `/tmp/t090.db`, campaign pair sqlite-set L8/xp0/town (32,16), gear+gold kept, campaign-only `--target-level 9`. Bot-only (no map/mob change) → no epoch bump.
- **Acceptance:** Verdict table (peak/end, deaths, killerByLvl, lastDeath, mend) + L9-reached yes/no. If yes, the ladder is open to L9; if no, name the wall (widow stats? pack density? debt?).
- **Tests:** Val + replay `mismatches=0`; suite green.
- **Out of scope:** Widow/gnoll stat tuning (that's #6, gated on this verdict).

### 2. Gravemother telegraph attack (HIGH — first decal-API caller)
- **Context:** T-ART-09 shipped `addTelegraph`/`addCircle` with no callers; the backlog names boss telegraphs (1009) as the use case. GDD boss: Gravemother L14, 3 phases, bell adds, ground-rot telegraphs.
- **Scope:** ONE telegraphed slam for Gravemother: stage 1→2→3 visuals (20/20/40t per decal law) then deterministic damage in the circle. Derive numbers from the shared boss-bolt path where possible (devlog 0033 premise discipline); anything invented gets flagged for director review.
- **Acceptance:** Telegraph shows 60t before damage lands; damage deterministic + replay-clean; kill-20-style decal count sane.
- **Tests:** Unit pins (telegraph→damage timing, gates) + soak + replay. Epoch bump ONLY if sim content shifts (cite the rule either way).
- **Out of scope:** Phases 2–3, bell adds, Sanctuary/Wither circles (separate cards).

### 3. In-world refine overlays via trailing wire field (MEDIUM)
- **Context:** T-ART-11 shipped inventory-only glow because no entity snapshot carries refine (deliberate non-wire scope). This card finishes the job.
- **Scope:** Append ONE trailing `u8` (equipped-weapon glow tier, 0/1/2) to the entity spawn/delta path — trailing append, no 237 bump (S16/T-070/T-071 precedent, fleet deploys lockstep); client draws the sprite overlay (alpha ≤ 90 law) for tier ≥ 1. Deterministic read-only field → no epoch bump (cite rule).
- **Acceptance:** +5 blade glows in-world day/night at 1×/2×; +10 silhouette marker; replay bit-exact.
- **Tests:** Codec round-trip pin + glow-tier pin; replay of touched journal.
- **Out of scope:** New messages (would move 237 — not this card); overlay sprite frames (reuse glow wash until art ships).

### 4. Tier-1 toll retune options (MEDIUM — price, do NOT build)
- **Context:** T-086 proved the Tier-1 toll (30 pelts / skill 20 / 120g) exceeds single-leg income ~6×. Retune is economy design — price it like T-076, don't take it.
- **Scope:** Devlog pricing A (lower pelts e.g. 30→10, keep skill/gold) vs B (keep toll, declare the rite multi-leg progression + design the banking instead) with epoch/soak costs for each. No code, no tests, no bump.
- **Acceptance:** Director can pick A/B from the paper alone; each option lists exact diff + test pins + soak plan.
- **Tests:** None (decision paper).
- **Out of scope:** Implementing either option.

### 5. Thornwall NPC placement 67/70/71 (MEDIUM — needs art-side positions)
- **Context:** T-ART-06 engine side shipped (constants + seams + generic render). The twins + two guards belong in Thornwall; registrar (72) is Marrowgate and steward (73) is Weeping Castle — neither map exists, so they stay unplaced.
- **Scope:** Get art-side tile positions for 67/70/71, add furniture spawns via mapgen (generator truth, regen `.tmj`, byte-identical others), verify spawn-sweep in Thornwall + karma lanes untouched.
- **Acceptance:** All three idle in Thornwall on a fresh boot; client generic rect+label renders them; refusals still work.
- **Tests:** Spawn-sweep unit + boot log; replay line (epoch bump ONLY if furniture shifts the sim — cite rule).
- **Out of scope:** 72/73 placement (no maps), sheets/portraits/dialogue (art-side cards).

### 6. Widow/gnoll tuning (MEDIUM — gated on #1, do NOT start early)
- **Context:** Pulled widows at the gnoll camp are "survivable but untuned" (devlog 0030). Tune only what #1 proves is a wall.
- **Scope:** AFTER #1 verdict: move exactly the named number (widow dmg/aggro/leash or gnoll overlap), one lever, before/after climb tables, revert-on-red (T-074 discipline).
- **Acceptance:** L8→L9 climb improves without breaking the L7 camp or the epoch-12 journals more than stated.
- **Tests:** A/B climb legs + replay; epoch bump only if content-sim shifts.
- **Out of scope:** Starting before #1 lands; stacking levers.

### 7. Multi-leg rite demo (MEDIUM — own DB discipline)
- **Context:** T-086 rejected fresh-DB harness for proving the rite end-to-end ("needs its own card and its own DB discipline"). This is that card.
- **Scope:** Persistent-DB demo (NOT fresh per leg): farm → bank pelts across 2–3 legs on one DB (document the banking: which DB, sqlite state between legs, same `--prefix` resume), attempt Tier-1 at the anvil, record gates hit.
- **Acceptance:** Either a graft lands (anvilTries ≥ 1 with server accept) or the binding gate is named with numbers per leg. Reusable DB recipe documented for future progression demos.
- **Tests:** Per-leg journals + replays; suite green.
- **Out of scope:** Changing the toll (that's #4); generalizing to all tiers.

### 8. Named-elite options paper (LOW — price, do NOT build)
- **Context:** Named elites need stat/timer design (Old Maw / Red Widow / Cantor Vex, GDD §9: 15–60 min timers, world-announced first-kill). Staff drafts options; director designs.
- **Scope:** Devlog paper: spawn-timer/locations/stat-shape options per elite derived from nearest shipped rows (Sepulcher Elites ×8 / Gravemother ×20 precedents), announcement path (system chat, existing), costs per option. No code.
- **Acceptance:** Director can scope three elite cards from the paper.
- **Tests:** None.
- **Out of scope:** Implementing any elite.

### 9. Decal pressure probe (LOW — read-only)
- **Context:** T-ART-09 chose a single 512-cap FIFO surface over per-chunk render targets "until profiling demands". This probe checks demand.
- **Scope:** Instrument (log-count, no sim change — or desk-count from kill pulses in t083/t084 journals): peak live decals per leg, eviction rate, frame-cost estimate of the y-sort. No code change to the surface.
- **Acceptance:** Table (peak/mean live decals, evictions/leg) + verdict: cap holds or chunking card opens.
- **Tests:** None (read-only).
- **Out of scope:** Implementing chunking.

### 10. Trade-pass human review packet (LOW — assemble, don't perform)
- **Context:** T-089 confirmed the programmable half is green; the remaining work is a human at a keyboard (T-033 human slice). Staff's job: make that session frictionless.
- **Scope:** Assemble the packet: latest soak telemetry table, trade/fence vendor flows, `/repair`/anvil feel checklist, screenshot/GIF shot list (trade window, repair, anvil tries), known human-only items (trade-pass UX feel). No code.
- **Acceptance:** A director can run the review from the packet alone; packet committed under `docs/tasks/done/` with the card.
- **Tests:** None (human session owns verification).
- **Out of scope:** Performing the review; changing trade UX.

---

## Verify on entry (repeat before first task)

```
cmake --build build -j"$(nproc)"
ctest --test-dir build
./build/tests/bh_tests
./build/server/bh_server --replay-world logs/t084.bwj
git log --oneline -3
```
