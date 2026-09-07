# Art-enablement backlog — T-ART-01..11 (engine tasks that unblock live art)

Source: `docs/art/00-VERIFY.md` (bible-vs-code ledger). These are the engine
gaps between "sheets exist in `assets/aigen/`" and "sheets render in the
client". Art production does NOT block on them (ship the sheets first), but
B3–B6 stay invisible in-client until T-ART-04/05 land. Card format per
`docs/tasks/README.md`: Context / Scope / Acceptance / Tests / Out of scope.
One card = one session = one commit. **Internal to the renderer/client — no
gameplay sim changes, no wire changes (T-ART-04 anim *selection* may read tick
stamps but never affects authority).**

---

## T-ART-01 — Point filtering in `loadAtlas`

- **Context:** `placeholder.cpp` calls `SetTextureFilter(tex, TEXTURE_FILTER_POINT)`
  but `engine/assets/atlas.cpp::loadAtlas` never does → real atlas textures
  would render with smooth filtering at zoom, shattering the era pixel look.
- **Scope:** set `TEXTURE_FILTER_POINT` on the texture after `LoadTexture` in
  `loadAtlas`. Single line.
- **Acceptance:** a packed real-art sheet renders crisp nearest at 1×/1.5×/2×.
- **Tests:** visual smoke on `style_tile_256_day.png` packed via atlaspack;
  existing atlas unit tests still pass.

## T-ART-02 — Snap wheel zoom to {1, 1.5, 2}

- **Context:** `engine/render/camera_rig.h:53–60` wheel does `zoom += wheel*0.25`
  clamped to 2.5 with no snap (F14) → non-integer zoom shimmers pixel art.
- **Scope:** snap wheel zoom to nearest of {1, 1.5, 2}; keep `Z` key toggle
  (1↔2). Art is validated at those three (bible §5 / README style-tile note 2).
- **Acceptance:** all reachable zoom levels ∈ {1, 1.5, 2}; no shimmer at 2×.
- **Tests:** unit for the snap function (boundaries 1.25→1.5, 1.75→2 …); manual
  wheel sweep at 1× and 2×.

## T-ART-03 — Light pools: painted decals now, additive mask later

