# T-150 — Client renders zone 6 (Weeping Castle) (P0, audit 1/20)

## Context
Audit finding **B1.2 / P0-1** (`docs/audits/2026-09-16-mvp-playable-audit.md`).
The server loads six zones (`server/src/main.cpp:1278,1628`), but the client's
`mapFileFor()` (`client/src/game.cpp:971-978`) has cases 2–5 and
`default: "assets/maps/thornwall.bhmap"` — **no case 6**. A human who walks the
castle portal is drawn Thornwall geometry while the sim runs the castle: gates,
heartstone, steward and siege choreography are all positioned against a map the
player cannot see. Friday-Night leg 3 cannot pass.

## Scope
- `client/src/game.cpp`: add `case 6: return "assets/maps/weeping_castle.bhmap";`.
- `tests/test_clientlaw.cpp`: pin `mapFileFor(6)` (and 1–5) so the table cannot
  lose a zone again; pin that every zone the server boots has a client case
  (derive the list from one shared place if cheap — do not invent a new header).
- Verify the castle's furniture kinds (73 steward, 75 gates, 76 heartstone) have
  a draw path in `atlasFor`/the furniture branch; if any falls back to the hero
  placeholder, say so in the PR (T-151 owns the HUD, not the sprite).
- OUT: any wire change, any siege HUD, any epoch/schema change.

## Acceptance criteria
1. `ctest` green with the new pins; suite count increases.
2. Client connected to a server with zone 6 loaded: walking the castle portal
   renders `weeping_castle.bhmap` (screenshot in the PR; headless `--shot`
   acceptable plus one graphical shot).
3. `grep -n "case 6" client/src/game.cpp` shows the map case (not only audio/
   terrainColor).
4. No journal/epoch/schema/wire diff (`git diff --stat` shows client + tests only).

## Tests required
`test_clientlaw` map-table pin; screenshot evidence; existing suite green.

## Evidence owed at merge
Suite count + ctest line, screenshot(s), devlog id, board row.
