# Task board

Card format: Context / Scope / Acceptance criteria / Tests required / Out of scope.
One card = one agent session = one PR. Move to `done/` with evidence pasted.

## Done — Sprint 1 (2026-09-04)

| Card | Title |
|---|---|
| [T-001](done/T-001.md) | Repo + CMake + presets + CI + warnings discipline |
| [T-002](done/T-002.md) | Window/input + fixed 20 Hz tick stepper |
| [T-003](done/T-003.md) | Isometric renderer (2:1 diamonds, prisms, painter sort) |
| [T-004](done/T-004.md) | Atlas/anim loader + procedural placeholder hero |
| [T-005](done/T-005.md) | Tiled JSON -> .bhmap converter + validator |
| [T-006](done/T-006.md) | Sim grid + deterministic RNG + A* |

## Done — Sprint 2 (2026-09-04, accelerated)

| Card | Title |
|---|---|
| [T-007](done/T-007.md) | Camera rig: follow/free-pan/zoom-at-cursor, bounds clamp |
| [T-008](done/T-008.md) | Day/night: real game clock (4h day) driving tint |
| [T-009](done/T-009.md) | Replay journal v0 + hash oracle (record/replay/verify) |
| [T-010](done/T-010.md) | Thornwall pass 2 (generator): graveyard, stalls, mud, grass variation |
| [T-011](done/T-011.md) | ADR-001..008 formal write-ups in docs/adr/ |
| [T-012](done/T-012.md) | Demo automation: tick-latched script, deterministic captures |

## Done — Sprints 3-4 (2026-09-04, accelerated): Phase 1 Netcore, M1 gate

| Card | Title |
|---|---|
| [T-013](done/T-013.md) | ENet transport + protocol v0 (messages.md -> generated serializers) |
| [T-014](done/T-014.md) | Login stub + account/char persistence (SQLite v1, ADR-0009 stub auth) |
| [T-015](done/T-015.md) | Zone server: tick loop + entity store + spatial hash |
| [T-016](done/T-016.md) | Authoritative movement sync + AoI deltas + client interpolation |
| [T-017](done/T-017.md) | Chat (say/global/system) + spawn-diff reconnect view |
| [T-018](done/T-018.md) | tools/bots v1: 20 headless wanderers + 10x60s soak gate (M1) |

## Done — Sprint 6 (2026-09-04, accelerated): economy & death (Phase 2)

| Card | Title |
|---|---|
| [T-021](done/T-021.md) | Inventory/equip/loot: item table, gear in combat math, mob drops |
| [T-025](done/T-025.md) | Death: XP debt 10->25% of bar, de-level at 0 XP |
| [T-026](done/T-026.md) | Skills v1: Power Swing + Soma weapon-skill spine |
| [T-027](done/T-027.md) | Vendor v0: Marta, 5-stock shop, junk pawn, proximity trades |
| [T-028](done/T-028.md) | Bots v1.5: vial economy + power swings |

## Done — Sprint 7 (2026-09-04): trade, density, balancer, replay (Phase 2 finish)

| Card | Title |
|---|---|
| [T-029](done/T-029.md) | Trade window: commit-time validation, autosafe rollback (ADR-0011) |
| [T-030](done/T-030.md) | Fields content to L11 (7 mob kinds, 10 spawners), density rule |
| [T-031](done/T-031.md) | Bots v2 grinder economy + server TTK balancer feed |
| [T-032](done/T-032.md) | World journal record/replay; wipe reproduces bit-exact |

## Open — M2 gate pass checklist (de-QA)

| Card | Title | Notes |
|---|---|---|
| T-033 (human slice) | Trade-pass UX review: trade window / `/repair` / anvil feel | **human-only**; soak telemetry attached in devlog 0031. The programmable half (14-bot grinder soak, bands, wipe replay) re-PASSed 2026-09-08 — see `done/T-033.md` |

## Done — Sprint 8 (2026-09-04): zones-in-process (Phase 3 launch)

