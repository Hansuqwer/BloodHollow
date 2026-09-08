# Art production docs — index (B0 drop, 2026-09-07)

Executes `docs/prompts/asset-research-bible.md` step 0–1 (research + style
lock) and pre-writes every design brief so B1–B8 are generation + cleanup
work only. **Nothing here is shipped art**; `assets/final/` untouched.

| File | What it is | Bible § |
|---|---|---|
| [`B0-GATE-DECISION.md`](B0-GATE-DECISION.md) | **binding**: gate approval + D1–D12 rulings (director delegate, 2026-09-08) | §0 |
| [`PARALLEL-ROADMAP.md`](PARALLEL-ROADMAP.md) | B0.5 plan; its §5 decision table is resolved by the decision doc | — |
| [`batches/`](batches/) | production batch records (summary · QA record · LICENSES · open decisions · hand-off), one file per batch; **B3 first pass (rev 2)**, **B4**, **B1 (terrain town + fields)**, and **B2 (terrain mine + both crypts)** landed offline 2026-09-08 | §15–17 |
| [`00-VERIFY.md`](00-VERIFY.md) | 25-row ledger: bible claim vs. code at `a262909`, verdict, implied engine cards T-ART-01…11 | §5 |
| [`01-FLAGS.md`](01-FLAGS.md) | contradictions between bible / GDD / research / code — decisions requested, nothing silently fixed | §18 |
| [`10-terrain.md`](10-terrain.md) | tilesets: plate list, **measured** transition pairs per map, furniture, palette anchors, prompts | §6 |
| [`20-mobs.md`](20-mobs.md) | 10 shipped mobs fully briefed + 13-row GDD roster table | §7 |
| [`30-npcs-players.md`](30-npcs-players.md) | 8 NPCs (+ portraits, barks), 3 classes × m/f, paper-doll layers, v0.2 Vampire spine note | §8–9 |
| [`40-items-icons.md`](40-items-icons.md) | weapon silhouettes + refine states, icon language, UI chrome incl. anvil theatre | §10–11 |
| [`50-vfx.md`](50-vfx.md) | 31 VFX with size × frames @ fps and colour family; callout font | §12 |
| `../research-notes/00-dossier-index.md` | five ancestor dossiers + Helbreath readability rulebook + style-tile proof | §3–4 |
| `../../assets/aigen/**/BRIEF.md` | 39 per-folder briefs with the exact prompt skeleton and QA checklist (§17 layout) | §13–17 |
| `../../tools/atlaspack/` | `bhpix.py` pipeline lib + `make_style_tile.py` reference chain | §5, §14 |

## Gate status

- **B0 style tile: APPROVED 2026-09-08** — [`B0-GATE-DECISION.md`](B0-GATE-DECISION.md)
  (gate sign-off + D1–D12 rulings; carried fix: plate floor clamp ≥ 24 at B1).
- **B1–B9:** production open; sequencing B3 → B4 → B1 → B2 → B5 → B6 → B7 → B8,
  offline QA sweep alongside. **B2 closed offline with rc 0 on 2026-09-08; assets remain
  UNVALIDATED in engine until T-ART-12.** Batch records under [`batches/`](batches/).

## Batch order after approval (bible §17)

B1 town + fields terrain → B2 mine + crypt → B3 mobs 1001–1005 → B4 1006–1010
+ boss → B5 NPCs → B6 player bases → B7 icons → B8 VFX + callouts → B9 night
+ crowd QA. Each batch: one drop, one `LICENSES.md` block, one QA triptych set
under `docs/research-notes/qa/`.
