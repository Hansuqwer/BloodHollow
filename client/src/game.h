#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "assets/atlas.h"
#include "bitmap_font.h"  // T-ART-13 red-caps callout font
#include "net_client.h"
#include "render/animstate.h"  // T-ART-04 combat anim hook
#include "render/camera_rig.h"
#include "render/decals.h"  // T-ART-09 ground decal layer (blood/telegraph/circle)
#include "terrain_skin.h"  // T-ART-12 textured ground + skinned prisms
#include "sim/bhmap.h"
#include "sim/journal.h"
#include "synthkit.h"  // T-067 procedural audio kit
#include "sim/walker.h"

namespace bh {

// Rising combat text (misses, damage, crits, kills).
struct Floater {
  float x = 0, y = 0;  // tile-space float
  std::string text{};
  unsigned char r = 255, g = 60, b = 60;
  double at = 0;
};

// T-066: petrify-fade vestige — a dead/departed entity's last pose, 0.6s grey fade.
struct Vestige {
  float x = 0, y = 0;  // tile-space float (last known)
  std::uint8_t kind = 0;
  double at = 0;
};

// Presentation wrapper: latest authoritative state + interpolation history.
struct RenderEnt {
  NetEntSnapshot snap{};
  float fromX = 0, fromY = 0;  // render-interp origin (tile-space float)
  double stamp = 0;            // GetTime() when the latest delta was applied
  bool hasInterp = false;
  // T-ART-04 combat anim hook (render-side only, never authoritative).
  EntAnimState animState = EntAnimState::kNone;
  double animStateAt = 0.0;    // GetTime() the state started (frame 0)
  double animStateUntil = 0.0;  // GetTime() a transient state expires
};

class Game {
 public:
  explicit Game(sim::Map map);

  // online mode (optional): NetClient is owned by main and outlives Game.
  void setOnline(NetClient* nc) { net_ = nc; }
  void bakeAudio();  // T-067: one-shot synth, no-op when audio device is down
  void frameBegin();  // per rendered frame: network poll + online input

  // ---- offline command funnel (single body used by input/scripts/replay) ----
  void commandPath(sim::TilePos goal);
  void commandStep(int dx, int dy);

  void attachJournalRecorder(sim::JournalWriter* jw) { rec_ = jw; }
  void attachJournalReplay(const sim::Journal* j) {
    replay_ = j;
    replayIdx_ = 0;
  }
  void setHashSink(std::vector<std::pair<sim::Tick, std::uint64_t>>* sink) { hashSink_ = sink; }

  void tick();  // one fixed 20 Hz sim step (offline sims; online = clock/anims)
  void render(double interpAlpha);

  sim::Tick tickCount() const { return tick_; }
  bool online() const { return net_ != nullptr; }
  bool onlineWelcomed() const { return net_ != nullptr && net_->welcomed; }

  void debugSetHour(float h);

  // T-ART-12/T-164 dev captures (render-only, no sim): --zoom forces the
  // camera zoom every frame, --flat-ground forces the legacy flat diamonds,
  // --lamp draws one shipped-style warm pool at the hero (same gradient call
  // and radius law as the online carried-light path, for night matrices).
  void debugSetZoom(float z) { debugZoom_ = z; }
  void debugSetFlatGround(bool f) { skin_.setFlatForced(f); }
  void debugSetLamp(int radiusTiles) { lampRadius_ = radiusTiles; }
  void debugSetFlatFont(bool f) {  // --flat-font: legacy DrawText (before shots)
    if (f) fontsTried_ = true;
  }
  // T-ART-14b captures: --cam TX,TY pins the camera target (render-only).
  void debugSetCam(float tx, float ty) {
    debugCam_ = true;
    debugCamTx_ = tx;
    debugCamTy_ = ty;
  }
  // T-ART-14 dev visual: --vfx-test loads the dirs:1 fixture strip and plays
  // it looping at the hero (proves load+play; real strips wire to events).
  void debugSetVfxTest(bool v) { vfxTest_ = v; }
  // T-ART-13 dev captures: --create 1M preselects the T-167 creation answer
  // so a fresh account lands in-world unattended (class 1..3, sex 1..2).
  void debugSetCreate(std::uint8_t c, std::uint8_t s) {
    createClass_ = c;
    createSex_ = s;
    debugAutoCreate_ = (c != 0 && s != 0);
  }
  // T-ART-12: offline --map loads by path; the skin needs the zone id.
  static std::uint16_t mapIdForFile(const std::string& path);
  void setLoadedMapId(std::uint16_t id) { loadedMapId = id; }

