# Session handoff — post-wave continuation (2026-09-12)

*Follows `docs/prompts/session-handoff-round3-2026-09-09.md` by three days,
seven journal epochs, one review era, and one merge wave. This handoff closes
the reconciliation era and opens the build era.*

## Your first move

Read **`docs/prompts/continue-building-2026-09-12.md`** — it is the standing
order for every build session from here: verified state snapshot, the law
(AGENTS.md condensed), card→PR workflow, the exact verification battery with
expected numbers, the build queue (Track A: M3 party-crypt gate · Track B:
Phase-4 spine · Track C: debt), a numbers cheat sheet, and the traps. Then run
its §10 startup checklist. **If the board contradicts it, the board wins.**

## Ground state (verified on master after the T-116 wave + this filing)

- `master` at journal **epoch 20**, PR queue **empty**, all wave branches
  deleted from the remote. This filing PR is the only thing landing.
- Suite: **206/206 doctest cases, 329,022 assertions, 0 failures**; `ctest`
  2/2; warning-free build (`-Wall -Wextra -Werror`).
- Duel pin: `bh_duel --selftest` → `win=1 ticks=41 hpEnd=82
  hash=b273be661b54673a` (deterministic).
- Gate leg of record: `logs/t115.bwj` → `[replay] OK ticks=202 sessionCmds=302
  hashes=3 mismatches=0 entities=191`. Stale legs (t107 @18, t104/t112 @19)
  are REFUSED by the epoch guard — that is correct, never "fix" it.
- Wire `kProtocolVersion` **237** · persist schema `user_version` **v10** ·
  `.bhmap` **v2** · offline builds work (vendored sqlite in
  `third_party/sqlite-amalgamation/`; pre-seed ENet in `build/*/​_deps/` to
  avoid its configure-time download).
- Soak p99 ≈ 1.3 ms at current scale (budget 10 ms; Phase-5 bar 25 ms @ 200
  bots).
- Git identity for task work:
  `git -c user.name=bloodhollow-dev -c user.email=dev@bloodhollow.local`.

## What this session did (all evidence in `done/` + devlogs 0077–0079)

| card | content | outcome |
|---|---|---|
| T-115 | Lineage reconciliation of the two epoch-19 lineages (union tryAnvil, epoch 20, fresh t115.bwj leg) | PR #14, merged as a **true merge commit** `0355367` |
| T-116 | Merge wave per `merge-wave-2026-09-12.md`: #12, #9, #15 (replaced #13), #10 squashed; #8 auto-closed via ancestry; #11 closed superseded; evidence PR #16 | master `c8d8087`, queue empty, all gates green |
| T-117 | This filing: continuation prompt + this handover (docs-only) | you are reading it |

Wave incidents worth remembering (also in the continuation prompt §8): a raw
base-branch delete closed PR #13 (recovered as #15 — retarget children
before deleting a base); `gh pr edit --base` fails silently (use the REST
PATCH); the board README junction conflicts by construction (keep ALL
sections, card order).

## Open director decisions — do NOT implement unasked

1. **Build-queue ordering.** The continuation prompt proposes M3 party-crypt
   gate first, then the Phase-4 spine (weapon-skill persistence → pledge-lite
   → Weeping Castle → siege law → holdings → bot army → anti-grief → EK
   board). The director may reorder; Blood Moon is explicitly a director
   call (roadmap Phase 4 vs GDD §10 v0.2 conflict).
2. **Toll verdict follow-through** — T-093 decision B (keep Tier-1 toll,
   multi-leg progression); verify its one-line docs edit landed, don't
   renegotiate.
3. **Human-only slices** — T-033 trade-pass UX session and T-099 packet
   performance stay director-scheduled.
4. **Alpha blockers** (Phase 5): real auth past the ADR-0009 stub,
   patcher/launcher, crash reporter, packet fuzzing — not started unless
   ordered.

## Standing watch items

- Wander-death drift: CLOSED as roam-RNG noise (T-083). Reopen only on
  perch-clustered all-L3 legs (T-074 discipline).
- Widow/gnoll tuning: T-095 shut; faster L9 is a **content-pacing** question
  (T-113/T-114), economy levers reopen only on a gold-starved re-run.
- Known debt, priced not scheduled: weapon-skill resets per login (T-096 —
  no schema column; first Phase-4 card), EntityStore SoA / per-tick
  allocations (T-110 truth-up), property fuzz (aspiration, not practice).
- Remote branch hygiene: stale per-task stubs `task/T-104..T-110` and
  `task/T-112-t111-epoch-followup` remain (history/rollback — leave them);
  `arena/01a09574-bloodhollow` belongs to a **different session — never
  touch or delete it**.

## Sandbox notes (if you are an Arena session on this repo)

- The main tree stays on the session branch; do task work in linked
  worktrees (`git worktree add -b task/T-NNN-slug build/wt/NNN master`).
  Anything under `build/` is disposable.
- `source /home/user/toolchain/env.sh` for the toolchain. `git`/`gh` auth is
  pre-configured — never ask the user for tokens.
- Never push to `master`; never self-merge (the two 2026-09-12 merge
  authorizations were single-use, documented in
  `merge-wave-2026-09-12.md` and `merge-filing-2026-09-12.md`, and are
  explicitly not precedent).
- Next counters (verify on the board anyway): card **T-118**, devlog **0080**.

*Filed as part of T-117. The review era is closed; the oracle is green; go
build the endgame.*
