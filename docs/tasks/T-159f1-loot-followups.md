# T-159f1 — Loot follow-ups (filed 2026-09-16, after T-159)

**Status:** `open` — deviations banked from T-159, wave-2 left them standing.

## Scope

1. Multi-affix items (Magic 1–2, Rare 2–3): needs a schema change (affix is one u8) — own ADR + migration + blob grammar bump.
2. Per-band drop tables toward ~45 rows (wave-2 added +9 +11 mob rows; vendor/fence presence re-check).
3. Client rarity chrome (name colour + bag-row marker — wire already rides ItemSlot rarity since T-159).
4. Statistical Boneyard pin (seed-fragile — find a deterministic formulation).
5. Live 200-kill all-tiers drop log (drop-log instrumentation first).

Each item is its own commit; batch shares one epoch bump only if sim moves.
