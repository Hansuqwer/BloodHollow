# 0117 — T-161b.1 Sanctuary (Cultist ch11) + T-161b.2 Weakness (ch12)

## What
Ground healing circle, first T-161b item. Cultist 14, 20 MP, 600t CD (the only
GDD number), radius 3, 600t hold, 40t pulses of (12+L)/2, Mend-mirror curse
math, recast moves (no stacking). Server emits kind 17 → client rings the
T-ART-09 circle (`addCircle` finally has a caller) + SANCTUARY floater + cast
pose + choir hum. Zero RNG; worldHash untouched.

## Why this shape
- Circles derive from journaled kSkill commands only → live and replay compute
  identical HP through already-hashed entity fields. Neutrality is structural,
  proven by leg: `wave2.bwj` re-replays mm=0 at epoch 30 (no bump owed).
- Numbers beyond the CD are flagged derivation in code + card (unlock 14 sits
  between Mass Mend 12 and Resurrect 20; MP 20 between 18 and 25; radius 3
  holds a tight party, not a zerg; 15 pulses ≈ 4.5× a Mass Mend burst only if
  the party stands its ground — the positional tax is the balance).

## Evidence (item 2 — Weakness)
- 5 new pins in `tests/test_weaken.cpp` (mob −15% dmg/def, all gates incl.
  self/party refusal + stranger takes it, 160t expiry, Purify lifts it,
  replay-path dispatch). Suite 341/341.
- Channel table 12→13 (Cultist ch12=12, others 0); wave-2 pins untouched
  (they only asserted ch10/ch11).
- `weakUntil`/`lastWeakTick` deliberately OUT of worldHash (T-133 precedent);
  `wave2.bwj` mm=0, epoch stays 30.
- Suite 336/336 (6 new pins in `tests/test_sanctuary.cpp`: cast, gates,
  pulses-vs-regen-control, expiry/recast, curse 75%, replay-path dispatch;
  wave-2 spare-slot pin updated to ch11=14).
- `headless` + `linux-gcc` builds warning-free; client compiles (render-only).
- Fresh leg `tools/t161b_sanct_leg.sh` (L14 crypt party, 300 s, disclosed
  staging): `logs/t161b_sanct.bwj` c11=2, replay mm=0 (60 hashes).
- Choir-bot casts ch11 in both support blocks (35 s period clears the 30 s
  server CD); SUMMARY + per-bot lines carry c11.

## Debt / next
- T-161b items 3–6 untouched (Raise/Corpse need entity law + epoch).
- Client circle persists until zone reload (decal law) while the hold is 30 s —
  accepted, matches the Wither-anticipating contract.
- M3 party-vs-solo re-measure still owed at card close (fuller kit).
