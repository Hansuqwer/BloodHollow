# T-146 — Launcher / patcher (human-only)

**Status:** `human-only` — director-owned, not agent-closable per `PROMPT-RESUME-MVP-2026-09-17.md` §2 (stop trigger: human-only items).

## Scope (MVP `docs/05-mvp.md` §7)

- Launcher updates client against manifest; wrong-version login refused (`kProtocolVersion` 238, `LoginResult` reason 4 already on `task/T-142-wiring-impl`).
- Client fetches manifest, rsync-like patch, version gate on `Hello.protoVersion`.
- Out of scope for agent: binary signing, installer, auto-update UI.

## Acceptance

- Clean-machine `bh_client` pulls manifest and self-updates; version mismatch refused.

## Agent action

- File card only; no code. Human implements.
