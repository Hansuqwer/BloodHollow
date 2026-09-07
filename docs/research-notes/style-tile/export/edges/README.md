# A12 — edge-mask preview (PROVISIONAL, tool output, no art)

`python3 tools/atlaspack/make_edge_preview.py` → this folder. Deterministic
(seed 1999). **No AI generation**: material A is the B0 fields/mud plate
(`../../plate_fields_mud_b0.png`, itself derived from the B0 plates — see
`assets/LICENSES.md`), material B is the client's flat placeholder
`terrainColor(0)` GRASS (64,84,46) from `client/src/game.cpp:660`.

| File | What |
|---|---|
| `edge_masks_board_3x.png` | row 1 the 8 base masks · row 2 the same composited (mud A, flat grass B) · row 3 a 7×5 autotile patch (B island + lone B tile) |
| `mask_edge_{NE,SE,SW,NW}.png` | face bands: B is the tile across that face; depth 0.6 diamond units (≈10 px), ±1 px scanline jitter, 2-px 50 % checker band (value 128) |
| `mask_corner_{N,E,S,W}.png` | vertex caps: B only touches at that point (diagonal neighbour); cap depth 0.5 |
| `edge_masks_audit.json` | per-mask B / band fractions; `face_all_B` = every pixel on the shared face is B (seamless butt joint against the full-B neighbour) — all four **true** |

Geometry source: `engine/render/iso.cpp:7–10` `tileToWorld` (+tx → screen SE,
+ty → screen SW), so the tile-space neighbour offsets are
faces NE (0,−1) · SE (+1,0) · SW (0,+1) · NW (−1,0); points N (−1,−1) ·
E (+1,−1) · S (+1,+1) · W (−1,+1) (`bhpix.EDGE_FACES / EDGE_POINTS`).

## How B1/B2 would use it (proposal — nothing here is a shipped format)

1. Per adjacency pair from `docs/art/10-terrain.md` (22 pairs) the artist starts
   from these 8 masks: `bhpix.blend_edge(A_cut, B_cut, mask)` gives the
   mechanical first cut, then the boundary is **hand-painted** (roots, mortar,
   algae) per bible §6.6 — no soft alpha; the checker band is a starting
   texture, not a blend.
2. Piece format (what T-ART-12 would load): 64×32 RGBA per `(pair, side)`,
   **B opaque where the mask says B, fully transparent where A shows** — an
   alpha-*cut* overlay, drawn over the A base tile. A tile with several B
   neighbours draws faces first, then points; points are skipped when either
   adjoining face is already B (`bhpix.edge_mask_for` encodes this rule; row 3
   of the board is that rule running).
3. Which material bleeds over which is a **priority order per pair**
   (roadmap D12). The masks are symmetric, so the choice costs nothing now, but
   it fixes on which tile the piece lives and therefore the lookup key.

## Not done / not claimed

- No transition *art* exists (A5 is BLOCKED on the B0 gate + D3); flat green is
  a placeholder so only the cut geometry can be judged.
- Engine load/draw of any piece is ⟨UNVERIFIED⟩ — there is no textured-ground
  path in the client (`game.cpp:671–686` flat diamonds; roadmap D6/T-ART-12).
