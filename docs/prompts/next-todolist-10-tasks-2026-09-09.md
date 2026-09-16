# Next-session prompt — 10-task recommended todolist (2026-09-09)

*Follows `docs/prompts/session-handoff-continuation-2026-09-09.md` (HEAD `814af0b`, suite 160/160, 328,489 assertions, epoch 12, wire 237, replay `logs/t082.bwj` OK). No open task cards in `docs/tasks/` — every item below needs a director go-ahead or a new card. One card = one sprint = one devlog = one commit (<~400 lines).*

## Ground state (verified 2026-09-09)

- HEAD `814af0b` in sync with `origin/master`. Leave another workstream's tree alone: `M tools/atlaspack/bh_mob_sheet.py`, `?? docs/prompts/arena-parallel-eval-brief.md`, `?? docs/prompts/art-b1-b8-execution.md`, `?? tools/atlaspack/b5_build.sh` — never stage.
- Build warning-free (`-Wall -Wextra -Werror`), `ctest --test-dir build` 2/2, `./build/tests/bh_tests` 160/160.
- Git identity: `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`; always `git pull --rebase` before push.
- Standard validation per task: fresh DB `/tmp/tNN.db`, server `--soak-secs 640 --record-world logs/tNN.bwj`, 14-bot grinder mix (wander×5/fighter×4/pilgrim×3/campaign×2, `--secs 540`), then `--replay-world` must read `mismatches=0`. Long runs detached: `(setsid ./build/<thing> ... > logs/<run>.log 2>&1 < /dev/null &)` then poll. 30 s per-command timeout.
- Epoch law: bump `kJournalEpoch` only on content-sim change; stale journals refuse by contract. Wire law: trailing appends don't bump 237 (S16 precedent). GDD-vs-shipped conflict → shipped wins + fix the GDD line.
- Determinism: `sim/rng.h` only; 20 Hz; integers in ticks/tiles in sim+server; C++20, no exceptions/RTTI across boundaries, `clang-format` before commit.

## How to use this list in opencode

Load these 10 items into the opencode todolist. Work top-down. Do NOT batch unrelated code changes into one commit. After each item: card in `docs/tasks/done/`, board update, GDD update if numbers changed, devlog, replay evidence, push.

---

### 1. T-083 wander-drift investigation (HIGH)
- **Context:** Wander-death drift 24→146 across two shifts. Shape says roam-RNG but monotonic. Prime suspect: live-entity growth (330→378) widening aggro coverage. Handoff says revisit if it persists two more legs.
- **Scope:** 2+ fresh short-soak legs with entity-count/aggro-coverage correlation. Read-only telemetry first; no balance change without a follow-up card.
- **Acceptance:** Correlation table (entities vs deaths vs aggro events) + verdict: RNG noise or real leak. If real, propose lever as T-084.
- **Tests:** Soak + replay `mismatches=0`; paste death/killer breakdown.
- **Out of scope:** Changing pack caps, retreat thresholds, mob stats.

### 2. L8→L9 step-up (HIGH)
- **Context:** Oldest open decision. `widow_glade` (L9) shares south bank with gnoll camp; no L8+ waypoint opened. Options in devlog 0030.
- **Scope:** Director picks option first, then: waypoint + threat-model clearance (same method as T-068: worst anchor + wander 5 + night aggro 8), map regen if needed.
- **Acceptance:** Chain-DB val at L8 reaches L9 without L11 / bridge-gauntlet contamination; replay clean. Epoch bump only if content-sim changes.
- **Tests:** Full val leg + replay; suite green.
- **Out of scope:** Re-tuning widow/gnoll stats unless director orders.

