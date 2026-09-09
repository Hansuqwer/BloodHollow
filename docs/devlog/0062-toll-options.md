# 0062 — The toll, priced two ways (T-093)

T-093 executes the "price it, do NOT build" pattern a second time (after T-076/B). No code changed, no tests, no epoch impact.

## Why B (non-binding)

The toll has the shape of a decision, not an accident: parts-gated, skill-gated, gold-gated, fiction that says "earn". Retunes that preserve the shape but lower one number (A) buy almost nothing measurable — the skill gate still binds in-leg, so the telemetry would read 0 either way and the epoch bump would be spent for a feeling. If the director's goal is grafts inside single soaks, say so explicitly and the combined lever (skill + pelts) gets priced next — that is a different design (tutorialized grafting) wearing the same card number, and it deserves its own sentence of intent first.

## Pointers verified tonight (not re-learned later)

- Toll literal: `shared/content/auras.h:25` (`{1, 20, 4001, 30, 120, ...}`).
- Gate order in `tryAnvil`: proximity → armed → order → skill → gold → parts (skill refuses before gold — a poor pilgrim with 30 pelts and skill 19 still hears "hand not steady").
- Bot rite math needs no change under either option (it reads the tier table live).
