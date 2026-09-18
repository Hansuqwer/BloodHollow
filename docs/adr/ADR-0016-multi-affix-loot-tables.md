# ADR-0016 — Multi-affix items + per-band drop tables (T-159f1.1/.2)

## Status
Accepted 2026-09-17 (agent wave R2; director review on PR).

## Context
T-159 ships one affix per item (Magic=1, Rare=1 — the Rare row never got its
second roll). T-159f1 items 1–2 owe: Magic 1–2 / Rare 2–3 affixes, and
per-band drop tables toward ~45 rows (kGearDrops has 4). Both are sim changes
under old journals (drop RNG stream + worldHash via the inv blob) → one
shared epoch bump (30→31). The affix ride-along also needs two more wire
bytes on ItemSlot → wire 241→242.

## Decision
- `InvSlot` gains `affix2`/`affix3` (u8, 0 = none). Blob grammar 8→10
  fields (`...:refine:rarity:affix2:affix3`); legacy tails default 0/0
  (T-049x compat rule, no SQLite migration — the blob is schemaless TEXT).
- Rolls: Magic = affix always + affix2 at 50%; Rare = affix + affix2 always +
  affix3 at 50%. Distinct by construction (mod-shift on collision, no extra
  RNG draws — keeps the stream auditable). Slot-gating per affix unchanged
  (wrong-slot = flavor, T-059 precedent); `hasAffix` matches any of the three.
- Uniques keep ONE fixed affix (boss path untouched).
- kGearDrops grows 4→45 rows banded by mob level (L1-3 → light gear … L12+ →
  warden gear), chances 2–5%, existing itemIds only. Every matching row rolls
  independently (T-127 per-row precedent — the table is live, not
  decorative). Economy note: expected gear/kill rises ~2× on covered mobs
  (2–3 rows × 2–4%); absolute rates stay low and the 32-slot cap still gates.
  Revisit only on gold-starved legs (T-113 discipline).
- Epoch 30→31 + fresh gate leg `logs/t159f1.bwj`; guard refuses `wave2.bwj`
  (epoch 30) exit 4. Wire 241→242 via protogen base 202→203 (field-addition
  class, T-142 precedent).

## Consequences
- Client shows up to three affix names in the bag row (rarity chrome first,
  affix names after — T-159f1.3 order kept).
- Old 8-field blobs (DB rows, journals) parse losslessly; re-saved rows
  upgrade to 10 fields on next logout save.