- **Context:** bible §4.3 / T-062 call for an "additive light mask"; the engine
  has none (00-VERIFY #7). Warm sources are currently just the night overlay.
- **Scope (two phases):** (1) ship painted ground-pool decals under every warm
  source (torch, anvil coals, candle clusters, Lantern Spider, "of the Vigil"
  gear) — art/content, 0 engine change; (2) engine additive light-mask layer
  composited after the night overlay, later.
- **Acceptance:** warm pools visible at night, 1–2 tiles wide, always under
  entities, never over the HUD.
- **Tests:** night screenshot set on Thornwall square + mine.

## T-ART-04 — Client anim-state hook (attack / cast / hurt / die / gib)

- **Context:** client plays only `"walk"`/`"idle"`
  (`game.cpp:724,760` — F4). Attack/cast/hurt/die/gib frames will ship in
  every atlas and never display. **The key task for art visibility.**
- **Scope:** per-entity anim state machine on the client keyed by authoritative
  sim events (swing/cast stamp → attack/cast anim; hit event → hurt; death
  event → die; respawn/despawn → vestige already exists). Durations from tick
  stamps (20 Hz), rendering-only. Contact frame must land on the swing
  resolution tick (bible §5 timing truth).
- **Acceptance:** a B3 mob sheet's attack + die frames play in-client; contact
  frame coincides with the server's hit moment.
- **Tests:** dual-bot duel screenshot series; frame-vs-tick alignment check.
- **Out of scope:** any sim-side change; server remains authoritative.

## T-ART-05 — Per-wireKind atlas table

- **Context:** every entity renders from `heroAtlas_` (F11); `wireKind` carries
  the mob index but the client ignores it.
- **Scope:** `atlasFor(wireKind)` keyed by `mobs/<id>_<slug>/`; fallback to the
  placeholder hero atlas for kinds with no sheet; furniture kinds use their
  own sheets. Missing frame must keep the red-circle marker for QA.
- **Acceptance:** spawning a Hollow Hound (1003) renders the hound sheet, not
  the placeholder hero.
- **Tests:** spawn each kind 1001–1010 + furniture; no red-diamond fallback on
  any shipped row.

## T-ART-06 — NPC furniture kinds 67+

- **Context:** `wirekind.h` ships vendor=64, anvil=65, bounty=66 (V5). NPC
  briefs (`docs/art/30-npcs-players.md`) need: 67 bonesmith_twins, 68
  confessor, 69 fence, 70 guard_ashen, 71 guard_synod, 72 registrar, 73
  steward.
- **Scope:** constants + spawner support + client furniture draw path for
  67–73 (idle 4f × 8 dirs, dialogue portrait channel when a portrait exists).
- **Acceptance:** all 8 NPC kinds spawn and idle in Thornwall/Marrowgate.
- **Tests:** spawn sweep; karma-band vendor refusal still works (Marta sneer
  portrait is a client swap, out of scope).
## T-ART-07 — Party overhead tint

- **Context:** bible §11 + R-TEAM: party members green overhead; client has the
  party frame panel but no overhead tint (F12).
- **Scope:** paint overhead name by priority (R-TEAM):
  chaotic red > enemy-town > party green > neutral.
- **Acceptance:** a 5-bot party reads as a unit at a glance in a mixed pile.
- **Tests:** 5-bot party + strangers screenshot; name tint priority unit check.

## T-ART-08 — Client map cases 4/5

- **Context:** server loads maps 1–5; `mapFileFor` knows 1–3 (F10) → Bonehowl
  Mine and Drowned Crypt art cannot be seen in-client.
- **Scope:** add cases for `bonehowl_mine` (4) and `drowned_crypt` (5). Two lines.
- **Acceptance:** both maps open in-client via portal/zone handoff.
- **Tests:** zone handoff 1→4→5→1, screenshots.

## T-ART-09 — Ground decal layer (blood, telegraphs, Sanctuary)

- **Context:** blood decals (10 min, §4.4), boss telegraphs (1009), Sanctuary /
  Wither circles, shadow-pool decals — all need a render-to-texture decal
  layer per map chunk (`docs/03-architecture.md` `gore` row).
- **Scope:** decal surface drawn under entities, layered, with decay timers;
  API for blood (kill pos), 3-stage telegraph, persistent spell circles.
  **Rendering-only, non-wire** (must not affect replay).
- **Acceptance:** blood persists 10 min and sorts under entities; telegraphs
  3-stage; replay stays bit-exact.
- **Tests:** kill-20 decal count/decay; replay round-trip unchanged.

## T-ART-10 — `anchorY` in atlas JSON

- **Context:** draw origin is hard-coded `{src.width*0.5f, 42.0f}`
  (`game.cpp:730,765` — F3/F18). 48×64 elites float ~10px; 64×64 bosses and
  any 32×56 player cell mis-anchor.
- **Scope:** optional `anchorY` key in the anim JSON (default 42), loaded by
  `loadAtlas`, carried per anim, used by the client draw call.
- **Acceptance:** Sepulcher Elite (48×64) and Gravemother (64×64) sit
  feet-on-diamond; 32×56 player cell (if director approves) anchors at 50.
- **Tests:** overlay each cell on the 64×32 diamond grid; verify feet.

## T-ART-11 — Refine +5 glow composite

- **Context:** "item glows from +5" (GDD §7); T-060 refine exists but no glow
  render (00-VERIFY #24).
- **Scope:** additive glow-overlay composite drawn over held/equipped items at
  refine ≥ 5 (overlay sprites ~2 frames, alpha ≤ 90 — see
  `docs/art/40-items-icons.md`); +10 unique silhouette add
  (`docs/art/40-items-icons.md` §10).
- **Acceptance:** +5 blade glows in-hand and in inventory; +10 reads as a
  silhouette change, not a brighter glow.
- **Tests:** anvil-refine a blade to +5/+10, screenshots day/night 1×/2×.