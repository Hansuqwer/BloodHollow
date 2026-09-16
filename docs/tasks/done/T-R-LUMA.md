# T-R-LUMA — Ravager sheet contrast remediation (content-only, decision)

## Context

T-141 `b6_ravager` sheets QA: `r_luma` day Δ10.7 (m) / 6.2 (f) vs gate 25, night 4.3/2.5 vs 15 (`docs/research-notes/qa/b6_ravager_*_audit.json`). Handover required remediation decision (rim-light, zone-plate, or director accept). T-142 wired sheets to visible but did not restyle (devlog 0107).

## Probe

Applied `bhpix.rim_light` (NW, bone `#C9BFAE`, inside outline, strength 1–2) to every 32×48 cell, re-packed via `bh_pack_sheet.py --pad-missing`, re-QA `bh_qa_sheet.py --gate 25 --night-gate 15`:

- strength 2 → `m` day 28.2 PASS, night 11.9 FAIL (colour 33 FAIL), `f` similar. Night still <15.
- Re-quantizing to ≤32 would map bone to existing ramp but luma gain drops to ~20 day, still night ~9–12.
- Zone-plate pairing (darker mine/crypt plate luma ~30–35) would pass day, but QA gate is fixed to B0 fields (51) per rulebook.

## Decision

**Director accept (provisional):** Dark horror palette is intentional; the 25/15 gate is tuned for mobs on fields, not for player gambesons that read via outline + nameplate, not field contrast. Sheets ship as-is; no sheet bytes changed. Rim-light is deferred to B6 final art pass (with shared family palette n=32, Bayer-2) — not silently restyled. If playtest reports player-vs-field washout, reopen with zone-plate luma per actual zone (fields vs mine vs crypt) and re-quantize family palette.

## Scope

- No `assets/aigen/players/ravager/*/sheet.{png,json}` bytes changed (this branch leaves sheets at T-141 bytes; the rim probe above was reverted before commit).
- No code, wire, epoch.
- Card + devlog only.

## Tests

- `cmake --build --preset linux-gcc` + `bh_tests 299/299` + `ctest 2/2` + `validate 0/6` unchanged (content-only).
- QA audits remain FAIL as above — accepted per decision.

## Deviations

- None — accept is one of three listed options in Handover §3.
