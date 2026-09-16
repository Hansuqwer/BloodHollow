# Arena Agent — Parallel Work Evaluation Brief (B0.5)

> **Deliverable type:** prompt / brief (evaluation only)
> **Status:** DRAFT — this evaluation does **not** advance the B0 style-tile gate
> **For:** arena.ai asset agent (art-domain contractor)
> **Context:** the engine/sim-centric "other agent" is active in the same repo right now.
> The director wants to know **what you (arena) can still produce in parallel** — with zero
> rework risk, no collisions with the sibling agent, and no dependency on approvals
> you don't have.

---

## 1. Role & mission

You are the B0 art-pipeline agent. Your mission in this brief is **analysis and
evaluation only** — you will **not** generate final art, not modify engine code,
and not commit anything without explicit instruction.

Answer, in one written artifact, the question:

> **"What can the art pipeline produce next in parallel, while the sibling agent
> is busy with its own stream — and which of those things should it be?"**

This brief defines the method, the hard rules, the ingest list, the candidate
space you must evaluate, and the exact deliverable file.

---

## 2. Hard rules (bind in this order)

1. **No production without approval.** The B0 style-tile gate (director sign-off)
   is still open. This brief does not open it. Evaluation and explicitly-labelled
   provisional pre-drafts are allowed; volume generation is not.
2. **Lane separation.** The sibling agent owns: `engine/`, simulation work,
   `docs/tasks/` (card creation is coordinated, see §7), `docs/devlog/`, `logs/`,
   and read-mostly shared content. You own: `docs/art/`,
   `docs/research-notes/`, `assets/aigen/`, `tools/atlaspack/`. Your plan's
   write-lists must never touch the sibling lane.
3. **Route wins.** Verified repo/engine behavior beats the bible and beats both
   prior drafts. When you assert a fact, state the source you verified it
   against, or tag it ⟨UNVERIFIED⟩.
4. **No asset claims.** Anything you output is a **proposal/draft** until it is
   loaded by the engine and passes QA. Never write "passed", "shipped",
   "blocking-resolved" unless a file-log or a test run proves it.
5. **Confess what you can't do.** If you cannot read the live repo, take
   captures, or open the GitHub UI, say so. Do not fabricate the unavailable
   inputs; flags will be marked depending on real access.
6. **Provenance guardrail.** Any image prototype you produce (only if the plan
   calls for one) must record model/date/post in `assets/LICENSES.md` —
   provider-hidden is acceptable **if labelled "undisclosed"**, never omitted.

---

## 3. Ingest list — read first (in this order)

1. `AGENTS.md`, `docs/01-research.md`, `docs/02-gdd.md` (project truth)
2. `docs/art/` — `00-VERIFY.md` (verified-facts ledger + T-ART cards),
   `01-FLAGS.md`, and the five production specs
   (terrain / mobs / npcs+players / items+icons / vfx)
3. `docs/research-notes/` — dossier index, style-tile README, the readability
   rulebook, and the per-ancestor dossiers
4. `docs/prompts/asset-research-bible.md` — the tiered source spec (be aware:
   the version in the repo may still carry superseded numbers — e.g. night
   operator, sheet caps — the ledger is the corrected truth)