| Card | Title |
|---|---|
| [T-035](done/T-035.md) | Thornwall Crypt map: gen+conv, L12 Revenant Sexton, sibling portal |
| [T-036](done/T-036.md) | Zones-in-process: N zones in World, portals, per-zone AoI/sim |
| [T-037](done/T-037.md) | Client zone handoff via Welcome reuse; map hot-swap |
| [T-039](done/T-039.md) | persist schema v4: characters.map_id zone persistence |

## Done — Sprint 9 (2026-09-05): anvil/aura spine (RFC 0001)

| Card | Title |
|---|---|
| [T-041](done/T-041.md) | Widow Anvil furniture + tryAnvil gate chain |
| [T-042](done/T-042.md) | Aura tiers I–II live effects (+atk, regen) |
| [T-043](done/T-043.md) | Protocol v31→131, inv blob v5, karma v6, bless journalling |
| [T-045](done/T-045.md) | Pilgrim bot profile (rite economy + anvilTries telemetry) |
| [T-046](done/T-046.md) | Karma columns + moral split (+15% XP / +15% gold) |
| [T-038](done/T-038.md) | RFC 0001: anvil/aura spine (implemented by T-041..T-046) |
| [T-040](done/T-040.md) | bot deaths probe: CombatEvent-driven (carried into S8.5 gate) |

## Done — Sprint 10 (2026-09-05): proc riddle + panel (aura spine complete)

| Card | Title |
|---|---|
| [T-047](done/T-047.md) | wireKind furniture floor assert + aura procs III–V (cleave/sunder/graft) |
| [T-048](done/T-048.md) | client anvil panel, karma readout, aura rows, kind-6/7 floaters |

## Done — Sprint 11 (2026-09-05): replay determinism (M2b unblocked)

| Card | Title |
|---|---|
| [T-049](done/T-049.md) | record-path determinism: shared applyWorldCommand; replay 3×300s clean |

## Done — Sprint 12 (2026-09-05): duel lab + campaign gate evidence

| Card | Title |
|---|---|
| [T-034](done/T-034.md) | bh_duel: gear-aware offline duel harness + era table |
| M2b (partial) | wipe-replay ACROSS chains bit-exact (12 legs, 1139 hashes); L1->8 campaign measured, plateau=L6, stays Phase-3 xparty |

## Done — Sprint 13 (2026-09-05): party system (Phase 3 spine)

| Card | Title |
|---|---|
| [T-050](done/T-050.md) | party core: wire v237, rules, slash verbs, despawn sweep, replay-invariant |
| [T-051](done/T-051.md) | party XP share: 12-tile radius, +12%/sharer, loot stays with killer |
| [T-052](done/T-052.md) | client party frame (roster panel, leader star, live hp bars) |
| latent fix | command-path WorldEvents drained pre-tick (anvil-floater-class bug) |

## Done — Sprint 14 (2026-09-05): class kits, Cultist-first

| Card | Title |
|---|---|
| [T-053](done/T-053.md) | class kits foundation: classId, persist v7, /kit oath, skill dispatch, MP pool |
| [T-054](done/T-054.md) | Cultist kit v1 (Mend/Bless/Ironskin) + Gravecaller Firebolt starter |
| [T-055](done/T-055.md) | race-proofed choir bot + dual-kit gate leg (mixed verdict, levers listed) |

## Done — Sprint 15 (2026-09-05): alignment & PK teeth

| Card | Title |
|---|---|
| [T-056](done/T-056.md) | PK law + chaos penalties: karma −(300+20d), whitening, drops, gallows, refusals, duels |
| [T-057](done/T-057.md) | alignment chrome: karmaBand wire, red nameplates, sheet stakes |
| correction | T-046 moral-split gate raised to GDD lawful >500 (xor-drift fix) |
| tooling | journal epoch marker `v N`; replay refuses mismatched epochs (exit 4) |

Execution prompt for the whole remainder queue: docs/prompts/phase3-remainder.md

## Done — Sprint 16 (2026-09-05): anvil gear-churn

