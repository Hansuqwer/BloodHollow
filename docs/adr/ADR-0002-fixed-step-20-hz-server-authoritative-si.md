# ADR-0002 — Fixed-step 20 Hz server-authoritative sim

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

Sim runs at fixed 20 Hz using integer ticks and Q10 fixed-point positions; clients send intents only; server validates speed/range/cooldowns. Era-authentic feel and provable determinism; no client authority ever.

## Consequences

Consequences: movement code lives in shared/sim (Walker) usable by client, server headless, and replay harness; floats restricted to render code.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
