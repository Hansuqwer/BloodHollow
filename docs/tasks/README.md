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

## Done — extended S35 (2026-09-09): Cultist Purify

T-082: channel 9 (gate, kit table ×3, unlock guard), Cultist unlock 6,
Mend-mirror gates, cleanse-only effect. No client key (6–8 precedent),
no epoch bump (stays **12**); fresh 540 s journal replays `[replay] OK
ticks=12801 sessionCmds=7391 hashes=129 mismatches=0`. Suite 160/160
(328,489). Devlog 0047.

## Done — extended S36 (2026-09-09): base-light director option

T-076 DECISION (no code): GDD §9 base-6 vs shipped 0 priced as A
(GDD-literal, torch duration-only, epoch bump) vs B (keep 0, one-line GDD
fix). Non-binding recommendation: B pending playtest evidence. Devlog 0048.

## Done — T-083 (2026-09-09): wander-drift investigation

T-083 READ-ONLY: two short-soak legs (240 s bots / 300 s soak, wander 16 + 24 deaths, entities 349/344, replays 0 mismatches). Drift 146→16→24 broken — roam-RNG noise confirmed, no leak, no T-084. Devlog 0049.

## Done — T-084 (2026-09-09): L8→L9 step-up camp

T-084 BOT-ONLY: widow north-edge camp (60,41) for L8+ in `tools/bots/main.cpp`. Val: L8 pair 86 kills / 0 deaths, replay 0 mismatches. L9 climb verdict parked (needs full leg). Devlog 0050.

## Done — T-085 (2026-09-09): base-light verdict B

T-085 DOCS-ONLY: GDD §9 base-6 → base 0 (T-076/B executed). Zero code, epoch stays 12, t082 replay unchanged. Devlog 0051.

## Done — T-ART-11 (2026-09-09): refine +5 glow

T-ART-11 RENDER-ONLY: glow law header + inventory glow rows/markers, 2 new tests (162 total). In-world overlays parked (need wire field). No epoch/wire change. Devlog 0052.

## Done — T-ART-09 (2026-09-09): ground decal layer

T-ART-09 RENDER-ONLY: decal law header + client surface (blood/telegraph/circle), 3 new tests (165 total). Boss cast APIs await callers. No epoch/wire change. Devlog 0053.

## Done — T-086 (2026-09-09): pilgrim anvil probe

T-086 AUDIT-ONLY: anvilTries=0 is correct — Tier-1 toll (30 pelts/skill 20/120g) exceeds single-leg income ~6×. No code, retune flagged for director. Devlog 0054.

## Done — T-087 (2026-09-09): TTK wobble closure

T-087 DETERMINISM-PROOF: duel table twice-identical, Ghoul row untouched since Sprint 18, soak wobble = regime noise. Re-pinned (9.7 s duel-med / 8–25 s soak band), watch CLOSED. Devlog 0055.

## Done — T-ART-06 remainder (2026-09-09): NPC kinds 67/70–73

T-ART-06 ENGINE SIDE: 5 constants + spawn seams + sweep test (167 total). Client generic branch already renders them. Placement/sheets/portraits parked for art-side. No wire/epoch change. Devlog 0056.

## Done — T-088 (2026-09-09): bank-road graze audit

T-088 AUDIT-ONLY: 8-leg geography, zero on-route L11, campaign clean 8/8 — era-correct wall content, no fix. Rect lever priced-not-taken (marginal math + epoch cost). Devlog 0057.

## Done — T-089 (2026-09-09): tuning validation

T-089 VALIDATION-ONLY: repent +20/72000t, lantern 150g, torch 8g re-derived + checked across 8 legs — all hold, zero changed. Trade-pass programmable half green, human slice unblocked. Devlog 0058.

## Done — T-090 (2026-09-09): L8→L9 climb verdict

T-090 BOT-ONLY: 540 s climb from 0 xp — 156 kills / 2 roam deaths, 76%+57% of bar, zero widow/L11. Verdict: ladder OPEN, pace-limited (logistics tax), no wall — #6 tuning NOT triggered. Devlog 0059.

## Done — T-091 (2026-09-09): Gravemother telegraphed slam

T-091 CONTENT (epoch 12→13): 60t wind-up + radius-2 slam replaces her instant bolt; movers dodge, mark-loss fizzles. Client stages 1-2-3 + floaters. 4 new tests (171 total). Fresh soak + replay clean, old journals refuse exit 4. Devlog 0060.

## Done — T-092 (2026-09-09): in-world refine glow

T-092 WIRE (stays 237): trailing `glowTier` on spawn/delta + scan + halo draw. Codec + scan tests (172 total). No epoch bump (t091 replays clean). Devlog 0061.

## Done — T-093 (2026-09-09): Tier-1 toll options

T-093 DECISION (no code): retune A (pelts 30→10, epoch bump, still skill-bound) vs B (keep toll, multi-leg progression, one docs edit). Non-binding recommendation: B. Devlog 0062.

## Done — T-094 (2026-09-09): Thornwall NPC posts + one-anvil fix

T-094 CONTENT (epoch 13→14): twins + post guards placed live (derived positions, flagged); found + fixed 63-anvils-per-zone spawn bug (T-069 class). Sweep test (173 total). Fresh soak + replay. Devlog 0063.

## Done — T-095 (2026-09-09): widow/gnoll tuning gate

T-095 GATE-CLOSED (no code): zero widow kills suffered in T-090 + t094 (14 bots) — no wall, no lever. Reopen rule framed; faster-L9 work belongs to logistics, not mob stats. Devlog 0064.

## Done — T-096 (2026-09-09): multi-leg rite demo

T-096 DB-DISCIPLINE: 2 natural legs bank 8→18 pelts (same DB+prefix, replays green) + bless-seeded leg proves the lane 3/3 grafts. Finding: skill resets per login (no schema column). Devlog 0065.

## Done — T-097 (2026-09-09): named-elite options

