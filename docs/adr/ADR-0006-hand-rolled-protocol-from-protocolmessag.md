# ADR-0006 — Hand-rolled protocol from protocol/messages.md

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

Wire format is generated from a single human-editable spec file with an explicit version byte; no protobuf/flatbuffers at our message count.

## Consequences

Consequences: protocol bumps are grep-able; version gate at login; revisit only if message count explodes.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
