# 0126 — B7 skill icons: key found, 9 gens, hotbar wired

## Key discovery

`.env` carries `POLINATION_API_KEY` (single L — the MCP server's
`POLLINATIONS_API_KEY` env name never matched it, which is why earlier
sessions reported "no key"). 35 chars, validated read-only against
`GET /v1/models`, then used via `Authorization: Bearer` header auth from a
sourced shell (the key never touches logs, docs, or committed files).
Gateway returns `b64_json` (JPEG) — decode + convert in post.

## Batch (9/10 session cap, flux.1-schnell, 256×256, 1 held in reserve)

Probe ch1 (cleaver) accepted first: single glyph, keyable green, no text.
Batch ch2–ch8 + ch10 saved to `assets/aigen/icons/skills/raw/` (seeds in
`prompt.md` §Runs). Deviations handled in post, zero regens: drop shadows
throughout, ch4 dark-mottled bg, ch7 white bg (adaptive border-median key +
1px MinFilter fringe erode), ch3 micro-text (gone at 32px), ground discs
kept as icon bases (read correctly at icon size).

## Post (brief-compliant, inline)

NEAREST fit to 28px on a 32 cell, MEDIANCUT ≤32 (final sheet: 31 colours),
pipeline `#151013` plate + 2px family bevel composited (not generated),
NEAREST 24px sheet + palette strip, greyscale silhouette check at 24px
(distinct per family; ch2/ch7 palms differ by motes-vs-ring). Sheet
`skill_icons.png` + JSON cell map keyed by channel.

## Wiring + proof

`Game::ensureSkillIcons` (lazy; missing files → placeholder plates, never
holes). Hotbar draws real icons + outlined key numerals + lock dim.
`docs/research-notes/qa/tart15_hotbar_icons.png` (Cultist L1: 6 icons +
Resurrect L20 lock). LICENSES rows appended with the model disclosed
(prior rows say "undisclosed" — Arena hid it; the gateway names it).

## Owed

Item/UI icon batch (next session: 1 held gen + fresh cap of 10),
vendor-panel plates, 2x hand redraws (programmatic NEAREST stands in),
cooldown sweep (needs cooldowns on the wire). Suite + replay unchanged
(396/396 + 387/387, t159f1 mm=0) — render + art only.