T-097 DESIGN (no code): three elites derived from the 1010 pattern (A melee / B Cantor-caster), fixed timers (determinism), broadcast first-kills. One card per elite when ordered. Devlog 0066.

## Done — T-098 (2026-09-09): decal pressure probe

T-098 READ-ONLY: peak 210 live decals vs 512 cap (2.4× headroom, 0 evictions, 0 telegraph/circle callers) — chunking stays closed. Reopen rule framed. Devlog 0067.

## Done — T-099 (2026-09-09): trade-pass review packet

T-099 ASSEMBLE (no code): one-page human session (trade/repair/anvil checklist + shot list) with programmable evidence pointers. Human performance + T-033 close remain director-scheduled. Devlog 0068.

## Done — T-100 (2026-09-09): L8→L9 climb repeat

T-100 BOT-ONLY: same-start repeat — 149 kills / 0 deaths, 82%+81% bar, hurt 270 déjà vu. Confirms T-090 (n=2): open ladder, potion economy, no wall. Ladder question closed pending route/gear/bar changes. Devlog 0069.

## Done — T-101 (2026-09-09): Old Maw elite

T-101 CONTENT (epoch 14→15): row 1012 + fields pit + shared first-blood announce (reused by next two). 3 new tests (176 total). Map-1 soak = regression proof (bots never meet it, stated). Devlog 0070.

## Done — T-102 (2026-09-09): Red Widow elite

T-102 CONTENT (epoch 15→16): row 1013 + mine nest + shared announce reuse. S18 gate moved 7→8 with mapgen truth. 3 new tests (179 total). Devlog 0071.

## Done — T-103 (2026-09-09): Cantor Vex elite

T-103 CONTENT (epoch 16→17): row 1014 (Sexton + half-rate instant bolt, boss-flag audited) + crypt choir. GDD elite line fully shipped. 4 new tests (183 total). Devlog 0072.

## Done — T-104..T-110 (2026-09-11): hardening wave (code in tree)

Shipped under the external-review map (task cards historically unfiled —
truth-up): T-104 refine parse throw-free · T-105 trade unequipped grammar ·
T-106 kill-line deque snapshots · T-107 worldHash economy (epoch 18,
`logs/t107.bwj`) · T-108 headless client-law gate · T-109 login limiter ·
T-110 docs/AGENTS truth-up. See README §Status.

## Done — T-111 (2026-09-12): critical-review hotfixes

T-111 FIXES: cross-zone respawn spatial==walker (F1), tryAnvil weapon-slot
index through part erase (F2), parseInvBlob throw-free ints (F3), bounded
mob-seed retries (F4), killPlayer brace tidy (F5). 4 new tests
(`test_t111_review.cpp`). No epoch bump. Devlog 0073 · prompt
`docs/prompts/critical-code-review-2026-09-12.md`.
**Correction (T-112):** F4 changes boot spawn counts + RNG stream — the
"no epoch bump" claim failed local replay (t107: 81/81 mismatches);
epoch 19 + fresh gate leg landed as T-112 below.

## Done — T-112 (2026-09-12): T-111 epoch follow-up

T-112 EPOCH FIX: kJournalEpoch 18→19 for T-111's F4 bounded scatter (probe:
T-111−F4 replays t107 clean, F4 alone breaks it 81/81). 3 new spawn-law
pins (`test_t112_spawn_epoch.cpp`, suite 201/328,962). Fresh gate leg
`logs/t112.bwj` (20 bots × 10 s): replay mismatches=0, p99=1.51 ms.
F1/F2/F3/F5 replay-neutral. Lineage note: PR #9's epoch-19 collision —
second merger re-bumps to 20. Devlog 0074.

## Done — T-113 (2026-09-12): L9 logistics scoping (design, no code)

T-113 DESIGN: 30-trip tax is bot belt policy (4-deep, buys 2/visit) — server
already permits 16-deep stockpiling (no depletion, stack 16). Armor quantified
as margin (~1 hp/hit), not pace. T-114 scoped: bots v3 flask belt + t090/t100
repeat discipline, no server change/epoch. Economy levers reopen only on
gold-starved re-run. Devlog 0075.

## Done — T-114 (2026-09-12): bots v3 flask belt — logistics lane closed with data

T-114 BOT-ONLY: belt 4→16 buy-to-depth (240g gate, 90g floor). Template
bit-exact from t100.bwj; 2×540s legs at epoch 18, replays mm=0. Stops
24–30 → 8/5 (criterion PASS, 3–5× cut) but bar 73.5%/55% avg (criterion
FAIL — pace unchanged, hurt rose): the camp's respawn+TTK throughput, not
vial flow, owns L9 pace. Logistics lane CLOSED; faster L9 = content-pacing
question (director call, nothing auto-triggers). No epoch bump (bots-only).
Devlog 0076 · card `done/T-114.md` · supersedes the open-row text landing
with PR #12 (T-113).

## Done — T-115 (2026-09-12): lineage reconciliation (epoch 20)

T-115 MERGE: task/T-112-t111-epoch-followup into task/T-104-review-fixes.
Duplicate fixes deduped (respawn home_ T-104#5≈F1; throw-free blob parse
T-104#6≈F3 — both parents' tests pass under survivors); tryAnvil union
(destroy-erase on F2's wslotIdx). kJournalEpoch 19→20 (double-19
resolved); fresh gate leg `logs/t115.bwj` (20 bots × 10 s, p99=1.27 ms,
replay mm=0). Suite 206/206 (329,022). Duel pin unchanged. PR includes
all of #8+#11's content — merge #9 first, then this, and #8/#11 close as
superseded. Devlog 0077 · card `done/T-115.md`.

## Done — T-116 (2026-09-12): merge wave — review era on master, queue empty

T-116 EXECUTION: director-authorized sequence per prompt
`docs/prompts/merge-wave-2026-09-12.md` (wave-scoped; no precedent).
Squash-merged #12 (T-113), #9 (T-104 wave + vendored sqlite), #15
(replacement for #13, closed by a raw base-branch delete — retarget
children first, delete last), #10 (B5 art; three-way deduped #9's
content, tree verified). #14 merged as a TRUE merge commit after board
sync (T-113/T-114 interleaved between T-112/T-115). #8 auto-closed
MERGED via ancestry reachability; #11 closed superseded. Master 0355367
at epoch 20: offline build, 206/206 (329,022), ctest 2/2, duel pin
`b273be661b54673a`, `t115.bwj` mm=0, epoch guard refuses t107(18) /
t104(19) / t112(19), CI green ×4 both matrices. Devlog 0078 · card
`done/T-116.md`.

