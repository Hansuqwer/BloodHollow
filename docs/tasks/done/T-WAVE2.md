# T-WAVE2-EPOCH30 — Wave-2 batch (SHIPPED 2026-09-16, epoch 29→30, wire 242→244, schema v15)

Proposal: `T-WAVE2-EPOCH30.md` (open file, superseded by this record — briefs
T-153/160/161/162/163/166/167 live in git history + PR #53). One stacked branch,
ONE epoch bump, one leg. Recommendations from the proposal all held:
T-160(B) five-stat, T-167 1-char-alpha, T-153 own session (separate branch).

## Shipped per card

- **T-167 char-create (1-char alpha):** fresh Hello → LoginResult ok + `CharCreatePrompt(120)` (no spawn); client parchment panel (1/2/3 + M/F + Enter) → `CharCreate(25)` → validated (class 1..3, sex 1..2) → `setCreation` + spawn with class/sex; `/kit` window law unchanged (covers fallback). Legacy rows: class kept, sex defaults 0→hero fallback (documented, no silent re-kit). Bots answer deterministically (name-hash kit/sex). Live proof: 8/8 leg bots created (kits 1/2/3, both sexes).
- **T-160 five-stat (ADR-0015):** INT/MAG assignable (stat 3/4, F8/F9 with vendor guard — also fixes latent F5-7/vendor F-key overlap); OwnStats already carried intg/mag (client now unpacks + shows); GDD §3 struck CHA; stale `messages.md` line fixed.
- **T-161 Resurrect (ch10):** `chUnlock` 10→12 (content-only widening, no wire/DB — draft caution cleared), Cultist ch10=20; `tryResurrect` = post-mortem debt rebate ≤6000 ticks (NOT corpse-raise: 3 s respawn makes it unusable — recorded deviation), 25 MP, 6-tile same-zone, 6000-tick CD (init -7000 so fresh cultists answer), one rebate per death; `trySkill` dispatches to 10; client KEY_SIX @selected + help line. Sanctuary/Curse/Raise + control sets → **T-161b** (filed).
- **T-162 roster:** D9 rename 1007→Waxen Celebrant; 11 rows (1015–1025: Wretch/Spider/Golem/Revenant/Banshee/Ringer + Pale Cultist + night Wraith/Bloodfiend + patrols); `nightSpawned` flag + GDD-§9 +50% at night (day-stragglers pay base); spawners placed in 4 mapgens (rects probed walkable, validate 0/6); elite pins updated (fields 8/mine 13/tcrypt 8/dcrypt 13). Art: quarantine note — new kinds fall back to placeholder (no crash), sheets queued, no gens spent.
- **T-163 field-war:** `MobDef.town` (+patrol rows 1024/1025); sworn-vs-enemy-patrol EK (karma-clean, T-130 law); `/ek` player twin of `gm ek`; GDD §1/§5/§12 amended (oath/EK/moon-lite back from the cut list). Gap recorded: patrol red-name tint needs a client pass.
- **T-166 bounty:** mark = one-kill drain (no progress counter exists — simpler than drafted); v15 columns persist it; board text states restart-survival; GDD §12 pickup-N cut.
- **Schema v15** (one migration): sex, last_death_tick, last_debt_xp, last_res_tick (-7000), bounty_mob, bounty_cycle. T-153 takes v16. **Journal:** `y`-sidecar (sex/stamps/bounty; absent pre-30 → exact Entity defaults); **hash** +lastDeathTick/lastDebtXp/lastResTick/nightSpawned (sex display-only, out).

## Evidence

- Headless 325/325 (11 new wave-2 cases: creation/fresh/domain, INT-MAG, ch10 unlock, rebate + 6 refusals, rename/rows, premium 40/44/66/40, EK 3-way, bounty round-trip + pay-once). ctest 2/2, validate 0/6.
- `logs/wave2.bwj` (8 fighters ×60 s + 8 relog ×10 s): replay ticks=1500 cmds=819 hashes=15 **mm=0**; epoch-29 guard exit 4. CI replays wave2 (t159 retired as leg).
- Client changes (creation panel, stat rows, KEY_SIX, F8/F9, help) compile-checked by review only here — CI graphical matrix owns the compile gate (flagged in PR).

## Deviations recorded

1. Resurrect = rebate, not corpse-raise (3 s respawn; GDD §3 noted).
2. Bell Ringer = elite caster, summoning deferred (T-161b).
3. Magic/Rare = 1 affix stands (T-159f1 owns multi-affix).
4. Patrol red-name tint deferred (client pass).
5. Live Resurrect proof deferred (unit pins + doctest; needs L20 cultist staging).
6. y-sidecar proven via relog wave (replay mm=0 across it), not a dedicated parse test.
7. Diff >400 lines (single atomic wave; per-card commits impossible — world.cpp spans 4 cards).