| Card | Title |
|---|---|
| [T-058](done/T-058.md) | Durability: burn on swing/hit, dormancy at 0, `/repair` at Marta (persist v8) |
| [T-059](done/T-059.md) | Affixes v1: Whet / Warding / Leech rolled at gear-drop time (persist v9, journal epoch 4) |
| [T-060](done/T-060.md) | Refine at the Widow Anvil: parts+50g toll, 0→1/1→2 sure, 2→3 60% or SHATTER (persist v10) |

Gate: suite 83/83 (327,466 assertions); epoch-4 smoke replay OK
(ticks=1455 hashes=14 mismatches=0; bots mend=14). Devlog 0017.

## Done — Sprint 17 (2026-09-05): day/night stakes

| Card | Title |
|---|---|
| [T-061](done/T-061.md) | Nightcreep: mobs ×1.15 dmg & +1 aggro 21:00–05:00, paired-world bites pinned |
| [T-062](done/T-062.md) | Night economy: +10% XP / +25% relative drops after dark; overlay alpha floor 150 documented |

Gate: suite 87/87 (327,479 assertions); S17 smoke replay OK
(ticks=1292 hashes=12 mismatches=0; journal epoch stays 4 — zero new rng
draws). Devlog 0018.

## Done — Sprint 18 (2026-09-05): content drop

| Card | Title |
|---|---|
| [T-063](done/T-063.md) | Bonehowl Mine (mapId 4) + Drowned Crypt Depths (mapId 5), portals + validators |
| [T-064](done/T-064.md) | Gravemother L14 (xp×20) + Sepulcher Elites (xp×8, 8 camps) + Blood Bolt (+25% night pin) |
| [T-065](done/T-065.md) | Wanted Board (wire 66): session-scoped quarry cycle, payout over natural band |

Gate: suite 91/91 (327,815 assertions); S18 smoke replay OK
(ticks=1255 hashes=12 mismatches=0; epoch 5 — zones enter worldHash,
entities 291→579). Devlog 0019. **M2b-final L1→8 chain now running.**

## Done — Sprint 19 (2026-09-05): VFX + audio pass 1

| Card | Title |
|---|---|
| [T-066](done/T-066.md) | Callout sweep: bless gold / ironskin steel / party shimmer + petrify-fade vestiges |
| [T-067](done/T-067.md) | Procedural synth kit (7 voices, ~78 KB, BH_NO_AUDIO-safe) |

Gate: suite 91/91 (327,815 assertions); S19 smoke replay OK
(ticks=1182 hashes=11 mismatches=0; epoch 5). Devlog 0020.

**Phase-3 queue drained.**

## Done — Sprint 20 (2026-09-06): replay trust

| Card | Title |
|---|---|
| [T-049](done/T-049.md) | record-path determinism CLOSED: 3× cadence-25 6-bot soaks clean, burst-semantics pins, ctest working-dir fix |

Gate: 93/93 suite; t49_repro verdict fail=0; ctest 100%. Devlog 0021.
M2b-final chain resumed (leg-4+) after the sandbox-eviction restart.

## Done — Sprint 21 (2026-09-06): kit v2 content half

| Card | Title |
|---|---|
| [T-054b](done/T-054b.md) | Chorus (ch6, L9) / Mass Mend (ch7, L12) / Haste (ch8, L10-11) — party law, cadence, gates pinned |

Gate: suite 98/98 (327,852 assertions); ctest 2/2; epoch stays 5. Devlog 0022.
Deferred on purpose: choir-bot v2 profile + T-034b retune → after the
M2b-final chain closes (binary-swap would poison pacing evidence).

## Closed — T-049x (2026-09-06): the relog launderer

The M2b-final chain closed; its persist-round replay leg exposed a launderer:
live login and replay applyLogin parsed 7-field inventory blobs differently
(equipped dropped / dormant gear resurrected / affix-refine lost / slot order
scrambled), below `worldHash`'s id/zone/pos/hp coverage. One shared grammar
(`parseInvBlob`) + canonical logout save + `BH_DUMP_ENTS` fingerprint probes
+ `tools/bh_probe_leg.sh`. Suite 105/105; all journals replay 0 mismatches.
Devlog 0023. Epoch stays 5.

