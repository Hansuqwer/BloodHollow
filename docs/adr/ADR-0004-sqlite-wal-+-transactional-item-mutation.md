# ADR-0004 — SQLite WAL + transactional item mutations

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

Persistence in SQLite (WAL), dirty-entity flush every 30 s + logout; trades/refines/item mutations in single transactions; schema versioned via migrate.sql files; daily .backup offsite.

## Consequences

Consequences: zero-ops alpha hosting; dupe/poof bugs contained by transactions; Postgres migration path documented but not built.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
