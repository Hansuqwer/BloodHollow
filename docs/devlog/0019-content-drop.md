# 0019 — Bonehowl & the Drowned Apse (Sprint 18): content drop

## What shipped

**T-063 maps.** Two new hand-written determinist generator scripts mint two
zones: **Bonehowl Mine** (56×40 — carved drift, two galleries, ore spur,
damp crab-vein, camp spawners of bats/hounds/gnolls/widow) and **Drowned
Crypt Depths** (48×36 — causeway over black water, tomb rows, boss apse).
Portals pair cleanly: fields-east-mouth ↔ mine shaft, crypt-vault-stairs ↔
causeway. `validate_links` sweeps 5 maps with **0 problems**.

**T-064 Gravemother + elites.** So the brief reads *L14 boss ×20, elites ×8*;
the shipped law reads those multipliers off XP (era-transparent in the
table): Gravemother pays 4000 XP (×20 baseline), Sepulcher Elites 3200 (×8
line). Blood Bolt finally lands — the signed "Blood Bolt +25%" clause from
S17 — as a ranged cast at 2–6 tiles when she's been aggroed: ×50% DEF cut,
×125/100 after dark. Eight elite camps ring her apse approach so the
fight has a procession, not just a room.

**T-065 Wanted Board.** Beside Marta, furniture wireKind 66. Stand near it
and the sheet pins itself (chat line names the purse); hunt the quarry and
the gold lands *on top of* the natural band — the board pays for the rumor,
not the corpse. No persistence: the board is part of the session fiction.
Quarry cycles every 90 world-minutes.

## What changed structurally

- worldHash now includes zones 4/5 (plus anvil/board furniture): entity
  count on the S18 leg jumped 291 → **579**. Journal epoch **4→5** is the
  correct contract response — old journals stale, not broken.
- Boss kit on MobDef: `boss`, `boltRange`, `boltCdTicks` zero-tolerated for
  trash rows. Melee keeps cadence on `lastSwingTick` shared with the bolt
  (one heartbeat, two casts).

## Gates

- Suite **91/91** (327,815 assertions) incl. S18 suite: 2 zone-asset cases,
  table law, bolt end-to-end, bounty lifecycle.
- Smoke leg replay **bit-exact**: `ticks=1255 sessionCmds=204 hashes=12
  mismatches=0 entities=579` (logs/s18_smoke.bwj, epoch 5).

## The M2b-final trigger (now firing)

S18 closes the content drop ⇒ the board's L1→8 campaign gate re-runs.
`tools/m2b_gate_chain.sh` (up to 10 legs × ~10 min) is running in the
background: leg journals land at logs/m2b_leg{N}.bwj, verdicts in
logs/m2b_gate.log. Sprint 19 (VFX/audio) proceeds in parallel; the chain's
result reports when done.

## Next (Sprint 19)

- **T-066** hit frames/callout sweep — flash budget per ability, red-caps
  capital letters law (already: Power-Swing!, BLOOD BOLT, aura bands).
- **T-067** procedural raylib `Wave` audio: <200 KB of synthesized kit
  (swing, hit, bolt, anvil ring, level-up chime) — no asset licensing risk.
