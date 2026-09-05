# ADR-0005 — Deterministic RNG + replay journals as QA oracle

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

All gameplay RNG via sim/rng.h (xoshiro256**, splitmix64 seeding, Lemire range mapping, no <random> distributions); Walker state is integer-only; client/server record journals (events + per-tick state hashes); tools/replay re-sims and diffs hashes.

## Consequences

Proven Sprint 2: 220-tick scripted session replays with 222/222 hash equality; tampered journals fail at the exact divergent tick.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
