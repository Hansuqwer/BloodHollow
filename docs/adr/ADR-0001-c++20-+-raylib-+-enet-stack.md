# ADR-0001 — C++20 + raylib + ENet stack

**Status:** accepted (Sprint 2, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

Selected C++20 with a hand-rolled engine on raylib 5 (client), ENet transport, SQLite persistence, single monorepo. True custom engine per project brief; one language across client/server/tools minimizes review surface for a solo director + AI agents. Alternatives rejected: Rust (iteration cost), C#/MonoGame (weaker custom-engine claim), Unity/Godot (not a custom engine, worse headless story).

## Consequences

Consequences: one toolchain to teach agents; raylib winding quirks documented in code; C++ discipline enforced by -Werror + tests.

## References

docs/03-architecture.md section 13; task board (docs/tasks/README.md).
