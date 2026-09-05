# ADR-0008 — Bots-first testing strategy

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

tools/bots lands before combat: scripted headless clients (grinder, support, wanderer, siege profiles) provide soak, load, and siege-rehearsal coverage; CI runs a 10-bot smoke.

## Consequences

Consequences: part-time solo director can test MMO-scale interactions without a playtest team; perf gates (tick p99) are CI-enforced.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