## Done — Sprint 22 (2026-09-06): choir-bot v2 + T-034b retune

| Card | Title |
|---|---|
| [T-054b](done/T-054b.md) | Cultist v2: Chorus (party-wide), Mass Mend, Haste; choir-bot v2 (potion priority, safe-chase, ch6/7/8 usage) |
| [T-034b](done/T-034b.md) | balance retune from duel table: widow dmg 24→22, widow leash 12→10, gnoll dmg 18→17 |

Gate: suite 105/105 (327,925 assertions); ctest 2/2; journal epoch 5→6
(content shifts sim under v5 journals; replay refuses by contract); fresh
smoke leg `tools/s22_smoke.sh` replay 0 mismatches; fresh duel table
`logs/duel-table-s22.csv`. Devlog 0024.

## Done — T-034c (2026-09-06): L4-L5 gear-band investigation

| Card | Title |
|---|---|
| [T-034c](done/T-034c.md) | L4-L5 gear-band investigation: the wall is a death-tax treadmill in the hound band — blade-only L4 = 7/9 @30% hp, and the campaign profile has no re-gear trip (shopping coupled to death-respawn) |

Finding: 24–48 deaths/leg × ~190 XP debt ≈ 7.6k XP wiped, ~break-even vs
~11–14k kill XP at L5 — the plateau. Ranked levers: (1) campaign re-gear
trip, (2) Hide Armor 120→80, (3) hound dmg 13→12, (4) L4-L5 death-tax
relief. Devlog 0026.

## Done — T-034d (2026-09-06): campaign re-gear trip

| Card | Title |
|---|---|
| [T-034d](done/T-034d.md) | Campaign re-gear trip: walk home when gold clears the next gear tier (blade @260g, then armor @120g), then back out |

Gate: smoke `tools/t034d_smoke.sh` (2 bots × 240 s) — `regear=2`, replay
0 mismatches. Devlog 0027.

## Done — T-034d chain rerun (2026-09-06): plateau moves L4-L5 → L6

Full `tools/m2b_rerun_chain.sh` (12 legs, fresh DB, 2 campaign bots, ~10.8k
ticks/leg). Every journal replays **0 mismatches**; `TARGET L8` never fires.

| leg | peak | end | deaths | kills |
|---|---|---|---|---|
| 1 | L3 | L3 | 58 | 81 |
| 2 | L5 | L5 | 14 | 126 |
| 3 | L6 | L6 | 40 | 139 |
| 4 | L6 | L5 | 76 | 113 |
| 5 | L6 | L3 | 128 | 88 |
| 6 | L3 | L1 | 182 | 45 |
| 7 | L4 | L4 | 56 | 125 |
| 8 | L4 | L4 | 48 | 126 |
| 9 | L4 | L3 | 68 | 97 |
| 10 | L5 | L4 | 22 | 108 |
| 11 | L5 | L5 | 16 | 112 |
| 12 | L6 | L6 | 32 | 143 |

**Verdict: the plateau moved past L5.** Baseline (T-034c) never touched L6 in
12 legs; this run reaches L6 four times and holds it in legs 3 and 12 (deaths
58→14 across legs 1→2). But L6 is a new wall — the over-level gnoll/widow
triangle — not the armor band. Devlog 0028.

## Done — Route v5c (2026-09-07): the wall was the waypoint, not the mobs

Closes the "L6 over-level wall" card. Devlog 0028's hypothesis (over-level
gnoll/widow triangle) was **wrong**: the death diagnostics showed `killerByLvl`
was L3-dominated (Feral Ghoul) and `lastDeath` clustered on `(55,18)/(55,20)` —
the centre of the `ghouls_east` rect (x[52,58] y[17,22], maxAlive 8) that Route
v4 parked every L3+ bot on. The wall was **pack density at the waypoint**, plus
a threat-axis retreat that walked bots past the L11 gravecaller barricade.

