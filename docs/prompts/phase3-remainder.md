# Phase 3 remainder — execution prompt (issued 2026-09-05)

Execute the remaining Phase-3 queue as consecutive sprints, each with full DoD
artifacts (task cards in `docs/tasks/`, moved to `done/` with evidence; devlog;
board update; commit with the bloodhollow-dev identity). Order is fixed by the
user: alignment/PK → anvil gear-churn → day/night → new content drop
(Bonehowl Mine, Drowned Crypt, Gravemother, elites, bounty board) → VFX pass.
Governing docs: `docs/02-gdd.md` (§§4–10), era pins in docs/01-research.md.
Standing constraints: deterministic sim only (all state journal/hash-visible;
socket traffic may use wall-clock), replay must stay bit-exact after every
sprint; the S13/S14 journals are the regression oracle; Linux+macOS targets;
never regress the solo baseline (bots with default kits must keep pacing
parity with S13 gate numbers).

Global mechanics pinned already (do not renegotiate silently): XP share kills
12-tile radius of the KILL, +12% per extra sharer, loot/gold to the killer; —
choral heals at 100% contribution (already true); party cap 8 (GDD says 5 at
MVP — 8 already shipped, keep); aura tiers at 20/50/80/120/150 weapon skill;
moral split ±15% (XP vs gold) live since T-046;
`kLevelCap = 25`, 3 pts/level. Anything in the GDD that contradicts shipped
values keeps the SHIPPED values; update the GDD instead.

---

## S15 — Alignment & PK teeth (execute first)

Cards: **T-056 PK law & chaos penalties**, **T-057 alignment chrome**.

T-056 scope:
- Karma loss on unlawful PK: killer of a lawful/neutral (victim karma ≥ 0)
  non-dueling player takes `−(300 + 20 × levelDiff)` where levelDiff is the
  victim's level deficit below the killer (0 if victim is higher — killing up
  is punished but not deep-red-instant). Karma clamps to ±1000. Victim karma
  >= 0 kills feed nothing else. Killing chaotic (red) players costs nothing
  — GDD-true: the gallows-market handles them.
- Karma whitening: `+1` per mob kill where `mob.level <= player.level`, and
  `+20` per logged hour (tick-accumulated, no wall clock).
- Chaotic (karma < 0) death penalties: drop 1–6 random *non-equipped*
  inventory items at the corpse; 15% per equipped slot rolled to unequip-drop
  too; respawn at the gallows pit (content coords — add a respawn anchor
  tile; for MVP the pit may be a town-edge square with a fiction line), never
  the temple square. Era text on the system channel.
- Vendor refusal: Marta refuses trade when karma < 0 (ch-2 sneer line).
  (Smugglers' Cove fence arrives with the content drop; refusal now, fence
  later is the intended erasable gap.)
- Consent dueling MVP: `/duel <name>` proposals expire in 20 s; accept by
  dueling back (`/duel <proposer>`); kills inside a duel: no karma, no XP
  debt, no drops; the duel auto-ends on kill, `/forfeit`, zone transfer, or
  party acceptance of either side. Announce start/end on ch-3.
- Command kind `kDuel` (journaled like everything else; replay-exact).
- Tests: karma-loss math matrix (vic lower/equal/higher), clamp, lawful-kill
  exemption of red targets, whitening cadence, chaotic drop roll bounds
  (1–6, seeded), gallows respawn for chaotic vs temple for lawful, vendor
  refusal, duel consumes penalty paths, replay parity.
- Gates: duel harness (tools/duel) gains a `--karma` knob proving red-death
  drops; journal replay of a PK'd campaign leg.

T-057 scope:
- Wire: `EntitySpawn + u8 karmaBand` (0 lawful / 1 neutral / 2 chaotic);
  field-growth, version stays count-derived; delta-side update piggybacked:
  recompute band on karma change, emit via the existing statsChanged event →
  server re-spawn? NO — cheap route: band in spawn + a ch-255 "X's soul
  blackens/whitens" line when crossing 0 (delta line not needed for MVP).