## Done — T-117 (2026-09-12): continuation prompt + session handover (docs-only)

T-117 FILING: standing orders for post-wave build sessions —
`docs/prompts/continue-building-2026-09-12.md` (state snapshot verified
against master c8d8087 at authoring time, the law, card→PR workflow,
verification battery with expected numbers, build queue: Track A M3
party-crypt gate · Track B Phase-4 spine · Track C debt, cheat sheet,
traps) + `docs/prompts/session-handoff-postwave-2026-09-12.md` (session
handover: ground state, wave record, open director decisions, watch
items). Docs-only: no code/wire/epoch (stays 20); next card T-118, next
devlog 0080. Squash-merged under `merge-filing-2026-09-12.md` (single-use
authorization, not precedent). Devlog 0079 · card `done/T-117.md`.
## Done — T-118 (2026-09-12): M3 party-crypt gate leg

T-118 BOT-ONLY: crypt profile (5 bots mixed kits Rav/Cult/Grave), party formation 100 tries/3 s, hold-in-town until 5, route 1:10,10→3:24,30, 3:44,6→5:3,30, boss camp 21,2. Pre-seed L12 via sqlite (Pit Blade/Hide Armor/16 vials). Legs: 300 s party 4 + 2 boss kills (killable proof), 300 s party 5 + bossSeen 22 curse 32 slam 56, 600 s party 5 + bossSeen 38 curse 50 slam 86, replay mm=0 epoch 20. Verdict: M3 exit met, TTK <15 s focused, no tuning. Suite 206/206, duel pin `b273be661b54673a`, t115 mm=0, t118 mm=0 (ticks 12042). Devlog 0080 · card `done/T-118.md` · `logs/t118.bwj` force-added.

## Done — T-119 (2026-09-12): PR #18 merge wave — M3 gate lands as T-118 + PR pipeline filed

T-119 WAVE: first cross-session counter collision resolved per
`docs/prompts/merge-pr18-2026-09-12.md` — PR #18 (another session's M3
party-crypt gate, CI-green, bots-only) was authored as T-117 from
`c8d8087`; PR #17 consumed T-117 on master in between. Sync merge
`e1a66da` renumbered the card to **T-118** (card/devlog/leg/script/board
row; `logs/t118.bwj` byte-identical rename, replay re-verified mm=0;
script paths made repo-relative — only deviation). Squash-merged `cca3f94`,
branch deleted. **Verdict on record: M3 exit MET** (party of 5 mixed kits
clears to Gravemother, 2 kills, TTK <15 s, no tuning) — Track A closed;
next card T-120 = weapon-skill persistence (Track B1). Filed the
**Continue prompt** `docs/prompts/continue-pr-pipeline-2026-09-12.md`
(card→PR loop, merge-wave protocol, collision rule, multi-session rules).
Epoch stays 20. Devlog 0081 · card `done/T-119.md`.


## Done — T-120 (2026-09-12): B1 weapon-skill persistence (schema v11, epoch 21)

T-120 PERSIST: `characters` gains `sword_skill` + `swing_lands` (additive-only, defaults 0, user_version 10→11). `CharacterRow` + `saveProgress` + `loginOrCreate` + `journalLogin` l-line v3 `l ... karma swordSkill swingLands inv` + replay parser v3→v2→v1 backward compat. `kJournalEpoch` 20→21 (progression persistence). Gate leg `tools/t120_weapon_skill_leg.sh` (5 bots 60s fighter gains skill 1 lands 29-35, same 5 bots 10s wander relog journal shows skill 1 lands 33, DB max 35). Replay: t115 epoch 20 vs 21 refused exit 4 (expected), t120 epoch 21 OK ticks 1500 cmds 529 hashes 15 mm=0. Suite 209/209 (329052), duel pin unchanged. Devlog 0082 · card `done/T-120.md` · `logs/t120.bwj` force-added.

## Done — T-121 (2026-09-12): PR #19 merge wave — weapon-skill persistence lands as T-120 (epoch 21)

T-121 WAVE: second counter collision resolved per
`docs/prompts/merge-pr19-2026-09-12.md` ("Execute.", single-use) — PR #19
authored as T-118 from `c8d8087`; master had taken T-118 (M3 gate) and
T-119 (wave evidence). Sync merge `ddad769` renumbered the card to
**T-120**, incl. the **binary leg collision** (their epoch-21 leg →
`logs/t120.bwj` byte-identical, master's M3 `t118.bwj` kept; replay
re-verified mm=0) and a stale pre-renumber M3 row copy dropped from their
board edit. **Full battery on the synced branch** (sim semantics moved):
suite 209/209 · 329,052, ctest 2/2, duel pin unchanged, `t120.bwj`
replay mm=0, epoch guard refuses t115/t118 (20 vs 21) by contract.
Squash-merged `7b2e9b8`, branch deleted. **The Sisyphus ladder is fixed**
(T-096): sword skill persists across relog — Track B1 DONE; next card
T-122 = pledge-lite (Track B2). Standing prompts amended (epoch 21,
schema v11, l-line v3 grammar). Devlog 0083 · card `done/T-121.md`.

