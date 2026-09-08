# Bible-vs-code verification ledger (2026-09-07)

Bible §5 says "verify against code, not memory". This is the ledger. Each row
is a claim from `docs/prompts/asset-research-bible.md` checked against the
tree at `a262909`. **Verified** = matches code · **Amended** = code says
otherwise, spec updated below · **Missing** = code has no such thing yet, art
must not assume it.

| # | Bible claim | Code | Verdict |
|---|---|---|---|
| 1 | Ground tile 64×32, 2:1 diamond | `sim::Map::tileW/tileH = 64/32`; `iso::tileToWorld` | **Verified** |
| 2 | Character cell 32×48, feet y=42, alpha-70 ellipse | `placeholder.cpp` kFw/kFh 32/48, `feet = oy+42`, `drawEllipse(... Color{0,0,0,70})`; draw origin `Vector2{w/2, 42}` in `game.cpp:730,765` | **Verified** |
| 3 | 8 dirs, order E,SE,S,SW,W,NW,N,NE | `grid.cpp kDx={1,1,0,-1,-1,-1,0,1} kDy={0,1,1,1,0,-1,-1,-1}` | **Verified** (dir 0 = +x = screen right-down-ish: iso E) |
| 4 | Atlas JSON schema v1 `{"anims":{name:{frameW,frameH,frames,dirs,fps,offsetX,offsetY}}}` | `atlas.cpp loadAtlas` — `dirs` must be 8, bounds `ox+frames*fw ≤ tex.w`, `oy+8*fh ≤ tex.h`; **one PNG, per-anim offset blocks** | **Verified**; `bhpix.pack_atlas` emits exactly this |
| 5 | Point filtering | `SetTextureFilter(POINT)` only in `placeholder.cpp`; `loadAtlas` does **not** set it | **Amended**: loader must call `SetTextureFilter(tex, TEXTURE_FILTER_POINT)` — 1-line engine card (T-ART-01) |
| 6 | Integer zoom 1×/1.5×/2× only | `engine/render/camera_rig.h`: Z toggles 1↔2, wheel steps **0.25 to max 2.5** | **Amended**: art is validated at 1/1.5/2; wheel zoom needs snapping to {1,1.5,2} (T-ART-02) or accept shimmer |
| 7 | Night = multiply + additive light mask, alpha ≤ ~65 % | `daynight.cpp`: normal-blend fullscreen rect, keys peak **(18,22,70,150)** = 59 % at 04:00; drawn after `EndMode2D` and *before* `drawChat/drawHud` (`game.cpp:1298–1303`) — HUD stays clean, but **world-space floaters/callouts are under the tint**; **no light mask exists** | **Amended**: cap already satisfied; light pools are *painted ground decals* until an engine light mask ships (T-ART-03); callouts need the R-TEXT night plate because they are tinted. QA uses the real overlay |
| 8 | Player anim set idle1/walk6/attack3/cast4/hurt2/die4/gib3 | engine only plays `"walk"` and `"idle"` (`animFrame(... moving ? "walk":"idle")`) | **Missing**: attack/cast/hurt/die/gib need a client anim-state hook (T-ART-04). Sheets ship the frames anyway |
| 9 | Mob roster 1001–1010 | `mobs.h kMobs` 10 rows exactly as bible table (names, levels) | **Verified** |
| 10 | Mob sprite selection by wireKind | `world.cpp`: `wireKind = 1-based index into kMobs`; client draws **every** entity with `heroAtlas_` | **Missing**: per-kind atlas table on client (T-ART-05). Delivery naming `mobs/<id>_<slug>/` is the intended lookup key |
| 11 | Furniture kinds 64 vendor / 65 anvil / 66 bounty | `wirekind.h` | **Verified**; NPC sprites need kinds 67+ (T-ART-06) — reserve: 67 bonesmith_twins, 68 confessor, 69 fence, 70 guard_ashen, 71 guard_synod, 72 registrar, 73 steward |
| 12 | Karma bands lawful/neutral/chaotic | `karmaBandOf`: <0 → 2 chaotic, >500 → 0 lawful, else 1; client tints `(170,200,255)/(190,190,200)/(235,60,50)` | **Verified**; bible's "lawful white" is actually **pale blue** in code — art uses code's colours |
| 13 | Party names green | not implemented (party frame is a panel; no overhead tint) | **Missing** (T-ART-07); rulebook defines priority chaotic-red > party-green |
| 14 | Callout kinds | T-066: 0 miss,1 hit,2 crit,3 SLAIN,4 xp,5 Power-Swing,6 anvil,7 aura proc,8 mend,9 BLOOD BOLT,10 BLESS,11 IRONSKIN,12 party xp,13 CHORUS,14 HASTE | **Verified**; §12 callout font replaces `DrawText` default |
| 15 | Items 2001–4005 + affixes | `items.h` 10 rows + `kAffixNames` Whet/Warding/Leech | **Verified** |
| 16 | Blackiron Ore, fodder, gold icons | no item rows yet (refine T-060 uses gold + durability; ore is GDD) | **Missing** rows; icons designed now, ids reserved 5001 ore, 5101–5103 fodder tiers |
| 17 | Aura tiers I–V | `auras.h` 5 tiers, parts 4001/4002/4004/4005 | **Verified** |
| 18 | Skill hotbar 1–8 | `kits.h` channels: 1 Power Swing, 2 Mend, 3 Bless, 4 Ironskin, 5 Firebolt, 6 Chorus, 7 Mass Mend, 8 Haste | **Amended**: only 8 skills exist on the wire; the other 16 GDD skills get icons **designed** now, shipped when channels exist |
| 19 | Maps: town, fields, mine, crypt (+castle) | 5 maps: thornwall(1), fields_overflow(2), thornwall_crypt(3), bonehowl_mine(4), drowned_crypt(5); client `mapFileFor` knows only 1–3 | **Amended**: client needs cases 4/5 (T-ART-08, trivial) |
| 20 | Terrain type ids | std maps: 0 GRASS,1 DIRT,2 WALL(prism),3 WATER,4 WOOD,5 PATH,6 MUD,7 DARKGRASS; thornwall_crypt: 0 STONE,1 FLOOR,2 WALL,3 SLAB,4 BONEPIT,5 CANDLE | **Verified**; tileset families map 1:1 onto these ids (see 10-terrain.md) |
| 21 | Blood decal 10 min, corpse stages | client has 0.6 s vestige only | **Missing** decal system (T-ART-09); decals designed |
| 22 | Boss cell 64×64; elite cell **40×60** (`docs/art/20-mobs.md`, true 1.25×) vs **48×64** (T-ART-10 card text) — **D5 ruled 40×60 / anchorY 52** (`B0-GATE-DECISION.md`, 2026-09-08); T-ART-10 card text to be amended by the sibling | `loadAtlas` accepts any frameW/H per anim; draw origin is hard-coded `{w/2, 42}` | **Amended**: feet anchor for taller cells must come from JSON (`anchorY`) — add optional key, default 42 (T-ART-10); elite anchorY = 52 (40×60) or 56 (48×64) |
| 23 | `assets/final/` untouched | nothing written there | **Verified** |
| 24 | "item glows from +5" | T-060 refine exists; no glow render | **Missing** render (T-ART-11); glow overlays designed |
| 25 | Base res 1024×768 | `InitWindow(1024,768)`, `rig_.init(1024,768)` | **Verified** |

## Engine cards implied (for the director to open; art does not block on them)

T-ART-01 point filter in loadAtlas · 02 snap wheel zoom · 03 light-pool decals /
light mask · 04 anim-state hook (attack/cast/hurt/die/gib) · 05 per-wireKind
atlas table · 06 NPC furniture kinds 67+ · 07 party overhead tint · 08 client
map cases 4/5 · 09 ground decal layer (blood, telegraphs, Sanctuary) · 10
`anchorY` in atlas JSON · 11 +5 glow composite.