5. `docs/tasks/art-backlog.md` — the T-ART-01..11 engine gap cards (they define
   what you can't yet consume — and what you can burn down)
6. `tools/atlaspack/` — the actual pipeline: `bhpix.py`, `make_style_tile.py`
7. `assets/aigen/**` — everything already briefed, their `BRIEF.md`s
8. Repo state: `git log`, `git status` (to detect sibling touches)

If any item cannot be read, list it in the report under "assumptions / unverified".

---

## 4. Step 1 — Draw the true dependency graph

Produce (in the output file, as an ASCII DAG):

- The art pipeline as blocks: B0 gate → B1 terrain → B2 mine/crypt → B3 mobs →
  B4 boss → B5 NPC/portraits → B6 player paper-dolls → B7 items/icons/UI →
  B8 VFX/gore → B9 full-QA sweep.
- **Gates and edges:** which blocks depend on (a) the B0 sign-off,
  (b) T-ART engine cards (and which ones), (c) pending director decisions,
  (d) sibling hand-offs (e.g. map data for the terrain manifests),
  (e) nothing — fully yours.
- For each node: **state** (actionable / partially blocked / blocked), **why**,
  and **what unlocks it**.

The output must answer: *"If I do nothing else, the first N evenings are spent
waiting on…"* — that waiting is precisely the space this brief asks you to fill.

---

## 5. Step 2 — Brainstorm the whole candidate space, then cull

Generate every "could produce next" you can find — a wall of post-its — then
score each candidate on the dimensions in §6. Do not filter while brainstorming,
and do not limit yourself to the list below; extend it with your own craft
knowledge.

| # | Candidate | Filing path (your lane only) | Directly unblocks | Collision risk |
|---|---|---|---|---|
| A1 | Pre-rolled **prompt package per remaining asset**: global prefix + subject + palette line + negative + named-dirs expansion + hand-fix list | `assets/aigen/**/prompt.md` (drafts) | Removes on-the-fly design time from B1–B8 | None |
| A2 | **Atlas JSON sidecars** drafted per sheet (stacked-layout geometry, offsetY blocks, byte budgets) — labelled UNVALIDATED until loader truth is confirmed | `assets/aigen/**/atlas.draft.json` | Removes the ATLAS_JSON wait from every block | None (engine read-only for you) |
| A3 | **Palette family matrix**: master library ramps interleaved with each family ramp, incl. the era-violet swap diagram | `assets/aigen/palettes/` | Feeds every later pass + LICENSES | None |
| A4 | Red-caps **bitmap callout font** prototype (7px cap, R7 glyph grid) | `docs/research-notes/style-tile/export/` | B8 callout system, UI mockup | Low |
| A5 | Terrain **transition pair library** pre-drafts (≥3 variants per adjacent material pair, per locked rule S-2) | `assets/aigen/terrain/` (draft) | De-risks the hardest part of B1 | Low |
| A6 | **15-pile crowd test** composition template (entity placement, layer order, checklist) without touching engine | `docs/art/qa/` | Pre-arms B9 | Sibling may instrument engine — mark as proposal |
| A7 | **Paper-doll layer master** split (base body vs gear slots, per class/sex, dir×8) | `docs/art/` | B6 — the long pole — starts its pre-work now | None |
| A8 | **Ancestor claim re-audit**: the ⟨P⟩ rows in the dossiers — confirm or kill each against captured evidence present in the repo | `docs/research-notes/…` | Removes dead-fact debt | Low |
| A9 | **UI/icon style pre-spec** drafted both ways (gothic-iron vs parchment choices pending chrome decisions) | `docs/art/` | B7 warm-up | None |
| A10 | Boss "player visibility through the pile" **occupancy study** (composition thumbnails, no rig art) | `docs/art/` | Protects the Gravemother fight | None |
| A11 | **Per-map art manifest** against the actual `.bhmap` legends (you READ maps only; sibling keeps writing them) | `assets/aigen/…/MAPS.md` | Pre-plans B1/B2 tile identities | Read-only — safe |

Add your own rows: fill the table with every idea you generate beyond these.
---

## 6. Step 3 — Score, then triage "now / warm / blocked"

Per candidate, fill this matrix:

| Criterion | Scale | Meaning |
|---|---|---|
| Director-free | yes / partial / no | blocked on a decision you don't hold |
| Engine-touched | yes / partial / no | needs a T-ART consensus first |
| Sibling-overlap | none / 1-path / real | touches the sibling's write-lane |
| Value during B0 wait | free / partial / full | earns its keep while the gate is open |
| Rework risk | none / low / high | if a pending decision flips, this turns to scrap |
| Leverage | 0–2 | "lift": unblocks a FUTURE block, not just this one |

Classification rules (apply in order):

1. **READY** — Director-free, engine-touched=no, sibling-overlap=none, and
   rework-risk=none/low. These are the "do this in the next 24–48h" list;
   rework-resistant by construction.
2. **WARM** — passes most of the above but depends on one pending decision you
   can see coming (zoom cell, ravager rim, terrain floor). Draft it as
   **both-options**, hold final consolidation until the sign-off.
3. **BLOCKED** — needs the B0 signature, an engine card, a sibling hand-off, or
   a director decision. Do **not** touch; only describe what would unlock it.

Then write the **triage table** into the deliverable: READY / WARM / BLOCKED,
each row with the reason and the one unlock condition.

---

## 7. Step 4 — Deliver one file: `docs/art/PARALLEL-ROADMAP.md`

### Required skeleton (a document, not art)

1. **Status header** — "no files written / N drafts written" + the write-list.
2. **Dependency graph** (ASCII).
3. **Candidate matrix** (the scored table from §5–§6).
4. **Recommendation** — the three items you would start this evening, the
   second-half plan while the gate is open, and what changes the moment the
   gate flips.
5. **Decision-needs table** — each open gate with: what it is, who owns it
   (director / sibling / engine card), what honors are at risk, and the
   "if the director answers X, the plan does Y" consequence line.
6. **Write-lane table** — files you will create/modify (all in your lane) and
   the no-touch list (sibling lane) so collisions are zero.
7. **Provenance + QA status** — every generated prototype's model/date/post
   recorded or explicitly "undisclosed"; every draft marked UNVALIDATED.
8. **Definition of done for THIS evaluation**:
   - a reviewer can act from the page alone ("do X this week, don't do Y
     while the gate is open");
   - every fact carries its verification source or ⟨UNVERIFIED⟩;
   - zero writes in the sibling lane;
   - no volume art generated (any prototype lives in
     `docs/research-notes/style-tile/export/` with provenance).

---

## 8. Never-do list (binding)

- Never touch `assets/final/`.
- Never claim a screenshot/dossier insight you did not gather from real captures
  or verified files.
- Never describe the sibling's work from guesses — only from commits, files, or
  the coordination contract.
- Never report "passed" / "shipped" / "blocking resolved" without a test log or
  file proof.
- Never commit or merge anything in this eval without explicit instruction.

---

## 9. The director's acceptance test

After reading `docs/art/PARALLEL-ROADMAP.md`, the director should be able, in
five minutes, to:

- pick three items from your READY list (or explain to you why they disagree);
- know exactly which outputs wait on the B0 sign-off and why;
- verify zero sibling-lane collisions from the lane table;
- answer the decision-needs rows cleanly, or mark them TBD explicitly.

That is the bar. Deliver the roadmap, not more art.