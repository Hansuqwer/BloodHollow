# ADR-0003 — Monorepo, zones-in-process v1 with split-ready API

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

One repo (client/server/shared/engine/tools); the world server v1 hosts all zones in one process as modules behind a location-agnostic zone API.

## Consequences

Consequences: single VPS ops; splitting into multi-process later is an ADR-level change touching zone routing only.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