## Done — T-118 series (2026-09-13): M3 gate, r8→r17 raider series (bot-side measurement)

T-118 BOT-ONLY (raider profile): ten gate legs (r9–r17) at the Thornwall
Crypt → Drowned Crypt Depths gate, five-bot mixed-kit party `m3g__00..04`,
every journal replayed bit-exact. Machinery: one-brain column,
direction-aware runner election (min-x on the west map-1 march) + dead-band
2, pile-node wait-immunity, swing-and-ride point-blank, guarded desync
re-adoption (stand-on-node ≤2, never node k−1); TEMP `[dbg]`/`fr/q/qT0`
trace scaffolding stripped at close-out. **Verdict: hard gate (reach the
font) PASSED once** — leg r9, 3/5 bots into map 5 (692.9/692.5/617.1 s),
boss_sight, clock expired before engagement. **Boss verdict NOT
established** — no leg engaged the Gravemother; r14 restored r9's posture
and did not reproduce the reach, so r9 is luck-dependent. Blocker is
content-side: the map-3 respawn-swarm attrition wall (racks' 30 s respawn +
cocoon widows + barrow-ring convergence); r17 at 1800 s budget = deaths
26/22/16/4/16, zero map-5 entries. Disclosed staging: `tools/m3_topup.sh`
seeds `class_id` (the in-game kit oath rejects L13) + inv/stat rows,
server-down only. Epoch stays 21 (bots/docs/logs only). Devlog 0084 · card
`done/T-118-m3-gate-raider-series.md` · handover `docs/handover/T-118-r17.md`
(§4 do-not-retry, §5 untried content-side fixes) · `logs/m3_gate.bwj`
force-added. *Row added by the T-124 wave — PR #25 shipped the card without
a board row.*

## Done — T-124 (2026-09-13): PR #25 merge wave — the gate series lands; #24 closed superseded

T-124 WAVE: director-directed merge (overriding never-self-merge for this
wave). PR #25 (`arena/01a09acb-bloodhollow`, `9163617`, 14 commits) verified
MERGEABLE/CLEAN + 4 green checks with master unmoved at `818d661`, then
squash-merged as **`2a813ec`** with the PR title as subject. PR #24 (r8
lineage, CONFLICTING) **closed unmerged** as superseded — its four r8
commits are cherry-picked into the merged series. Post-merge battery on
master: 209/209 · 329,052, ctest 2/2, duel pin `b273be661b54673a`,
`m3_gate.bwj` replay `ticks=36042 sessionCmds=5933 hashes=360 mismatches=0
entities=189` (epoch 21 = build 21, guard did not refuse), `t120.bwj` mm=0,
guard refuses `t118.bwj` (20 vs 21) exit 4. Epoch 21 / wire 237 / schema v11
untouched (diff is tools/docs/logs only). Sandbox taxes recorded: shallow
clone → `--unshallow`; no cmake/ninja → pip `--break-system-packages`; apt
blocked → headless preset (3 client-law TUs left to CI); `bh_maps` ALL-target
must be built or 4 `loadZone` tests fail and replay cannot open the maps;
replay is `bh_server --replay-world` (there is no `bh_replay`). **Queue
flags:** PRs #22/#23 (T-122/T-123) now CONFLICT in `tools/bots/main.cpp`
(merge-tree verified on both heads) and carry devlog 0084/0085, which master
has taken — they renumber on merge. Devlog 0085 · card `done/T-124.md` ·
next card **T-125**, next devlog **0086**.

## Done — T-125 (2026-09-13): M3 gate follow-on r18→r22 — the reach stops being luck

T-125 BOT-ONLY (raider profile; no server/sim change, epoch stays 21). Six
900 s legs (r18-r22b), every journal replayed bit-exact. **r18** restored r9's map-1
posture (individual return march + cap-cross q>=2, keeping r16b's
election/re-adoption): 0 map-5 entries, deaths 12/4/6/2/8, but **all five
reached map-3 x=38-39** (barrow corridor) vs r17's x=24. **r19** added
handover 5 fix #2 — **distance-aligned marks** (nearest-first; per-peer
last-seen hp is stale, distance is the only field a 3-tile-tight column
agrees on): **3/5 map-5 entries at 147.1/147.8/196.4 s** — ~4x faster than
r9's 617-693 s, `bossSeen=12 curse=2 slam=4 kills=0`. **r20** fixed a bug the
r19 forensics exposed: the Firebolt cast and kiter band were gated on
`kitClass == 3` (Cultist), but kits.h gives ch5 to the **Gravecaller** (L1)
and leaves the Cultist's ch5 at 0, so `World::trySkill` dropped every such
cast — **the whole r8->r19 series fought melee-only with a no-op "kiter"**.
Gates moved to `kitClass == 2` and the channel table pinned in
`tests/test_kits.cpp` (runtime-proven via `World::trySkill`): **4/5 map-5
entries at 268.4/268.6/269.4/272.1 s**, `bossSeen=20 curse=2 slam=0`, first
elite kills at this gate. **Boss verdict STILL NOT ESTABLISHED** — the party
arrives on map 5 at 84-100% hp and died crossing (3,30)->(22,3) at deepest
x=12, because map 5 had a **single** route node. **r21** staged that
crossing — **(6,21)->(13,10)->(22,10)->(22,3)** on the verified walkable
corridor — and it worked: **4/5 map-5 entries at 231.7/232.8/237.5/237.9 s**,
map-5 traces 24->122, **elites 3/5/5/5**, trash 2-4 -> 12-14, `bossSeen=26`.
The party now dies at **x=15** inside **three simultaneous aggro fields**
(apse elite (16,4) r8, Sexton (21,5) r8, Gravemother (21,2) r8) — the last 7
tiles are the whole fight. **r22** (kiter band on map 5, so the Gravecaller
trades bolts from 7-8, outside her bolt range 6) was **attempted, never
exercised by either of its two legs, and reverted**: r22 never entered map 5
(618 of 791 traces on map 1), r22b put two bots there for 1.3/2.3 s stuck at
node (6,21). The change is inert on maps 1/3 where both legs were lost, so
that is **leg variance, not the lever** — and an unexercised `continue`
branch on map 5 is the r8c stall risk inside the best-known configuration.
**Leg variance is large** (same binary: 0/5 then 2/5; r21 4/5) — one leg
cannot retire a lever, and one cannot promote one. Next: re-run r21, then
break the x=15 triple-aggro (candidate node (19,10)). Also corrected:
`m3_topup.sh` never seeded the gear/stats its own handover claimed (added,
idempotent) — without it a cold-sandbox top-up stages a fists-and-rags L13.
Suite **209/209 · 329,058** (+6), ctest 2/2, duel pin `b273be661b54673a`,
r17 journal preserved byte-identical (md5 `b4c0cf3f…`). Devlog 0086 · card
`done/T-125.md` · handover `docs/handover/T-125-r18-r22.md` · journals
`logs/m3_gate_r{18,19,20,21,22,22b}.bwj` force-added. Next card **T-126**,
next devlog **0087**.