- Client: nameplate red for band 2 (era red-name), stat panel gains
  alignment label + explicit gamble line ("karma N — red below 0: drops on
  death, shops refuse") — legibility is the era rule.
- Tests: shard-boundary band computation; crossing event fires both ways.

## S16 — Anvil gear-churn (durability / refine / affixes)

Cards: **T-058 durability**, **T-059 affixes v1**, **T-060 refine**.
(GDD §7/Soma research: repair + monster-part tolls, first 2 attempts
guaranteed, gear churn is the gold sink.)

- T-058: `InvSlot.durability` (max by item tier); melee swings/armor hits
  −1 at per-GDD rate; 0 ⇒ item dormant (kept, contributes nothing, red row in
  inventory UI), repair at Marta for gold (junk tier tax) or parts at the
  anvil. Wire: ItemSlot + 2 bytes.
- T-059 affixes v1: drop-time roll (1 of 4 era mods: whet/leech/ward/swift —
  small single lines, NOT random-tree), shown in name suffix + tooltip row;
  seeded by world rng, journaled by kill replay (mob loot already flows the
  journal).
- T-060 refine: anvil adds +1..+3 refine levels on top of aura tiers —
  attempt table (success/degrade/destroy) using the existing mercy mask;
  parts+golds toll scale with refine level; destroy feeds karma negative
  (already plumbed).
- Gates: durability can't go negative, dormant items never swing; replays
  bit-exact; anvil leg with pelts/gold economy shows the churn loop.

## S17 — Day/night gameplay

Cards: **T-061 nightcreep**, **T-062 night economy**.

- T-061: night window (game hour 22–06) — night packs buffs: +15% HP/dmg,
  graveyard table swaps (ghouls get "Restless" aura at night: +1 aggro,
  +10% speed), Gravecaller Blood Bolt +25% at night (GDD), Blood Curse hook
  tag only (MVP-lite listing).
- T-062: night loot table +25% drop chance (era danger pay), XP +10% at
  night. **Legibility floor (locked research finding — Soma private servers
  added permanent-day because nights were pitch black)**: tint floor does not
  go below the existing readable cap; horror comes from content, not
  darkness. Client clock dial already exists (H/N debug keys — keep).
- Global: night schedule must be deterministic from tick (it already is) —
  no new journal surface.
- Tests: hour gating exact (22:00:00 tick boundary), buffs swap, Blood Bolt
  night-up, tint-floor assert on the renderer constants.

## S18 — Content drop: Bonehowl Mine + Drowned Crypt + Gravemother + elites + bounty board

Cards: **T-063 maps**, **T-064 Gravemother + elites**, **T-065 bounty board**.

- T-063: `mine.bhmap` + `crypt2.bhmap` (generator + converter), portal wiring
  from fields-overflow; monster pallets per GDD §9 (Mine Wretch, Lantern
  Spider, Drowned kind…), density rule of S7 (see T-030 card).
- T-064: elite flag on spawner rows (×8 XP), named Gravemother boss (L14,
  ×20 XP, bespoke melee + brood-spawn) — wireKind reclamation per T-047
  discipline; leashes per T-034b-style caps.
- T-065 bounty board: town furniture; lawful karma players post/claim PK
  bounties on chaotic heads (claim pays the poster's stake + server honor
  cut); board state session-scoped (MVP), EK leaderboard hook deferred.
- Tests + gates: spawner immutability, boss solo-impossibility duel-harness
  number (a solo veteran should cap ~5/9 wins — era), bounty money flow
  conservation test.

## S19 — VFX/audio pass 1

Cards: **T-066 hit frames + ability callouts**, **T-067 audio spine**.

- T-066: 2-frame hit flash (white→palette), dedicated per-channel callout
  colors already partially present — sweep to: heal green, bless gold,
  curse violet, crit "!" red-cap, party-share shimmer. Death: brief
  petrify-fade before despawn. All client-side, zero wire.
- T-067: procedural audio stubs (sine-swipe swing, thud hit, choir sting on
  mend/bless, anvil toll) synthesized at boot into raylib Wave buffers —
  no asset downloads in-sandbox; keep < 200 KB total; "audio pass 1" is
  presence, not polish.
- Gates: soak smoke with client hidden; no test breakage; screenshots
  attached to the devlog.

---

Execution discipline: one system = one card = one session = one commit
message; board and M2b-final stay open; after S15/S16 land, re-run the
dual-kit gate leg (S14 script) to confirm pack-camp compatibility. The
M2b-final L1→8 rerun fires after S18 (boss + bounty need to exist in the
sitting).