 private:
  sim::TilePos findSpawn() const;
  void handleInput();        // offline input (journaled)
  void handleInputOnline();  // online input (sends intents)
  bool chatTyping();         // chat focus key capture; true => input consumed
  void applyNetState();
  void stepMovement();       // offline walker sim
  void applyJournalEvents();
  void drawGround();
  void drawHero();           // offline hero
  void drawEntitiesOnline();
  void drawRemoteEnt(const RenderEnt& e, bool isOwn);
  static void setAnimState(RenderEnt& e, EntAnimState st, double now);
  bool isPartyMember(std::uint32_t id) const;  // T-ART-07 overhead tint
  void drawPathPreview() const;
  void drawCommandMarker() const;
  void drawHud() const;
  void drawChat() const;
  void drawFloaters();
  void ensureFonts();  // T-ART-13: lazy bitmap fonts (GL must be up first)
  // T-ART-13 bitmap name tag: bitmap small caps when loaded (legacy DrawText
  // fallback), degrading to the karma badge in piles of 4+ (R-TEXT-2).
  void drawNameTag(Vector2 w, const std::string& name, Color nc, int yOff,
                   std::uint32_t id, bool canDegrade);
  void noteVestiges();   // T-066: harvest despawnedIds into vestige queue
  void drawVestiges();   // T-066: petrify-fade silhouettes under the world
  void noteDecals();     // T-ART-09: harvest kill pulses into blood decals
  void drawDecals();     // T-ART-09: decal surface, under entities, y-sorted
  // T-ART-14b: event-fed VFX (tier-0 procedural strips; art lane redraws).
  struct VfxPlay {
    std::string strip;  // dir under assets/aigen/vfx/
    float x = 0, y = 0;  // tile-space float
    double at = 0;       // GetTime() the play started
    float dur = 0.5f;
  };
  std::unordered_map<std::string, Atlas> vfxAtlases_;  // lazy, cached
  std::deque<VfxPlay> vfxPlays_;  // capped 16, oldest dropped
  const Atlas* vfxStrip(const std::string& name);
  void noteVfx();      // CombatPulse -> strip plays (render-only)
  void drawVfxPlays();  // world-space, over entities, under floaters
  void addTelegraph(float x, float y, int stage);  // T-ART-09: boss API (1-3)
  void addCircle(float x, float y);                // T-ART-09: spell-circle API
  void drawStatPanel() const;
  void drawHotbar() const;  // T-ART-15: skill slots (keys+locks; icons pending)
  void drawPartyFrame() const;  // T-052 top-left HB-style party list
  std::uint32_t chanTarget() const;  // T-054: party-frame pick else self
  void drawInventoryPanel() const;
  void drawVendorPanel() const;
  void drawDeathOverlay() const;
  bool vendorNear() const;
  bool fenceNear() const;  // T-069: Sable's crate section of the panel
  bool anvilNear() const;
  void drawAnvilPanel() const;
  void drawTradeBanner() const;
  void drawSiegePanel() const;   // T-151 parchment siege readout
  void drawPledgePanel() const;  // T-151 pledge roster / emblem / vault
  void drawHelpPanel() const;    // T-169: /help verb + hotkey reference
  void drawCreatePanel() const;  // T-167: pre-world class+sex picker
  bool isPledgeMemberName(const std::string& n) const;
  Color terrainColor(std::uint16_t type) const;
  float gameHour() const;
  Vector2 entRenderPos(const RenderEnt& e) const;
  // T-071/T-164: one warm pool (shared by the online carried-light loop and
  // the offline --lamp dev visual — same call, same radius law, no fork).
  void drawLightPool(Vector2 worldPos, int radiusTiles) const;