## Done — T-126 (2026-09-15): affix v2, table 3 → 10 (H3 loot depth, 1/5)

T-126 CONTENT (epoch 21→22): 7 hooked affixes (Ox/Thorns/Focus/Embers/
Vigil/Greed/Mending, one-per-item, wrong-slot mute); armor-gated hpMax
re-sync in toggleEquip (T-047 pin safe); 10 new tests incl. 1M-draw
distribution + paired-world Greed + Thorns never-kill floor. Gate leg
`logs/t126.bwj` (5 fighters, 40 kills, mm=0); guard refuses t120 exit 4.
Duel pin unchanged. Devlog 0087 · card `done/T-126.md`. Next: T-127 Old Maw
uniques (then Widow T-128, Cantor T-129, Gravemother T-130).

## Done — T-127 (2026-09-15): Old Maw uniques ×3 (H3 loot depth, 2/5)

T-127 CONTENT (epoch 22→23): `UniqueDropDef` + `kUniqueDrops` (3 rows for
1012 @4%, fixed Embers/Thorns/Greed) + `grantUniqueDrop` (cap-32, chatCh 2
broadcast); 3 new ItemDefs (2201/2103/2202, never stocked). 4 new tests in
`test_uniques.cpp` (grows with T-128..T-130). Gate leg `logs/t127.bwj`
(52 kills, mm=0); guard refuses t126 exit 4. COLLISION FLAG: arena PR #23
also claims epoch 23 — T-115 precedent rules. Devlog 0088 · card
`done/T-127.md`. Next: T-128 Red Widow uniques.

## Done — T-128 (2026-09-15): trio uniques ×9, 12/12 set complete (H3 loot depth, 3–5/5 COMBINED)

T-128 CONTENT (epoch 23→24): 9 ItemDefs (2301/2104/2302 Widow,
2303/2105/2304 Cantor, 2401/2106/2402 Gravemother @6%) + 9 `kUniqueDrops`
rows, slot-legal fixed affixes; `test_uniques.cpp` +3 cases (counts/rates,
slot-law sweep, per-boss grants, never-stocked sweep). Combined card —
T-129/T-130 stay free; the 10 → 40 table is a filed follow-up. Gate leg
`logs/t128.bwj` (60 kills, mm=0); guard refuses t127 exit 4. Devlog 0089 ·
card `done/T-128.md`. Next: Phase H4 (Blood Moon + EK + L19 oath).

## Done — T-129 (2026-09-15): Blood Moon flag + two levers (H4 night war, 1/3)

T-129 CONTENT (no epoch bump, stays 24): session moon-to-dawn via `gm
blood-moon` → journaled `kBloodMoon` (H1 shape); curse 60 s + bite ×1.30
while red; scheduler/tint/gating filed as follow-ups. 6 new tests incl.
paired-worlds ratio. `t128.bwj` re-replays mm=0 (neutrality proof). Devlog
0090 · card `done/T-129.md`. Next: T-130 EK ledger + L19 oath.

## Done — T-130 (2026-09-15): EK ledger + L19 town oath (H4 night war, 2–3/3)

T-130 CONTENT (no epoch bump, stays 24, schema v12): `/oath` (L19+,
one-time, journaled kOath) + `towns.h` (Thornwall/Ashen, Marrowgate/Synod)
+ war-kill EK fame instead of stain/wanted (duels + guard-murder excluded)
+ `w`-line journal sidecar (v3 l-line untouched) + `ekBoard`/`gm ek`
readout (site page = Phase O). 8 new tests; T-118 migration pin updated
to v12 (expected cascade). `t128.bwj` re-replays mm=0. Devlog 0091 · card
`done/T-130.md`. Next: Phase S siege battle logic.

## Done — T-131 (2026-09-15): siege scheduler + registration (Phase S, 1/4)