### 3. Base-light verdict A/B (HIGH)
- **Context:** GDD §9 base-6 vs shipped 0. T-076 priced both, recommends B. Devlog 0048.
- **Scope:** Implement director's pick only: A = GDD-literal base 6, torch duration-only, epoch bump; B = keep shipped 0 + one-line GDD fix, no code.
- **Acceptance:** Night overlay floor still holds (peak < 150 floor); torch/lantern behavior matches chosen spec; GDD line fixed.
- **Tests:** Night screenshot + replay; suite green.
- **Out of scope:** New light-mask layer (that's T-ART-03 phase 2).

### 4. T-ART-11 refine glow composite (MEDIUM)
- **Context:** Now unblocked by T-079 (+4–+7 shipped). GDD §7 "item glows from +5". Spec in `docs/tasks/art-backlog.md` + `docs/art/40-items-icons.md`.
- **Scope:** Additive glow overlay at refine ≥5 (alpha ≤90, ~2 frames), +10 silhouette change. Render-only, replay-invariant.
- **Acceptance:** +5 blade glows in-hand + inventory day/night at 1×/2×; +10 reads as silhouette, not brighter glow.
- **Tests:** Anvil-refine to +5/+10 screenshot set; replay round-trip unchanged.
- **Out of scope:** Sim/wire/epoch changes.

### 5. T-ART-09 ground decal layer (MEDIUM)
- **Context:** Blood decals (10 min), boss telegraphs, Sanctuary/Wither circles need a render-to-texture layer (`docs/03-architecture.md` gore row). Needs art-side input on assets first.
- **Scope:** Decal surface under entities with decay timers; API for blood / 3-stage telegraph / persistent circles. Rendering-only, non-wire.
- **Acceptance:** Kill-20 leaves countable decals decaying correctly; telegraphs 3-stage; replay bit-exact.
- **Tests:** Kill-20 count/decay test + replay unchanged.
- **Out of scope:** Gameplay effects from decals.

### 6. Pilgrim anvilTries=0 probe (MEDIUM, small)
- **Context:** Standing telemetry anomaly, never scheduled. Bot-only.
- **Scope:** Trace why pilgrim profile never attempts anvil; fix harness or profile, no economy change.
- **Acceptance:** `anvilTries` non-zero with reason, or documented proof the gate chain correctly refuses.
- **Tests:** Pilgrim-only soak snippet + replay.
- **Out of scope:** Anvil rates/prices.

### 7. Mid-band TTK wobble closure (MEDIUM)
- **Context:** Ghoul touched 8.9 pin twice; confirmed noise, not closed. Owned by T-074 discipline.
- **Scope:** Fresh comparison legs with fixed seeds; close or re-pin with confidence interval.
- **Acceptance:** Updated pin or closed ticket with evidence table.
- **Tests:** Duel-table rerun (`bh_duel`) + soak cross-check.
- **Out of scope:** Balance changes.

### 8. T-ART-06 remainder: NPC kinds 67/70–73 (LOW)
- **Context:** 68 confessor + 69 fence shipped. Remaining: 67 bonesmith_twins, 70 guard_ashen, 71 guard_synod, 72 registrar, 73 steward. Needs design/portraits.
- **Scope:** Constants + spawner support + client furniture draw path (idle 4f×8 dirs).
- **Acceptance:** All kinds spawn + idle in Thornwall/Marrowgate sweep; karma refusals still work.
- **Tests:** Spawn sweep test.
- **Out of scope:** Dialogue content, EK ledger (blocked on Marrowgate).

### 9. Bank-road corner night-graze (LOW)
- **Context:** Known minor pathing graze at night, unchanged across shifts.
- **Scope:** Waypoint nudge or blocker rect adjustment, map regen if needed.
- **Acceptance:** Before/after graze count on night leg; no new wall.
- **Tests:** Night soak snippet + replay.
- **Out of scope:** Mob AI changes.

### 10. Karma/price tuning pass (LOW, needs director pins)
- **Context:** Derivations awaiting human feel: repent +20/72000t, lantern 150g, T-033 trade-pass UX (human-only).
- **Scope:** Only after director pins numbers: apply + GDD update + soak.
- **Acceptance:** Soak shows intended effect without economy break; GDD matches shipped.
- **Tests:** Targeted soak + replay.
- **Out of scope:** Implementing without pins — do NOT tune unasked.

---

## Verify on entry (repeat before first task)

```
cmake --build build -j"$(nproc)"
ctest --test-dir build
./build/tests/bh_tests
./build/server/bh_server --replay-world logs/t082.bwj
git log --oneline -3
```
