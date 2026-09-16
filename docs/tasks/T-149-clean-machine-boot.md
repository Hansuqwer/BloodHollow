# T-149 — Clean-machine boot (human-only)

**Status:** `human-only` — director-owned.

## Scope (MVP §7)

- Game boots from clean install on Linux and macOS by a person who is not you (no dev env, no `.env`, no prebuilt `build/`).
- Verifies `cmake --preset linux-gcc`, `bh_server`, `bh_client`, `bh_maps`, `validate_links 0/6`.

## Agent action

- File card only; director schedules.