Fix (`tools/bots/main.cpp` only — no content/price/mob numbers):
edge-stand the ladder `(55,19)→(55,14)`, level-blind pack cap for L3+,
retreat **home** on map 1 (sticky until healed *and* unpursued), defend at
point-blank while disengaging, swarm panic-break at 5.

val4 (resumed DB at L5, 540 s, same harness/seed as val2): **maxLevel 5 → 7**,
deaths **66 → 10**, bot 00 deaths **32 → 0** (L5→L6 t=47 s, L7 t=531 s,
`killerByLvl` empty), replay 0 mismatches. Full 12-leg chain (fresh DB): 12/12
replay 0 mismatches, mean end level **4.08 → 4.58**, worst close **L2** vs the
baseline's L1, legs ending L5+ **5/12 → 8/12**, deaths 740 → 662; ceiling stays
L6 (legs 11-12). Suite 105/105, ctest 2/2, journal epoch unchanged at 6.
Devlog 0029; analysis prompt
`docs/prompts/campaign-pack-wall-analysis.md`. Commit `a2dcc6b`.

| Card | Title |
|---|---|
| [Route v5c](devlog/0029-wall-was-the-waypoint.md) | Campaign pack wall: edge-stand waypoint, level-blind pack cap, retreat home, defend-while-fleeing |

## Done — T-068 (2026-09-07): the L8 content gate opens

Closes the "L8 content gate" card. Director picked **Option A — move the L11
Gravecaller barricade off the bridge approach** (over B "add a crossing" /
C "re-anchor the gnolls"). `gravecaller_barricade` rect `(6,18)→(1,43)` in
`tools/mapgen/make_thornwall.py` (+ map regen): placement derived from the
threat model (worst in-rect anchor + wander 5 + night aggro 8) so every bot
route tile stays ≥ 8 from mob reach, day and night; the L11 stays as a
far-marsh hazard. Journal epoch **6 → 7** (content shifts the sim under v6
journals; stale by contract). Campaign bot opens the L7+ waypoint: gnolls
north edge `(53,41)`.

val5b (chain DB, chars set L7 in town, 720 s): **`TARGET L8 DONE in 444.3s`**
— peak **L8** / end L7, deaths **6** (zero L11; killers were road bats and
the pre-existing hound bridge gauntlet), replay **0 mismatches** (epoch 7,
`ticks=15601 hashes=157`). Suite 105/105 (327,925 assertions), ctest 2/2.
val5 diagnostic (DB resumed parked at the perch): the known bad-leg death
loop (156 deaths, L3 ghouls) — harness must set state via sqlite; recorded
in devlog 0030.

| Card | Title |
|---|---|
| [T-068](done/T-068.md) | L8 content gate: move the gravecaller barricade off the bridge approach (+ L7 waypoint, epoch 7) |

## Done — overnight S21 (2026-09-09): Smugglers' fence

T-056's named gap closed: Sable the Fence (wireKind 69) seeds at the gallows
pit, pawns junk at 60% for any band, sells the 3-item secret stock to chaotic
eyes at +25%. One-Marta fix (425-duplicate spiral break restored; soak entity
count 544–584 → ~330, p99 12.9–16.4 → 3.8–5.6 ms). Epoch **7 → 8**; fresh
600 s grinder-mix journal replays
`[replay] OK ticks=14001 sessionCmds=5984 hashes=141 mismatches=0`.
Suite 112/112 (327,969 assertions). Devlog 0032.

| Card | Title |
|---|---|
| [T-069](done/T-069.md) | Fence at the Smugglers' Cove (+ one-Marta fix, epoch 8) |

## Done — overnight S22a (2026-09-09): Blood Curse + chapel cure

Debuff loop closes: boss Blood Bolt hits curse 30 s (600 ticks); potions
and Mend land at 75% while cursed; OOC regen untouched; `/confess` at the
chapel Confessor (wireKind 68, thornwall chapel rect) clears. Premise
corrections flagged: Gravecaller carries no bolt kit (source = shared
boss-bolt path, i.e. Gravemother); Bless unscaled (no heal component);
vfx-14 already HASTE (no new combat kind); OwnStats +1 trailing u16, version
stays 237 (S16 precedent, stated). Epoch **8 → 9**; fresh 580 s journal
replays `[replay] OK ticks=13601 sessionCmds=6093 hashes=137
mismatches=0`. Suite 119/119 (328,008 assertions). Devlog 0033.

