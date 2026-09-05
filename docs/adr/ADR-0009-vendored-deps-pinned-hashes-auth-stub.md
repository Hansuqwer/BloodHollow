# ADR-0009 — Vendored netcode deps & auth stub boundaries

**Status:** accepted (Sprint 3–4, 2026-09-04) · **Owner:** director · **Agents:** binding

## Decision

External dependencies are fetched by CMake `file(DOWNLOAD)` with pinned **SHA-256
hashes** in `third_party/CMakeLists.txt`, never by "get latest" convenience:

| Dep | Version | SHA-256 |
| --- | --- | --- |
| ENet | 1.3.18 | `28603c895f9ed24a846478180ee72c7376b39b4bb1287b73877e5eae7d96b0dd` |
| SQLite amalgamation | 3430200 | `a17ac8792f57266847d57651c5259001d1e4e4b46be96ec0d985c953925b2a1c` |

Both are compiled as plain static libraries (`bh_enet`, `bh_sqlite`) with their
include dirs marked `SYSTEM` so `-Wall -Wextra -Werror` applies only to our
code. Neither upstream CMake works under CMake 4.x (they predate the 3.5
floor), which is why we bypass their build systems — this is deliberate, not
lazy.

**Auth stub (hard boundary):** `server/src/persist.cpp` implements
`stubPasswordHash` — 4096× iterated salted FNV-1a. This is a *plumbing stub*
to develop the login flow, **not a cryptographic primitive**. It MUST be
replaced with a memory-hard KDF (argon2id or scrypt) before any build leaves
localhost development. Tracked on the board; any public/LAN test without the
replacement is a release-blocker. The accounts table schema already stores
`salt` + `pwhash` as opaque byte-strings, so the swap is code-only, no
migration.

## Consequences

- Reproducible builds: a corrupted/mutated tarball fails the hash check at
  configure time, never at link time with a different lib than intended.
- Protocol version (`kProtocolVersion`, currently **1** — the first shipped
  wire format) is exact-match gated at `Hello`; version bumps are deliberate
  compatibility breaks, never silent.
- Agents must not paste ENet/SQLite sources into-repo (licensing hygiene +
  hash pinning keep provenance auditable).

## References

`third_party/CMakeLists.txt`; `server/src/persist.cpp` (`stubPasswordHash`);
`shared/protocol/messages.md`; `docs/03-architecture.md` §14 (security).
