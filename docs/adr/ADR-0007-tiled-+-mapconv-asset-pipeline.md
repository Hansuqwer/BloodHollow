# ADR-0007 — Tiled + mapconv asset pipeline

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

Maps authored in Tiled (or the deterministic generator), converted by tools/mapconv to binary .bhmap (RLE layers, blocker bitfield, checksum); CI regenerates and diffs sources; renderer-refusable assets via --validate.

## Consequences

Consequences: binary artifacts are build outputs; generator change = reviewable code change; art budgets enforced at convert time (later).

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