| Card | Title |
|---|---|
| [T-070](done/T-070.md) | Blood Curse + chapel cure (epoch 9) |

## Done — overnight S22b (2026-09-09): night light

Torch (3003, 8g) burns 6 tiles / 300 s; Blessed Lantern (3004, 150g)
toggles 8 forever while held; both ride the journaled `kUseItem` lane (no
new command). Warm mask at night, floor-safe by construction (peak 90 <
150 floor). `nightOnly` spawner field + `.bhmap` v2 + mapconv prop;
`night_ghouls` (36,26, maxAlive 4) hunts 21:00–05:00 only. Build fix:
`bh_maps` syncs all five maps (zones 2–5 were stale v1). Judgment calls
flagged: lantern price/placement, Marta F1–F7 / fence F8–F10. Epoch
**9 → 10**; fresh 560 s journal replays `[replay] OK ticks=13201
sessionCmds=5633 hashes=133 mismatches=0`. Suite 126/126 (328,054).
Devlog 0034.

| Card | Title |
|---|---|
| [T-071](done/T-071.md) | Night light: torch + lantern + night-only spawns (epoch 10) |

## Done — overnight S23 (2026-09-09): aura tiers III–V audit

Audit-only (brief premise corrected — T-047 already shipped III–V):
gates 20/50/80/120/150 hold, effects match GDD §3, replay parity
spot-checked on the epoch-10 journal (`mismatches=0`). No code change,
no epoch bump (stays **10**). Devlog 0035.

## Done — overnight S24 (2026-09-09): gate guards + spawn protection

Gate Guard 1011 (L15 wall, xp 0, guard flag) posts at the east gate and
bridge approach; unlawful PK ≤8 of an anchor marks wanted 240 s (guards
acquire in leash 12, vendors refuse, death binds at gallows). Spawn
protection 100 ticks on spawn + respawn (lookup skips; retaliation still
fires). Judgment calls flagged: post sizes, no-free-hits, fence open to
wanted, expiry stand-down. Epoch **10 → 11** (S23 didn't bump — chain
shifts one, stated). Fresh 540 s journal replays `[replay] OK ticks=12801
sessionCmds=7321 hashes=129 mismatches=0`. Suite 134/134 (328,092).
Devlog 0036.

| Card | Title |
|---|---|
| [T-073](done/T-073.md) | Gate guards + spawn-camp protection (epoch 11) |

## Done — overnight S25 (2026-09-09): bad-leg levers, measured + reverted

Bench pinned (rerun legs 3/5/7: 142/134/116 deaths, ALL L3 ghouls at the
ghouls_east/orchard edges). One lever moved: pack-cap level gate dropped
(poverty-trap hypothesis). A/B (L2-parked pair, 540 s): BEFORE 32 deaths /
L5 → AFTER **104 deaths / L3** — lever rejected and reverted to v5c,
nothing shipped. Bot-only, epoch stays **11**. Next pit noted (retreat
5→4, repeats, L1-naked). Devlog 0037.

| Card | Title |
|---|---|
| [T-074](done/T-074.md) | Bad-leg death levers: pack-gate drop measured, rejected, reverted |

## Done — extended S26 (2026-09-09): client quick wins

T-ART-01 (point filter in `loadAtlas`), T-ART-02 (wheel snap {1,1.5,2} +
`test_zoom.cpp` boundary pins), T-ART-08 (map cases 4/5). Render-only, no
epoch impact (stays **11**). Ledger: T-ART-03 done-superseded by T-071,
T-ART-06 partial (68/69 shipped), T-ART-09/11 parked. Suite 135/135
(328,102). Devlog 0038.

## Done — extended S27 (2026-09-09): anim-state hook

