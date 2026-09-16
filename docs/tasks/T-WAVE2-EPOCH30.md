# T-WAVE2-EPOCH30 — Wave-2 batch: one epoch bump for five sim cards (PROPOSAL 2026-09-16)

**Status:** `proposal` — director approves the batch + the three recommendations below, then cards execute in order on one stacked branch with a SINGLE epoch 29→30 bump + fresh leg at the end.

## Why a batch

T-161 (kit channels + `chUnlock` width), T-162 (spawners/entities), T-163 (warband spawner), T-166 (bounty payout law), T-167 (creation flow + migration) ALL change sim semantics or world composition. Five separate bumps = five dead leg generations. One bump, one leg, one replay proof.

## Order (risk-first, content-last)

1. **T-167 char-create** (schema additive + migration pin + creation panel). Blocks visible-class payoff, touches login — highest regression surface, goes first.
2. **T-161 Resurrect-first** (ch10+ appended channels, journaled, replay-exact). Wire/DB-visible width change called out, not snuck.
3. **T-162 roster** (D9 rename 1007→Waxen Celebrant + missing rows + night-only + spawner placement; mapgens must stay byte-identical).
4. **T-163 field-war** (Marrowgate warband spawner + `/ek` player readout; no second city).
5. **T-166 bounty** (per-character mark persistence, additive schema; payout-once pin; pickup-N cut recorded in GDD §12).
6. Bump 29→30 + fresh substantive leg (siege + pledge + refine + trade + mine + PK + bounty + EK) + CI leg swap t159→new.

## Recommendations (director decides; staff recommends)

- **T-160 stat model → (B) five stats.** Six-stat CHA needs schema + wire + UI + three economy hooks for one stat; the shipped game already balances without it. Keep STR/VIT/DEX/INT/MAG, make INT/MAG assignable (casters exist — also fixes the stale `messages.md` line), hard-code aura radius 12 + pledge gate, strike CHA from GDD §3/§8 with ADR + change-control note. Implement inside wave-2 (touches the same creation/stat panel as T-167).
- **T-167 slots → 1-char alpha.** MVP's 4 chars/account + offline slots is a second card (char-select UI + per-slot persist). Alpha wave 1 ships ONE char/account WITH class+sex at creation (unblocks T-142 visible classes + T-162 art targeting) + migration default for existing rows. Change-control sentence for the MVP row cut goes in the T-167 ADR.
- **T-153 argon2id → APPROVE, own session.** Vendored argon2 + ADR-0012 + schema v15 (`kdf` PHC string) + rehash-on-login migration + `--no-register` wave-1 posture. Security-sensitive + network fetch: exactly one card, no batching. T-109 limiter stays as-is.

## Explicitly OUT of wave-2

- **T-164 night light** → pairs with the T-ART graphical wave (needs screenshot matrix + fps evidence; headless cannot prove it).
- **T-156 renderer cards** → same graphical wave, order 12→15→13→14.
- **T-153** → own session per above. **T-165 remainder / old PRs** → director merge lane. **T-157 20×1800s soak + full M5** → director-scheduled.
- **T-160 if director picks (A) six stats** → leaves wave-2 (schema+wire+economy bigger than the batch; own card).

## Preconditions

T-157 m4e29 verdict recorded; PRs #54/#55 merged (stack base moves to master); Friday-Night date set (wave-2 lands before or after per director — say which at approval).
