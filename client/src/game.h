#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "assets/atlas.h"
#include "net_client.h"
#include "render/animstate.h"  // T-ART-04 combat anim hook
#include "render/camera_rig.h"
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
  void noteVestiges();   // T-066: harvest despawnedIds into vestige queue
  void drawVestiges();   // T-066: petrify-fade silhouettes under the world
  void drawStatPanel() const;
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
  Color terrainColor(std::uint16_t type) const;
  float gameHour() const;
  Vector2 entRenderPos(const RenderEnt& e) const;

  sim::Map map_;
  std::uint16_t loadedMapId = 1;
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

  // chat + UX state
  bool chatFocus_ = false;
  std::string chatBuf_{};
  std::deque<ChatLine> chatLog_{};
  std::deque<Floater> floaters_{};
  std::deque<Vestige> vestiges_{};  // T-066: capped 16
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

  NetClient* net_ = nullptr;
  sim::JournalWriter* rec_ = nullptr;
  const sim::Journal* replay_ = nullptr;
  size_t replayIdx_ = 0;
  std::vector<std::pair<sim::Tick, std::uint64_t>>* hashSink_ = nullptr;
  bool inTick_ = false;
};

}  // namespace bh
