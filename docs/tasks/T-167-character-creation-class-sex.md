# T-167 — Character creation: class + sex at creation (1 char/account, ADR'd) (P1, audit 18/20)

## Context
Audit findings **B12.1 / B2.1 / B2.2 / E1.1 / P0-5**, and the §7 C1 cut
recommendation. Today `Db::loginOrCreate` selects `… FROM characters WHERE
account_id=? LIMIT 1` and inserts exactly one character named after the account
(`server/src/persist.cpp:332,370`) — MVP §1 asks for **4 chars/account + offline
char slots**. Class is sworn after spawn with `/kit <name>`, and
`grep -rni "\bsex\b" server/src shared client/src` returns **nothing**: there is
no sex field in `Entity`, `CharacterRow`, or the wire, which is exactly why T-142
(PR #49) flagged a scope alert — the sprite work needs both.

## Scope
- **Director decision first (ADR)**: ship 1 char/account for alpha (audit C1) or
  the MVP's 4. This card implements **class + sex at creation either way**, since
  both are prerequisites for visible classes (T-142/T-156).
- Creation flow: `Hello` → if the account has no character, a `CharCreate`
  exchange (name already fixed by the account; choose kit 1–3 and sex m/f) →
  `LoginResult`. Persist `sex` (and keep `class_id`), widen `OwnStats`/snapshot
  only if T-142's wire didn't already carry them (coordinate — do not ship two
  versions of the same field).
- Client: a minimal creation panel (era parchment, two rows of choices, no art
  dependency beyond text) shown before entering the world; `/kit` remains as a
  fallback only until first oath, then refuses (keep the existing one-time law).
- If 4 slots are chosen: char-select list, per-slot persistence, and the offline
  slot view (MVP §1) — that is a second card, not this one.
- OUT: respec, race selection (human only), appearance sliders, name-change tokens.

## Acceptance criteria
1. ADR records the 1-vs-4 decision and the change-control sentence (MVP §1) if the
   MVP row is cut.
2. New account → creation panel → choose Gravecaller/female → in world as that kit;
   sqlite row shows `class_id=2` and the new sex column; relog restores both.
3. Existing pre-creation characters migrate with a defined default (no silent
   re-kit); migration pin on a v14 fixture DB.
4. Sprites follow: with PR #49 merged, the chosen class/sex resolves to a packed
   sheet where one exists and falls back to the hero otherwise (client-law pin).
5. Replay `mm=0`; schema bump additive; epoch bump only if world composition
   changed (state which).

## Tests required
Migration fixture, creation-path pins (refusals: duplicate, unknown kit), persist
round-trip, client-law sheet resolution.

## Evidence owed at merge
ADR, suite count, screenshots of the creation panel + a female Gravecaller,
migration proof, replay line, devlog, board row.

## Staff recommendation (2026-09-16, director decides)

**1-char alpha.** Ship class+sex at creation for one char/account now (unblocks
visible classes); 4-slot char-select is a follow-up card with a change-control
sentence. Rationale + wave-2 sequencing in `T-WAVE2-EPOCH30.md`.