T-131 CONTENT (no epoch bump, stays 24): Saturday 20:00–21:30 window
(pure tick math) + captain-id registration (cap 8, journaled kSiegeReg)
+ holder slot + kSiegeStart battle-to-window-end. Zone-agnostic (map =
PR #23, rehearsal = PR #29, pledge bands = Phase P follow-up). 6 new
tests. `t128.bwj` re-replays mm=0. Devlog 0092 · card `done/T-131.md`.
Next: S2 gates → Heartstone → crown.

## Done — T-132 (2026-09-15): siege gates (Phase S, 2a/4)

T-132 CONTENT (no epoch bump, stays 24): kind-75 Outer/Inner gates on
zone-6 load (staging positions) + `/breach` → journaled kBreach (−10/ram,
300 hp, splinters broadcast at 0). Breach-by-channel by design (setAttack
refuses furniture; killMob would drag loot). 4 new tests. `t128.bwj`
re-replays mm=0. Devlog 0093 · card `done/T-132.md`. Next: T-133
Heartstone + crown.

## Done — T-133 (2026-09-15): Heartstone + crown (Phase S, 2b/4)

T-133 CONTENT (no epoch bump, stays 24): kind-76 stone on zone-6 load +
presence attunement (1200 uncontested ticks, contest freezes, no decay) +
`/crown` → journaled kCrown → 200-tick kneel (move/hit/death/leave/end
breaks) → holder set + battle end + broadcast. `Entity.crownUntil`
session-only. 6 new tests (incl. the movement-validation lesson). `t128.bwj`
re-replays mm=0. Devlog 0094 · card `done/T-133.md`. Next: S3 taxes +
holder persist.

## Done — T-134 (2026-09-15): taxes + vault + holder buff (Phase S, 3/4)

T-134 CONTENT (no epoch bump, stays 24): `siege_state` table (holder,
vault, crowns) + boot load + throttled save; 5% PvE kill-gold tithe
while held; holder +10% hit&dmg; crown books name/crowns/dirty; `gm
siege` readout. Vault spending = Phase P. 6 new tests. `t128.bwj`
re-replays mm=0. Devlog 0095 · card `done/T-134.md`. Next: S4 bots +
M4 gate.

## Done — T-135 (2026-09-15): Weeping Castle map live (Phase S, map 6/6)

T-135 CONTENT (epoch 24→25): adopted castle tmj+generator ex T-123 lane
(byte-stable verified); Thornwall `castle_road` portal (regen-verified);
mapconv + boot zone-6 wiring; validator to 6 maps. Mid-card correction:
no-bump claim failed 12/12 on re-replay (entity-set shift, T-068
precedent) → bumped + fresh `t135.bwj` (mm=0); guard refuses t128 exit 4.
PR #23 SUPERSEDED on the map paths (comment posted). Devlog 0096 · card
`done/T-135.md`. Next: T-136 rehearsal flag + siege bots.

## Done — T-136 (2026-09-15): rehearsal mode + siege bots (Phase S, 4a/4)

T-136 CONTENT (no epoch bump, stays 25): `--siege-rehearsal` (window
bypass, journal marker, exit-4 cross-mode refusal) + `siege` attacker
profile (march/reg/start/breach/crown choreography, fighter except
march, traces, verb telemetry). Drill (900 s, disclosed L15 top-up):
6 bands, battle, both gates, ATTUNED, 369 crowns, replay mm=0; live
crown moves to T-137 (yard-pack pressure mapped over 4 iterations).
t135 re-replays mm=0. Devlog 0097 · card `done/T-136.md`. Next: T-137
defenders + M4.

## Done — T-137 (2026-09-15): defenders + live crown + M4 PASS (Phase S done)

T-137 CONTENT (no epoch bump, stays 25): `--defenders` hold-ring bots +
hold mode (adjacent mobs only) + kiters + party bands (cap counts bands;
spawn mustering) + crown re-kneel guard + breach use-after-erase fix.
M4 VERDICT PASS: flips 3/3 (replay mm=0) + p99 6.6 ms @40 bots (< 25 ms).
Contest-denial proven inverted (12v6/12v3 freeze correctly). Full
forensics in devlog (6 drill lessons). Devlog 0098 · card `done/T-137.md`.
Next: Phase P (pledge-lite).

> NOTE (T-138 rebase, 2026-09-15): the T-122 pledge-lite row below landed via the
> `task/T-138-pledge-lite` cherry-pick of `c0de563` (PR #22 lane); T-121 precedent
> rules — incoming number kept, board position appended.

## Done — T-122 (2026-09-12): pledge-lite (Track B2) — the oath office opens (epoch 22)

T-122: server-only pledge core per the 05-mvp.md cut — **create / emblem
(placeholder 0–9) / ranks Liege·Bloodsworn·Initiate / pledge chat**;
vault/tax deferred to B5 holdings, roster UI + emblem-over-head to a
follow-up wire card. Registrar (72) takes the town-square post
(Chebyshev ≤ 3, confessor pattern); founding gate **level ≥ 10 +
10,000g** — flagged **GDD §8 deviation** (CHA ≥ 20 asked, no CHA stat
exists; switch when the six-stat model lands). Membership by character
name, schema **v12** (`pledge_id`/`pledge_rank` columns + `pledges`
table), journal kinds 28–34 + **g-sidecar** login line (k-line pattern);
pledge state stays outside worldHash by design. **Epoch 21→22** — the
registrar is world composition (T-112 spawn law; t120 replayed 15/15
mismatch before the bump, refuses by guard after). Leg of record
`logs/t122.bwj`: 5 bots found `t122clan`, swear/promote/kick/leave,
relog → membership restored, replay `ticks=1222 cmds=105 hashes=12
mm=0`. Suite **218/218 · 329,270**, ctest 2/2, duel pin unchanged.
Devlog 0084 · card `done/T-122.md`.

## Done — T-138 (2026-09-15): pledge-lite rebase onto Phase-S stack (Phase P, 1/3)

T-138 REBASE: `git cherry-pick -n c0de563` (PR #22 lane) onto
`task/T-137-defenders-m4`. Journal kinds 34–40, schema v12→**v13**,
epoch 25→**26**. Bots keep siege choreography AND pledge ceremony; single
siege-aware fighter gate; whitelist excludes pledge. Repair: loadPledges
spliced inside loadSiege restored. Battery + `logs/t138.bwj` on the PR.
Devlog 0099 · card `T-138.md` (→ done on merge). Next: pledge bands +
vault follow-ups.

## Done — T-139 (2026-09-15): pledge bands muster sworn war-hosts (Phase P, 2/3)

T-139 MUSTER: `siegeRegister` keys bands by pledge id (one pledge, one
band); sworn callers muster missing members into the enlisted band (no slot);
desertion never un-enlists; unaffiliated keep party shape. No epoch (26), no
schema, no wire, no journal change. 286/286 (4 new) · `logs/t139.bwj`
(5-member single enlistment, replay mm=0) · `t138.bwj` mm=0 (neutrality).
Devlog 0100 · card `T-139.md` (→ done on merge). Next: T-140 pledge vault.

## Done — T-140 (2026-09-15): pledge vault, deposit-only MVP (Phase P done)

T-140 VAULT: `Pledge::vault` (hash-neutral) + tithe (kind 41) + vault
readout + sworn-holder drip routing (name-keyed, offline-safe); schema
v13→**v14**; disband burns the pool. No epoch (26), no wire. 291/291
(5 new) · `logs/t140.bwj` (vault=500, kind-41, replay mm=0) · `t138`/`t139`
mm=0 (neutrality). Live-crown drip proof deferred (doctest twins pin it).
Devlog 0101 · card `T-140.md` (→ done on merge). Next: Phase A art.

## Done — T-141 (2026-09-15): Ravager WIP sheets packed + QA-triaged (Phase A, 1/3)

T-141 SHEETS (content-only): 20 landed Ravager keyframes → v1 sheets
(128×384, walk/attack/cast/die × S/SE/E, idle/hurt/gib + W-side padded);
feet gate PASS (m walk_SE floater fixed in-cell); colours ≤32 PASS;
R-LUMA FAIL recorded (Δ 10.7/4.3, remediation = T-142); LICENSES rows.
b5_build.sh exit 0 · suite 2/2 · validate 0/6. No epoch/wire/code.
Devlog 0102 · card `T-141.md` (→ done on merge). Next: T-142 wiring.

## Done — T-142 (2026-09-16): player sheets wired — class/sex to visible (Phase A, 2/3)

T-142 WIRE 237→238: spawn/delta +`classId`+`sex` (237→238 via protogen base 201; old clients refused reason 4), server packs `classId` (sex 0 until T-142b — no DB column), client `NetEntSnapshot` + `playerSheetPaths` + `atlasForPlayer` cache, `kind==0` branches to it, hero fallback on unknown/missing (Gravecaller/Cultist pending). Ravager m/f live; `test_clientlaw` +1 (299/299). No epoch (render-only, `t146` mm=0). Devlog 0107 · card `done/T-142.md`. Merge repair epoch 26→27 (H1/H2 mine+steward vs stack — `t146` leg) landed first.

## Done — T-R-LUMA (2026-09-17): Ravager contrast accept

R-LUMA remediation: rim-light NW bone inside-outline strength1 +9, strength2 +17 but colour 33 and night 11.9 <15; re-quantized drops. Decision ACCEPT provisional — dark horror palette reads via outline/nameplate. Sheets unchanged. Devlog 0108 · card `done/T-R-LUMA.md`.

## Done — T-143 (2026-09-15): ops boxes, agent-closable set (Phase O, 1/2)

T-143 OPS (no code): systemd unit (verify clean) + backup script + REAL
drill (accounts=5 chars=5 pledges=1 uv=14, integrity ok) + GM runbook
(verified verbs only; /ban + broadcast carded as gaps) + crash posture
(journald/coredumpctl). Handshake refusal pre-existing (reason=4).
Director keeps: launcher, site, legal, clean-boot, surveys. Devlog 0104 ·
card `T-143.md` (→ done on merge). Next: M5 pre-soak + T-142 wiring.

## Done — T-144 (2026-09-15): M5 pre-soak signal 60×10 min (Phase O, 2/2)

T-144 SOAK (no code, scratch DB): 60 fighters × 600 s live — clean exit,
0 error lines, tick p99 ≤ 5.1 ms (budget 25), RSS flat 8.2 MB, 60/60
online. Full M5 (200×12 h + MBA fps) stays director-scheduled. Devlog
0105 · card `T-144.md` (→ done on merge). Next: Friday-Night readiness.

## Done — T-145 (2026-09-15): Friday-Night readiness map (human-run)

T-145 READY (no code): 4 acceptance legs → runnable tools + entry
criteria + go/no-go list (`docs/ops/friday-night-readiness.md`). Legs 1–3
runnable on current stack; leg 4 director survey. Devlog 0106 · card
`T-145.md` (→ done on merge). Queue head: T-142 wiring implementation.

## Done — T-159 (2026-09-16): loot depth — 4 tiers + 5 slots + 20 affixes

T-159 LOOT (epoch 28→29, wire 241→242): rarity 0..3 rolled 78/17/4.6/0.4 in
killMob; helm/amulet/ring slots (ItemDef 0..6, +9 rows); affixes 11..20 all
hooked (Pall +acc/+evd via effEvd, Boneyard +5% crit, Last Rites +8 <20% hp in
equippedWeaponDmg); 8-field blob; ItemSlot rarity wire (protogen base 202).
314/314 headless · `logs/t159.bwj` (8×30s, ticks=641 cmds=417 hashes=6,
replay mm=0) · epoch28 guard exit 4. Deviations + T-159f1 in card.
Devlog 0109 · card `T-159-loot-depth.md` (→ done on merge). Next: T-154/T-168.

## Done — T-154/T-168 (2026-09-16): headless gate + one build-dir law
T-154 core (raylib-gated test TUs) already in tree — verified headless
314/314, not redone. Shipped: 9 legs honour `BH_BUILD_DIR` (default
`build/linux-gcc` unchanged); `--out` on 4 mapgens (byte-identical diffs);
CI mapgen gate 1/6→6/6 + `headless` job (no X11) + t159 replay guard;
`tools/clean_clone_check.sh` → PASS on fresh clone. Caught: unquoted `:`
broke ci.yml (fixed, yaml-parsed). Devlog 0110 · card
`T-154-headless-build-law.md` (→ done on merge). Next: T-156 T-ART-12..15.

## Audit wave T-150..T-169 (2026-09-16 playable-audit; drafts in PR #53 / commit a1bd867)

Shipped → `done/`: T-150 (zone6 client map), T-151 (siege/pledge wire+HUD),
T-152 (GM allowlist+/ban/announce), T-153 (argon2id, ADR-0012),
T-154/T-168 (headless gate + build-dir law), T-155 (--bless guard),
T-157 verdicts + F3(a) (M4 FAIL, cause + attune gap carded),
T-158 (GDD truth-up, ADR-0013/14), T-159 (loot depth), T-169 (/help+F1),
T-165-Wave0, wave-2 (creation/five-stat/Resurrect/roster/war/bounty,
epoch 30). Open: T-156 T-ART-12..15 · T-157-F2 bands · T-157-F3 attune
(director call) · T-159f1 loot follow-ups · T-161b kit spine part 2 ·
T-164 night light · T-165 remainder (director merges).
Human-only T-146..T-149 stay director-owned.

## Done — T-158 (2026-09-16): GDD law truth-up + ADRs (docs-only)

T-158 TRUTH-UP: 7 deviation rows all adopt-shipped (ADR-0013 enhancement/
economy, ADR-0014 siege/moon + change-control); GDD §6/§7/§8/§9 amended;
README epoch-18/t107/Phase-4 lines → epoch 29/t159/wave-2; AGENTS.md DoD
+--bless ban +headless gate; T-138..T-145 → done/, stale opens T-104/T-112/
T-142 removed. No code/wire/epoch. Devlog 0111 · card `done/T-158.md`.

## Done — T-157 (2026-09-16): evidence refresh — M4 verdict FAIL, cause found, fixes carded

`m4e29` (12v6 × 750 s @ epoch 29, `logs/t137_m4e29.bwj` force-added): flips 0
→ M4 FAIL; p99 3.0 ms PASS; replay 15042/2155/150 mm=0 PASS. Root cause:
`[gm-denied] tick=1494` — T-152's allowlist denied bot0's single-shot horn,
battle never started (drill-vs-tree, not a sim bug). Open: `T-157-F1`
(horn: BH_GM_NAMES + retry + abort-fast, blocker), `T-157-F2` (7 fragmented
bands, quality). Readiness doc → epoch 29. F1 re-run owes re-verdict; M1 soak
+ M2/M3 scheduled. Devlog 0112 · card `done/T-157.md`.

## Done — T-157 re-verdict (2026-09-16): horn fixed, gates 2/2, flips 0

F1(a) landed in drill; `m4e29b`: battle joined (12 bands), both gates ~tick
5400, attuned 0, flips 0 → M4 still FAIL, gap now contest-side (60 s window
never opened vs 6 holders). p99 2.8 ms PASS, replay mm=0 PASS (leg kept).
Open: `T-157-F3` (attune levers, recommend 16v6/900 s first). Devlog 0113.

## Done — T-157 F3(a) (2026-09-16): 16v6 tried, attune still denied

`m4f3a` (16v6 × 900 s): battle 1, gates 2/2, attuned 0, flips 0 → M4 still
FAIL. p99 4.8 ms PASS, replay mm=0 PASS (leg kept). (a) exhausted — sim
constants unchanged since epoch-25 3/3, smells structural. `T-157-F3` now a
director call: (b) tune attune law or (c) accept gate shape. Devlog 0114.

## Done — Wave-2 epoch 30 (2026-09-16): creation, rebate, roster, war, bounty

Six cards, one bump (epoch 29→30, wire 242→244, schema v15): T-167 creation
(prompt/panel/CharCreate, bots answer, 8/8 live), T-160 five-stat (ADR-0015,
F8/F9), T-161 Resurrect rebate (ch10, deviation recorded), T-162 roster +
night premium (11 rows, D9 rename, validate 0/6), T-163 field-war + /ek,
T-166 bounty persistence (y-sidecar, hash widened). 325/325 (11 new) ·
`logs/wave2.bwj` replay mm=0 + epoch-29 guard exit 4 · CI leg swapped. Gaps:
T-161b, T-159f1, patrol red-name tint, live Resurrect staging. Client compiles
via CI matrix. Devlog 0115 · card `done/T-WAVE2.md`. Open wave-2 follow-ups:
T-161b, T-159f1.

## Done — T-153 (2026-09-16): argon2id password hashing (schema v16)

Vendored argon2-20190702 (pinned hash, `bh_argon2`), m=19MiB/t=2/p=1:
hash 47 ms / verify 44 ms measured. Stub rows rehash silently on login
(proven incl. second-login-uses-argon); garbage refuses reason-1, no crash;
new accounts hash at birth. Single-threaded tick impact stated + bounded.
Runbook §7 wave-1 `--no-register` default. 5 new tests, suite 330/330.
Devlog 0116 · card `done/T-153.md` · ADR-0012.

## Open — Phase 3 remaining (refreshed 2026-09-09: stale rows retired)

| Card | Title | Notes |
|---|---|---|
| L9 pace logistics → **T-114** | Ladder open per T-090/T-100 (n=2, ~80% bar/leg, zero widow/L11); binding constraint is vial flow + bridge travel (30 shop trips/leg), not mob stats | **scoped by T-113 (devlog 0075)**: the 30-trip tax is bot belt policy (4-deep, buys 2) — server already allows 16-deep stockpiling. T-114 = bots v3 flask belt (tools/bots only, no epoch), then t090/t100-discipline re-run ×2; economy/content levers reopen only on gold-starved re-run — T-095 stays shut |
| Bot bad-leg deaths | v5c legs 3/5 perch mode: T-074/T-077 levers measured red + reverted; T-083 closed the drift as roam-RNG (leg totals stable ~130–160, distribution roams) | reopen only on perch-clustered all-L3 legs (T-074 discipline); reproductions: leg 3/5 logs, val5 |

Retired: "L8→L9 step-up, no waypoint" (T-084 opened the (60,41) camp; T-090/T-100 climbed it twice).