T-ART-04: render-side combat anim states (attack/cast/hurt/die) keyed by
`CombatEvent` pulses, contact frame first, walk/idle fallback until B3+
lands. Durations half-cadence derived (8/8/4t, die held). Render-only, no
epoch impact (stays **11**). Suite 137/137 (328,115). Devlog 0039.

## Done — extended S28 (2026-09-09): client atlas batch

T-ART-05 (`atlasFor`: mob sheets resolve, hero fallback, 1011 noted
sheetless), T-ART-07 (overhead tint red > party > lawful; enemy-town a
documented no-op), T-ART-10 (`anchorY` wired end to end). Render-only, no
epoch impact (stays **11**). Suite 140/140 (328,140). Devlog 0040.

## Done — extended S29 (2026-09-09): karma repentance

T-075: `/repent` (journaled `kRepent`) at the confessor ≤3, 72000t
cooldown, +20 karma via `bumpKarma`, wanted refused, curse untouched.
Amount/cooldown derive from whitening pins (flagged). No epoch bump
(stays **11**); fresh 540 s journal replays `[replay] OK ticks=12801
sessionCmds=7483 hashes=129 mismatches=0`. Suite 147/147 (328,167).
Devlog 0041.

## Done — extended S30 (2026-09-09): levers round 2, red again

T-077: retreat 5→4 in the L2-park regime — AFTER 92 deaths / L3 vs BEFORE
32 / L5. Rejected and reverted (3-line comment kept on the restored line,
flagged). Bot-only, epoch stays **11**. Repeats + L1-naked stay parked.
Devlog 0042.

## Done — extended S31 (2026-09-09): guard-murder consequences

T-078: player kills 1011 → GDD-§5 stain vs L15 (−580 at L1, −299 net at
L15 with the blind whitening tick) + shared-path wanted. Mob kills
exempt; no factions/marks. No epoch bump (stays **11**); fresh 540 s
journal replays `[replay] OK ticks=12801 sessionCmds=6929 hashes=129
mismatches=0`. Suite 150/150 (328,177). Devlog 0043.

## Done — extended S32 (2026-09-09): refine +4 to +7

T-079: GDD rates 65/50/35/25 above frozen shipped rows; failure slips one
temper (+7 bid resets to +0); shatter stays refine-2-only. No wire change;
+5 glow stays T-ART-11's (now unblocked). **Epoch 11 → 12**; fresh 540 s
journal replays `[replay] OK ticks=12801 sessionCmds=7984 hashes=129
mismatches=0`. Suite 152/152 (328,447). New watch: wander-death drift
(24→146 across the shift, shape says roam-RNG, recorded). Devlog 0044.

## Done — extended S33 (2026-09-09): trade transaction log

T-080: one canonical audit line per executed swap (lower-id leads),
nothing on cancel/oversell; tmp-path test seam; pre-existing swap test
repointed + `logs/trades.log` git-ignored (suite-hygiene fix found live).
No epoch bump (stays **12**); fresh 540 s journal replays `[replay] OK
ticks=12801 sessionCmds=7389 hashes=129 mismatches=0`. Suite 154/154
(328,457). Devlog 0045.

## Done — extended S34 (2026-09-09): durability on death

T-081: GDD-literal −5 on all surviving gear (floor 0, dormant never
destroyed); junk/consumables exempt; drops-then-wear order. No epoch bump
(stays **12**); fresh 540 s journal replays `[replay] OK ticks=12801
sessionCmds=7536 hashes=129 mismatches=0` (Ghoul TTK back at the 8.9 pin —
wobble confirmed noise). Suite 156/156 (328,471). Devlog 0046.

## Open — Phase 3 remaining

| Card | Title | Notes |
|---|---|---|
| L8→L9 step-up | `widow_glade` (L9) shares the south bank with the gnoll camp; no L8+ waypoint opened; pulled widows survivable but untuned | follow-up only if the director wants the ladder pushed past L8 |
| Bot bad-leg deaths | v5c legs 3/5 (142/134 deaths) and the val5 diagnostic show the perch death-loop mode persists on some starts | levers: pack cap / retreat threshold; reproductions: leg 3/5 logs, val5 |