  sim::Map map_;
  std::uint16_t loadedMapId = 1;
  TerrainSkin skin_;  // T-ART-12: baked textured ground + prism skins
  BitmapFont calloutFont_;  // T-ART-13: 7x11 red-caps (floaters)
  BitmapFont nameFont_;     // T-ART-13: 5x7 small caps (nameplates)
  bool fontsTried_ = false;
  // R-TEXT-2: screen-space name-tag anchors for the pile test, rebuilt in
  // drawEntitiesOnline before any tag draws.
  std::unordered_map<std::uint32_t, int> namePileCounts_;
  bool zoneReload(std::uint16_t mapId);   // T-037: hot-swap on Welcome reuse
  static const char* mapFileFor(std::uint16_t mapId);
  sim::CostGrid grid_{};
  CameraRig rig_{};
  Atlas heroAtlas_{};
  // T-ART-05: per-wireKind atlas table (lazy, cached). Mobs resolve to
  // assets/aigen/mobs/<id>_<slug>/ sheets; everything without a shipped
  // sheet (players, 1011 Guard, furniture) falls back to the hero atlas —
  // furniture never reaches it (placeholder branch draws first).
  std::unordered_map<std::uint8_t, Atlas> mobAtlases_{};
  const Atlas& atlasFor(std::uint8_t kind);
  // T-142: per-(class,sex) player atlas table (lazy, cached). Ravager m/f
  // ship today; Gravecaller/Cultist resolve once their sheets land; sex 0
  // or unknown class falls back to hero (T-142b captures sex at creation).
  std::unordered_map<std::uint16_t, Atlas> playerAtlases_{};
  const Atlas& atlasForPlayer(std::uint8_t classId, std::uint8_t sex);
  Atlas testVfx_{};      // T-ART-14 fixture strip (dev visual only)
  bool vfxTest_ = false;
  void drawVfxTest();  // T-ART-14: looping fixture playback at the hero
  // T-ART-15 skill icons (aigen sheet; placeholder plates until loaded).
  mutable Texture2D skillIcons_{};
  mutable std::unordered_map<std::uint8_t, Rectangle> skillIconRects_{};
  mutable bool skillIconsTried_ = false;
  void ensureSkillIcons() const;
  // T-ART-15 item icons (aigen sheet keyed by itemId; rarity plate fallback).
  mutable Texture2D itemIcons_{};
  mutable std::unordered_map<std::uint32_t, Rectangle> itemIconRects_{};
  mutable bool itemIconsTried_ = false;
  void ensureItemIcons() const;
  // Draws the icon square at (x,y) (opaque plate baked in); false = caller
  // keeps the rarity-plate/text fallback.
  bool drawItemIcon(std::uint32_t itemId, int x, int y, int size) const;

  // offline sim
  sim::Walker walker_{};
  std::deque<sim::TilePos> path_{};

  // online state (presentation-side)
  std::unordered_map<std::uint32_t, RenderEnt> rents_{};

  sim::Tick tick_ = 0;
  double animT_ = 0.0;
  bool showGrid_ = false;
  bool showPath_ = true;
  float debugHourOffset_ = 0.0f;
  float debugZoom_ = -1.0f;   // >0 forces camera zoom (dev captures)
  int lampRadius_ = 0;        // >0 draws one warm pool at the hero (dev)
  bool debugCam_ = false;     // --cam: pin camera target (dev captures)
  float debugCamTx_ = 0, debugCamTy_ = 0;

  // chat + UX state
  bool chatFocus_ = false;
  std::string chatBuf_{};
  std::deque<ChatLine> chatLog_{};
  std::deque<Floater> floaters_{};
  std::deque<Vestige> vestiges_{};  // T-066: capped 16
  std::deque<Decal> decals_{};      // T-ART-09: ground decals, capped kDecalCap
  SynthKit kit_{};  // T-067: bake at boot (no assets)
  void playCallout(std::uint8_t kind);  // T-067: map floater kinds to sounds
  std::uint32_t targetId_ = 0;
  sim::TilePos cmdMarker_{-1, -1};
  double cmdMarkerAt_ = -1.0;
  double lastStepSent_ = -1.0;
  double lastPing_ = 0.0;
  sim::Tick onlineBaseTick_ = 0;  // tick_ at welcome (for online clock estimate)
  bool showInv_ = false;
  int invHover_ = -1;
  bool showVendor_ = false;
  bool showAnvil_ = false;
  bool showHelp_ = false;  // T-169: /help overlay
  // T-167 creation panel selection (1..3 kit, 1..2 sex, 0 = unpicked)
  std::uint8_t createClass_ = 0, createSex_ = 0;
  bool debugAutoCreate_ = false;  // --create: answer once, dev captures only

  NetClient* net_ = nullptr;
  sim::JournalWriter* rec_ = nullptr;
  const sim::Journal* replay_ = nullptr;
  size_t replayIdx_ = 0;
  std::vector<std::pair<sim::Tick, std::uint64_t>>* hashSink_ = nullptr;
  bool inTick_ = false;
};

}  // namespace bh
