# 0055 — The Ghoul is fine; the ruler wobbles (T-087)

T-087 closes the mid-band TTK watch. The Ghoul touched 8.9 twice and the room kept saying "noise, not closed". Now it is closed, with the reason written down.

## Two rulers, two readings

- The **duel ruler** (clean 1v1, fixed seeds): Ghoul dies in 9.7 s to fists, 5.7 s to a shank, 4.8 s to a blade, under a second to a geared skilled alchemist. Twice-generated tables are byte-identical. This ruler has not moved — the Ghoul's row and the combat math are byte-untouched since Sprint 18.
- The **soak ruler** (`TTK(25t hps)` off live legs): 8.5–177.6 s across 16 legs, because it measures everything at once — who killed (L1 fists vs L8 blade), how far they chased, how big the pack was, and how few samples there were (n = 10–41). A leg where L8 campaigners swat ghouls between widow pulls (t084, n=10, 74.7 s) is not a balance signal; it is a scheduling artifact.

## Judgment calls

- No re-tune: chasing a regime-confounded metric with mob numbers would be exactly the blind patch the stop conditions forbid.
- The re-pin (9.7 s duel-med / 8–25 s soak band at n≥20, n≥30 to claim) makes the next wobble self-adjudicating: inside the band, ignore; outside with small n, re-run; outside with n≥30 and a killer-mix table, open a real card.
- Duel-table CSVs stay tracked evidence (s12/s21/s22 precedent); the duplicate generation run was deleted, not committed.
