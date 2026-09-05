#include "game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "assets/placeholder.h"
#include "content/items.h"
#include "content/kits.h"
#include "content/auras.h"
#include "content/wirekind.h"
#include "render/daynight.h"
#include "render/iso.h"
#include "sim/clock.h"
#include "sim/tick.h"

namespace bh {

Game::Game(sim::Map map) : map_(std::move(map)) {
  grid_ = map_.costGrid();
  rig_.init(1024.0f, 768.0f);
  const Vector2 sw = iso::tileToWorld(0, map_.h - 1, map_.tileW, map_.tileH);
  const Vector2 ne = iso::tileToWorld(map_.w - 1, 0, map_.tileW, map_.tileH);
  const Vector2 se = iso::tileToWorld(map_.w - 1, map_.h - 1, map_.tileW, map_.tileH);
  rig_.setBounds(static_cast<float>(sw.x), -static_cast<float>(map_.tileH),
                 static_cast<float>(ne.x), static_cast<float>(se.y) + map_.tileH, 180.0f);
  heroAtlas_ = makeHeroAtlas(Color{74, 58, 84, 255},   // cloak
                             Color{150, 30, 30, 255},   // blood trim
                             Color{214, 198, 190, 255}  // pale skin
  );
  const sim::TilePos s = findSpawn();
  walker_.place(s);
}

sim::TilePos Game::findSpawn() const {
  const int cx = map_.w / 2;
  const int cy = map_.h / 3;
  for (int r = 0; r < 64; ++r) {
    for (int dy = -r; dy <= r; ++dy) {
      for (int dx = -r; dx <= r; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
        if (map_.inBounds(cx + dx, cy + dy) && !map_.isBlocked(cx + dx, cy + dy)) {
          return sim::TilePos{cx + dx, cy + dy};
        }
      }
    }
  }
  return sim::TilePos{1, 1};
}

// ---- offline command funnel ------------------------------------------------

void Game::commandPath(sim::TilePos goal) {
  if (rec_ && rec_->ok()) {
    rec_->event(inTick_ ? tick_ : tick_ + 1, sim::kEventPath, goal.x, goal.y);
  }
  if (!map_.inBounds(goal.x, goal.y) || map_.isBlocked(goal.x, goal.y)) return;
  const sim::TilePos start = walker_.moving ? walker_.target : walker_.tile();
  if (start == goal) {
    path_.clear();
    return;
  }
  const sim::PathResult res = sim::findPath(grid_, start, goal);
  if (res.found) path_.assign(res.tiles.begin(), res.tiles.end());
}

void Game::commandStep(int dx, int dy) {
  if (rec_ && rec_->ok()) {
    rec_->event(inTick_ ? tick_ : tick_ + 1, sim::kEventStep, dx, dy);
  }
  path_.clear();
  const sim::TilePos cur = walker_.tile();
  walker_.beginStep(grid_, sim::TilePos{cur.x + dx, cur.y + dy});
}

// ---- input ------------------------------------------------------------------

bool Game::chatTyping() {
  if (!chatFocus_) return false;
  int c = GetCharPressed();
  while (c > 0) {
    if (c >= 32 && c <= 126 && chatBuf_.size() < 160) {
      chatBuf_.push_back(static_cast<char>(c));
    }
    c = GetCharPressed();
  }
  if (IsKeyPressed(KEY_BACKSPACE) && !chatBuf_.empty()) chatBuf_.pop_back();
  if (IsKeyPressed(KEY_ESCAPE)) {
    chatFocus_ = false;
    chatBuf_.clear();
    return true;
  }
  if (IsKeyPressed(KEY_ENTER)) {
    chatFocus_ = false;
    if (!chatBuf_.empty()) {
      if (net_ != nullptr && net_->state == NetClient::State::kInWorld) {
        net_->sendChat(chatBuf_[0] == '.' ? 0 : 1, chatBuf_.substr(chatBuf_[0] == '.' ? 1 : 0));
      } else {
        chatLog_.push_back(ChatLine{2, "system", "chat needs --server; you're offline."});
        while (chatLog_.size() > 12) chatLog_.pop_front();
      }
    }
    chatBuf_.clear();
  }
  return true;  // consume all other input while typing
}

void Game::handleInput() {
  if (chatTyping()) return;
  if (IsKeyPressed(KEY_ENTER)) {
    chatFocus_ = true;
    return;
  }
  if (IsKeyPressed(KEY_F3)) showGrid_ = !showGrid_;
  if (IsKeyPressed(KEY_F4)) showPath_ = !showPath_;
  if (IsKeyPressed(KEY_H)) debugHourOffset_ += 1.0f;
  if (IsKeyPressed(KEY_N)) debugHourOffset_ -= 1.0f;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    // party frame rows are castable targets (T-052/T-054): click = select
    if (net_->partyId != 0 && !net_->party.empty()) {
      const Vector2 mp = GetMousePosition();
      const float px = 8.0f, py = 110.0f;
      const float ph = 18.0f + net_->party.size() * 24.0f;
      if (mp.x >= px && mp.x <= px + 150 && mp.y >= py && mp.y <= py + ph) {
        const int row = static_cast<int>((mp.y - (py + 18)) / 24.0f);
        if (row >= 0 && static_cast<size_t>(row) < net_->party.size()) {
          targetId_ = net_->party[static_cast<size_t>(row)].entityId;
        }
        return;  // swallow: clicking the frame never walks
      }
    }
    const Vector2 mw = GetScreenToWorld2D(GetMousePosition(), rig_.cam);
    commandPath(iso::worldToTilePos(mw, map_.tileW, map_.tileH));
  }
  const int sx = (IsKeyDown(KEY_D) ? 1 : 0) - (IsKeyDown(KEY_A) ? 1 : 0);
  const int sy = (IsKeyDown(KEY_S) ? 1 : 0) - (IsKeyDown(KEY_W) ? 1 : 0);
  if ((sx != 0 || sy != 0) && !walker_.moving) {
    int dx = sx + sy;
    int dy = sy - sx;
    dx = dx < -1 ? -1 : (dx > 1 ? 1 : dx);
    dy = dy < -1 ? -1 : (dy > 1 ? 1 : dy);
    commandStep(dx, dy);
  }
}

void Game::handleInputOnline() {
  if (chatTyping()) return;
  if (IsKeyPressed(KEY_ENTER)) {
    chatFocus_ = true;
    return;
  }
  if (IsKeyPressed(KEY_F3)) showGrid_ = !showGrid_;
  if (IsKeyPressed(KEY_F4)) showPath_ = !showPath_;
  if (net_->state != NetClient::State::kInWorld) return;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    // inventory panel click routes to use/equip before any world click
    if (showInv_) {
      const Vector2 mp = GetMousePosition();
      const float px = 1024.0f - 250.0f, py = 150.0f;
      if (mp.x >= px && mp.x <= px + 240 && mp.y >= py + 20 && mp.y <= py + 200) {
        const int row = static_cast<int>((mp.y - (py + 24)) / 16.0f);
        if (row >= 0 && static_cast<size_t>(row) < net_->inventory.size()) {
          auto it = net_->inventory.begin();
          std::advance(it, row);
          const content::ItemDef* d = content::findItem(it->second.itemId);
          if (d != nullptr) {
            if (net_->tradeWithId != 0) {
              net_->sendTradeOfferItem(it->second.itemId, 1);  // click offers one
            } else {
              if (d->slot == 2) net_->sendUseItem(it->first);
              if (d->slot <= 1) net_->sendToggleEquip(it->first);
            }
          }
        }
        return;  // swallow: don't walk through the panel
      }
    }
    const Vector2 mw = GetScreenToWorld2D(GetMousePosition(), rig_.cam);
    const Vector2 clickedTile = iso::worldToTile(mw, map_.tileW, map_.tileH);
    // entity under cursor? nearest within 0.9 tiles (mobs preferred)
    std::uint32_t pick = 0;
    float best = 0.9f;
    for (const auto& kv : rents_) {
      if (kv.first == net_->ownId) continue;
      const Vector2 rp = entRenderPos(kv.second);
      const float d = std::hypot(rp.x - clickedTile.x, rp.y - clickedTile.y);
      const bool mob = kv.second.snap.kind != 0;
      if (d < best || (mob && d < best + 0.25f)) {
        if (kv.second.snap.hp > 0) {
          best = d;
          pick = kv.first;
        }
      }
    }
    if (pick != 0) {
      targetId_ = pick;
      net_->sendAttack(pick);
      cmdMarker_ = sim::TilePos{-1, -1};  // no ground marker on attack
    } else {
      targetId_ = 0;
      const sim::TilePos goal = iso::worldToTilePos(mw, map_.tileW, map_.tileH);
      net_->sendPath(goal.x, goal.y);
      cmdMarker_ = goal;
      cmdMarkerAt_ = GetTime();
    }
  }
  if (net_->ownStats.statPoints > 0) {
    if (IsKeyPressed(KEY_F5)) net_->sendStat(0);
    if (IsKeyPressed(KEY_F6)) net_->sendStat(1);
    if (IsKeyPressed(KEY_F7)) net_->sendStat(2);
  }
  if (IsKeyPressed(KEY_I)) showInv_ = !showInv_;
  // ---- trade (T-029): T opens with nearest player; P commits; X cancels. ----
  if (IsKeyPressed(KEY_T)) {
    std::uint32_t bestP = 0;
    float bd = 3.0f;
    for (const auto& kv : rents_) {
      if (kv.first == net_->ownId || kv.second.snap.kind != 0) continue;
      if (kv.second.snap.hp == 0) continue;
      const Vector2 rp = entRenderPos(kv.second);
      const Vector2 op = entRenderPos(rents_[net_->ownId]);
      const float d = std::hypot(rp.x - op.x, rp.y - op.y);
      if (d < bd) {
        bd = d;
        bestP = kv.first;
      }
    }
    if (bestP != 0) {
      net_->sendTradeOpen(bestP);
      net_->tradeWithId = bestP;
      net_->tradeWithName = rents_[bestP].snap.name;
      net_->tradeGoldOffered = 0;
    }
  }
  if (net_->tradeWithId != 0) {
    if (IsKeyPressed(KEY_P)) net_->sendTradeCommit();
    if (IsKeyPressed(KEY_X)) net_->sendTradeCancel();
    if (IsKeyPressed(KEY_H)) net_->sendTradeOfferGold(net_->tradeGoldOffered + 10);
  }
  if (IsKeyPressed(KEY_ONE) && targetId_ != 0) net_->sendSkill(1, targetId_);
  // kit channels (T-054): 2 Mend 3 Bless 4 Ironskin 5 Firebolt.
  // Choir casts take the selected party target (else self); Firebolt wants the
  // attack target specifically (selected mob), falling back to 0 = no-op server.
  if (IsKeyPressed(KEY_TWO)) net_->sendSkill(2, chanTarget());
  if (IsKeyPressed(KEY_THREE)) net_->sendSkill(3, chanTarget());
  if (IsKeyPressed(KEY_FOUR)) net_->sendSkill(4, chanTarget());
  if (IsKeyPressed(KEY_FIVE)) net_->sendSkill(5, targetId_);
  if (IsKeyPressed(KEY_Q)) {
    // quick-sip: first Blood Vial stack
    for (const auto& kv : net_->inventory) {
      if (kv.second.itemId == 3001) {
        net_->sendUseItem(kv.first);
        break;
      }
    }
  }
  if (showVendor_) {
    if (IsKeyPressed(KEY_G)) net_->sendSellJunk();
    for (int i = 0; i < 5; ++i) {
      if (IsKeyPressed(KEY_F1 + i)) {
        net_->sendBuy(content::kVendorStock[static_cast<size_t>(i)], 1);
      }
    }
  }
  // trade mirror hygiene: server-directed lines are the truth
  if (net_->tradeWithId != 0) {
    for (const auto& cl : net_->chatIn) {
      if (cl.channel == 255 &&
          (cl.text.find("trade completed") != std::string::npos ||
           cl.text.find("trade cancelled") != std::string::npos)) {
        net_->tradeWithId = 0;
        net_->tradeWithName.clear();
        net_->tradeGoldOffered = 0;
      }
      if (cl.channel == 255 && cl.text.find("trading with") != std::string::npos) {
        net_->tradeWithName = cl.text.substr(13, cl.text.find(" - offer") > 13 ? cl.text.find(" - offer") - 13 : 16);
      }
    }
  }

  // auto-open vendor panel by proximity; auto-close when walking off
  const bool nv = vendorNear();
  if (nv && !showVendor_) showVendor_ = true;
  if (!nv && showVendor_) showVendor_ = false;
  // T-048: anvil panel + F-key petition (next tier of the equipped blade)
  const bool na = anvilNear();
  if (na && !showAnvil_) showAnvil_ = true;
  if (!na && showAnvil_) showAnvil_ = false;
  if (IsKeyPressed(KEY_F) && anvilNear() && !chatFocus_) {
    std::uint8_t cur = 0;
    bool armed = false;
    for (const auto& kv : net_->inventory) {
      const content::ItemDef* d = content::findItem(kv.second.itemId);
      if (kv.second.equipped && d != nullptr && d->slot == 0) {
        cur = kv.second.aura;
        armed = true;
        break;
      }
    }
    if (armed && cur < 5) net_->sendAnvilOp(static_cast<std::uint8_t>(cur + 1));
  }
  const int sx = (IsKeyDown(KEY_D) ? 1 : 0) - (IsKeyDown(KEY_A) ? 1 : 0);
  const int sy = (IsKeyDown(KEY_S) ? 1 : 0) - (IsKeyDown(KEY_W) ? 1 : 0);
  if (sx != 0 || sy != 0) {
    int dx = sx + sy;
    int dy = sy - sx;
    dx = dx < -1 ? -1 : (dx > 1 ? 1 : dx);
    dy = dy < -1 ? -1 : (dy > 1 ? 1 : dy);
    const double now = GetTime();
    const auto own = net_->ents.find(net_->ownId);
    const bool ownMoving = own != net_->ents.end() && own->second.moving;
    if (!ownMoving && now - lastStepSent_ > 0.15) {
      lastStepSent_ = now;
      net_->sendStep(dx, dy);
    }
  }
  if (GetTime() - lastPing_ > 1.0) {
    lastPing_ = GetTime();
    net_->sendPing();
  }
}

// ---- online state application -------------------------------------------------

void Game::applyNetState() {
  const double now = GetTime();
  for (const std::uint32_t id : net_->despawnedIds) rents_.erase(id);
  for (const std::uint32_t id : net_->spawnedIds) {
    auto it = net_->ents.find(id);
    if (it == net_->ents.end()) continue;
    RenderEnt re;
    re.snap = it->second;
    re.fromX = static_cast<float>(re.snap.x) / 1024.0f;
    re.fromY = static_cast<float>(re.snap.y) / 1024.0f;
    re.stamp = now;
    re.hasInterp = true;
    rents_[id] = re;
  }
  for (auto& [id, re] : rents_) {
    auto it = net_->ents.find(id);
    if (it == net_->ents.end()) continue;
    const NetEntSnapshot& s = it->second;
    const Vector2 cur = entRenderPos(re);
    const float nx = static_cast<float>(s.x) / 1024.0f;
    const float ny = static_cast<float>(s.y) / 1024.0f;
    if (nx != re.fromX || ny != re.fromY || cur.x != nx || cur.y != ny) {
      re.fromX = cur.x;
      re.fromY = cur.y;
      re.stamp = now;
      re.snap.x = s.x;
      re.snap.y = s.y;
    }
    re.snap.dir = s.dir;
    re.snap.moving = s.moving;
    re.snap.hp = s.hp;
    re.snap.hpMax = s.hpMax;
    re.snap.kind = s.kind;
    re.snap.level = s.level;
    re.snap.name = s.name;
  }
  if (targetId_ != 0) {
    const auto it = net_->ents.find(targetId_);
    if (it == net_->ents.end() || it->second.hp == 0) {
      // keep ring while dead-corpsing only if still visible; drop when gone
      if (it == net_->ents.end()) targetId_ = 0;
    }
  }
  for (const CombatPulse& cp : net_->combatIn) {
    auto it = rents_.find(cp.target);
    if (it == rents_.end()) continue;
    const Vector2 rp = entRenderPos(it->second);
    Floater f;
    f.x = rp.x;
    f.y = rp.y;
    f.at = now;
    if (cp.kind == 0) {
      f.text = "miss";
      f.r = 170;
      f.g = 170;
      f.b = 180;
    } else if (cp.kind == 4) {
      f.text = "+" + std::to_string(cp.amount);
      f.r = 90;
      f.g = 220;
      f.b = 110;
    } else if (cp.kind == 8) {
      // T-054 Mend: life given floats over the RECIPIENT (green +N)
      f.text = "mend +" + std::to_string(cp.amount);
      f.r = 90;
      f.g = 230;
      f.b = 120;
    } else if (cp.kind == 5) {
      // skill hit (Power Swing + Firebolt): red-caps over the CASTER
      {
        auto ai2 = rents_.find(cp.attacker);
        if (ai2 != rents_.end()) {
          const Vector2 ap = entRenderPos(ai2->second);
          f.x = ap.x;
          f.y = ap.y;
        }
      }
      f.text = "Power-Swing! " + std::to_string(cp.amount);
      f.r = 255;
      f.g = 60;
      f.b = 40;
    } else if (cp.kind == 6) {
      // anvil ceremony (T-041): red-caps over the petitioner
      auto ai = rents_.find(cp.attacker);
      if (ai != rents_.end()) {
        const Vector2 ap = entRenderPos(ai->second);
        f.x = ap.x;
        f.y = ap.y;
      }
      f.text = "Widow's Rite tier " + std::to_string(cp.amount);
      f.r = 120;
      f.g = 235;
      f.b = 235;
    } else if (cp.kind == 7) {
      // aura proc bands (T-047): 1xxx cleave, 2xxx sunder, 3xxx graft
      auto ai = rents_.find(cp.attacker);
      if (ai != rents_.end()) {
        const Vector2 ap = entRenderPos(ai->second);
        f.x = ap.x;
        f.y = ap.y;
      }
      if (cp.amount >= 3000) { f.text = "Graft +" + std::to_string(cp.amount - 3000); f.r = 120; f.g = 235; f.b = 130; }
      else if (cp.amount >= 2000) { f.text = "Sunder!"; f.r = 255; f.g = 150; f.b = 40; }
      else { f.text = "Cleave " + std::to_string(cp.amount - 1000); f.r = 235; f.g = 190; f.b = 90; }
    } else if (cp.kind == 3) {
      f.text = "SLAIN";
      f.r = 255;
      f.g = 200;
      f.b = 60;
    } else {
      f.text = std::to_string(cp.amount);
      if (cp.kind == 2) {
        f.text += "!";
        f.r = 255;
        f.g = 120;
        f.b = 40;
      } else {
        f.r = 235;
        f.g = 70;
        f.b = 70;
      }
    }
    floaters_.push_back(f);
    while (floaters_.size() > 24) floaters_.pop_front();
  }
  for (const ChatLine& c : net_->chatIn) {
    chatLog_.push_back(c);
    while (chatLog_.size() > 12) chatLog_.pop_front();
  }
  static bool welcomedShown = false;
  if (net_->welcomed && !welcomedShown) {
    welcomedShown = true;
    onlineBaseTick_ = tick_;
    chatLog_.push_back(ChatLine{2, "system", "you step into Thornwall. (Enter = chat)"});
  }
  net_->clearPulse();
}

Vector2 Game::entRenderPos(const RenderEnt& e) const {
  const double now = GetTime();
  const float tx = static_cast<float>(e.snap.x) / 1024.0f;
  const float ty = static_cast<float>(e.snap.y) / 1024.0f;
  const float kLerpSecs = 0.12f;  // ~2.4 ticks: smooth at 20 Hz
  const float a = static_cast<float>(std::min(1.0, (now - e.stamp) / kLerpSecs));
  return Vector2{e.fromX + (tx - e.fromX) * a, e.fromY + (ty - e.fromY) * a};
}

// ---- tick -------------------------------------------------------------------

void Game::applyJournalEvents() {
  const auto& ev = replay_->events;
  while (replayIdx_ < ev.size() && ev[replayIdx_].tick <= tick_) {
    const sim::JournalEvent& e = ev[replayIdx_++];
    if (e.tick != tick_) continue;
    switch (e.type) {
      case sim::kEventPath: {
        const sim::TilePos goal{e.a, e.b};
        if (!map_.inBounds(goal.x, goal.y) || map_.isBlocked(goal.x, goal.y)) break;
        const sim::TilePos start = walker_.moving ? walker_.target : walker_.tile();
        if (start == goal) {
          path_.clear();
          break;
        }
        const sim::PathResult res = sim::findPath(grid_, start, goal);
        if (res.found) path_.assign(res.tiles.begin(), res.tiles.end());
        break;
      }
      case sim::kEventStep:
        path_.clear();
        {
          const sim::TilePos cur = walker_.tile();
          walker_.beginStep(grid_, sim::TilePos{cur.x + e.a, cur.y + e.b});
        }
        break;
      default:
        break;
    }
  }
}

void Game::stepMovement() {
  walker_.step();
  if (walker_.moving || path_.empty()) return;
  const sim::TilePos cur = walker_.tile();
  while (!path_.empty()) {
    const sim::TilePos t = path_.front();
    if (t == cur) {
      path_.pop_front();
      continue;
    }
    if (walker_.beginStep(grid_, t)) {
      path_.pop_front();
      return;
    }
    path_.clear();
    return;
  }
}

void Game::frameBegin() {
  if (net_ == nullptr) return;
  net_->poll();
  // zone handoff: server re-sends Welcome on portal transfer
  if (net_->welcomed && net_->mapId != loadedMapId) {
    if (zoneReload(net_->mapId)) net_->ents.clear();
  }
  applyNetState();
  handleInputOnline();
}

void Game::tick() {
  ++tick_;
  animT_ += sim::kTickSeconds;
  if (net_ == nullptr) {
    inTick_ = true;
    if (replay_) {
      applyJournalEvents();
    } else {
      handleInput();
    }
    stepMovement();
    inTick_ = false;
    if ((rec_ != nullptr && rec_->ok()) || hashSink_ != nullptr) {
      const std::uint64_t h = sim::walkerStateHash(walker_, path_);
      if (rec_ && rec_->ok()) rec_->hash(tick_, h);
      if (hashSink_ != nullptr) hashSink_->push_back({tick_, h});
    }
  }
}

void Game::debugSetHour(float h) {
  debugHourOffset_ = h - sim::hourAt(tick_);
}

float Game::gameHour() const {
  float base;
  if (net_ != nullptr && net_->welcomed) {
    // estimate from the server's welcome clock + local ticks since
    const float welcomeH = net_->welcomeHour;
    const float hoursSince =
        static_cast<float>(tick_ - onlineBaseTick_) / static_cast<float>(sim::kTicksPerGameHour);
    base = welcomeH + hoursSince;
  } else {
    base = sim::hourAt(tick_);
  }
  float h = base + debugHourOffset_;
  while (h >= 24.0f) h -= 24.0f;
  while (h < 0.0f) h += 24.0f;
  return h;
}

// ---- rendering --------------------------------------------------------------

Color Game::terrainColor(std::uint16_t type) const {
  switch (type) {
    case 0:  return Color{64, 84, 46, 255};
    case 1:  return Color{94, 74, 52, 255};
    case 3:  return Color{36, 54, 88, 255};
    case 4:  return Color{118, 90, 58, 255};
    case 5:  return Color{86, 84, 92, 255};
    case 6:  return Color{60, 54, 40, 255};
    case 7:  return Color{50, 66, 38, 255};
    default: return Color{80, 80, 88, 255};
  }
}

void Game::drawGround() {
  for (int y = 0; y < map_.h; ++y) {
    for (int x = 0; x < map_.w; ++x) {
      const size_t i = static_cast<size_t>(y) * static_cast<size_t>(map_.w) +
                       static_cast<size_t>(x);
      const std::uint16_t t = map_.ground[i];
      if (t == 2) continue;
      const Vector2 c = iso::tileToWorld(x, y, map_.tileW, map_.tileH);
      iso::drawDiamond(c, map_.tileW, map_.tileH, terrainColor(t),
                       showGrid_ ? Color{0, 0, 0, 60} : Color{0, 0, 0, 0});
      if (showGrid_ && map_.blocked[i] != 0) {
        iso::drawDiamond(c, map_.tileW, map_.tileH, Color{120, 30, 30, 60},
                         Color{180, 40, 40, 90});
      }
    }
  }
  bool heroDrawn = false;
  if (net_ == nullptr) {
    const int heroRow = static_cast<int>(std::floor(walker_.fy() + 0.5f));
    for (int y = 0; y < map_.h; ++y) {
      for (int x = 0; x < map_.w; ++x) {
        const size_t i = static_cast<size_t>(y) * static_cast<size_t>(map_.w) +
                         static_cast<size_t>(x);
        if (map_.ground[i] != 2) continue;
        const Vector2 c = iso::tileToWorld(x, y, map_.tileW, map_.tileH);
        iso::drawPrism(c, map_.tileW, map_.tileH, 28, Color{120, 120, 128, 255},
                       Color{74, 74, 82, 255}, Color{92, 92, 100, 255});
      }
      if (!heroDrawn && y == heroRow) {
        drawHero();
        heroDrawn = true;
      }
    }
    if (!heroDrawn) drawHero();
  } else {
    for (int y = 0; y < map_.h; ++y) {
      for (int x = 0; x < map_.w; ++x) {
        const size_t i = static_cast<size_t>(y) * static_cast<size_t>(map_.w) +
                         static_cast<size_t>(x);
        if (map_.ground[i] != 2) continue;
        const Vector2 c = iso::tileToWorld(x, y, map_.tileW, map_.tileH);
        iso::drawPrism(c, map_.tileW, map_.tileH, 28, Color{120, 120, 128, 255},
                       Color{74, 74, 82, 255}, Color{92, 92, 100, 255});
      }
    }
    drawEntitiesOnline();
  }
}

void Game::drawHero() {
  const Vector2 w = iso::tileToWorldF(Vector2{walker_.fx(), walker_.fy()}, map_.tileW,
                                      map_.tileH);
  const Rectangle src =
      animFrame(heroAtlas_, walker_.moving ? "walk" : "idle", walker_.dir, animT_);
  if (src.width <= 0.0f) {
    DrawCircleV(w, 8.0f, RED);
    return;
  }
  DrawTexturePro(heroAtlas_.tex, src, Rectangle{w.x, w.y, src.width, src.height},
                 Vector2{src.width * 0.5f, 42.0f}, 0.0f, WHITE);
}

void Game::drawRemoteEnt(const RenderEnt& e, bool isOwn) {
  const Vector2 rp = entRenderPos(e);
  const Vector2 w = iso::tileToWorldF(rp, map_.tileW, map_.tileH);
  if (content::wireIsFurniture(e.snap.kind)) {
    // furniture band (T-047 constants): vendor stall 64, widow anvil 65
    if (e.snap.kind == content::kWireKindAnvil) {
      DrawRectangle(static_cast<int>(w.x) - 12, static_cast<int>(w.y) - 22, 24, 18,
                    Color{70, 124, 128, 255});
      DrawRectangle(static_cast<int>(w.x) - 8, static_cast<int>(w.y) - 30, 16, 8,
                    Color{40, 80, 84, 255});
      DrawText("anvil", static_cast<int>(w.x) - 12, static_cast<int>(w.y) - 18, 2,
               Color{150, 240, 240, 255});
    } else {
      DrawRectangle(static_cast<int>(w.x) - 14, static_cast<int>(w.y) - 26, 28, 22,
                    Color{120, 92, 40, 255});
    }
    if (!e.snap.name.empty()) {
      const int tw = MeasureText(e.snap.name.c_str(), 10);
      DrawText(e.snap.name.c_str(), static_cast<int>(w.x) - tw / 2,
               static_cast<int>(w.y) - 52, 10, Color{190, 210, 220, 255});
    }
    return;
  }
  if (e.snap.id == targetId_) {
    DrawEllipseLines(static_cast<int>(w.x), static_cast<int>(w.y), 14.0f, 7.0f,
                     Color{220, 40, 40, 230});
  }
  const Rectangle src = animFrame(heroAtlas_, e.snap.moving ? "walk" : "idle",
                                  static_cast<int>(e.snap.dir), animT_);
  const Color tint = isOwn ? Color{255, 255, 255, 255} : Color{190, 190, 200, 255};
  if (src.width > 0.0f) {
    DrawTexturePro(heroAtlas_.tex, src, Rectangle{w.x, w.y, src.width, src.height},
                   Vector2{src.width * 0.5f, 42.0f}, 0.0f, tint);
  } else {
    DrawCircleV(w, 8.0f, MAROON);
  }
  {
    std::string label = e.snap.name;
    if (!label.empty() && e.snap.level > 1) {
      label = label + " L" + std::to_string(e.snap.level);
    }
    if (!label.empty()) {
      const int tw = MeasureText(label.c_str(), 10);
      // T-057: chaotic = era red name (the gamble must read at a glance)
      Color nc = isOwn ? Color{230, 210, 190, 255} : Color{190, 190, 200, 255};
      if (e.snap.karmaBand == 2) nc = Color{235, 60, 50, 255};
      else if (e.snap.karmaBand == 0) nc = Color{170, 200, 255, 255};
      DrawText(label.c_str(), static_cast<int>(w.x) - tw / 2,
               static_cast<int>(w.y) - 52, 10, nc);
    }
  }
  if (e.snap.hpMax > 0 && e.snap.hp < e.snap.hpMax) {
    const float frac = static_cast<float>(e.snap.hp) / static_cast<float>(e.snap.hpMax);
    DrawRectangle(static_cast<int>(w.x) - 14, static_cast<int>(w.y) - 46, 28, 3,
                  Color{0, 0, 0, 180});
    DrawRectangle(static_cast<int>(w.x) - 14, static_cast<int>(w.y) - 46,
                  static_cast<int>(28.0f * frac), 3, Color{150, 30, 30, 235});
  }
}

void Game::drawEntitiesOnline() {
  std::vector<const RenderEnt*> order;
  order.reserve(rents_.size());
  for (const auto& kv : rents_) order.push_back(&kv.second);
  std::sort(order.begin(), order.end(), [&](const RenderEnt* a, const RenderEnt* b) {
    return entRenderPos(*a).y < entRenderPos(*b).y;
  });
  for (const RenderEnt* e : order) drawRemoteEnt(*e, e->snap.id == net_->ownId);
}

void Game::drawPathPreview() const {
  for (const sim::TilePos t : path_) {
    const Vector2 c = iso::tileToWorld(t, map_.tileW, map_.tileH);
    iso::drawDiamond(c, map_.tileW, map_.tileH, Color{190, 40, 40, 40},
                     Color{220, 60, 60, 120});
  }
}

void Game::drawCommandMarker() const {
  if (cmdMarker_.x < 0) return;
  const double age = GetTime() - cmdMarkerAt_;
  if (age > 0.8) return;
  const Vector2 c = iso::tileToWorld(cmdMarker_, map_.tileW, map_.tileH);
  const auto a = static_cast<unsigned char>(220 * (1.0 - age / 0.8));
  iso::drawDiamond(c, map_.tileW, map_.tileH, Color{0, 0, 0, 0},
                   Color{230, 70, 70, a});
}

const char* Game::mapFileFor(std::uint16_t mapId) {
  switch (mapId) {
    case 2: return "assets/maps/fields_overflow.bhmap";
    case 3: return "assets/maps/thornwall_crypt.bhmap";
    default: return "assets/maps/thornwall.bhmap";
  }
}

bool Game::zoneReload(std::uint16_t mapId) {
  if (mapId == loadedMapId) return true;
  std::string err;
  auto m = sim::loadBhmap(mapFileFor(mapId), &err);
  if (!m) {
    std::fprintf(stderr, "[client] zone map load failed (%u): %s\n",
                 static_cast<unsigned>(mapId), err.c_str());
    return false;
  }
  map_ = std::move(*m);
  loadedMapId = mapId;
  rents_.clear();
  floaters_.clear();
  targetId_ = 0;
  cmdMarker_ = sim::TilePos{-1, -1};
  // camera: jump to the arrival point (server Welcome carries it)
  if (net_ != nullptr) {
    auto it = net_->ents.find(net_->ownId);
    if (it != net_->ents.end()) {
      const Vector2 w = iso::tileToWorldF(
          Vector2{static_cast<float>(it->second.x) / 1024.0f,
                  static_cast<float>(it->second.y) / 1024.0f},
          map_.tileW, map_.tileH);
      rig_.cam.target = w;
    }
  }
  return true;
}

void Game::drawFloaters() {
  const double now = GetTime();
  while (!floaters_.empty() && now - floaters_.front().at > 0.9) floaters_.pop_front();
  for (const Floater& f : floaters_) {
    const double age = now - f.at;
    const float rise = static_cast<float>(age) * 28.0f;
    const auto a = static_cast<unsigned char>(255 * (1.0 - age / 0.9));
    const Vector2 w = iso::tileToWorldF(Vector2{f.x, f.y}, map_.tileW, map_.tileH);
    DrawText(f.text.c_str(), static_cast<int>(w.x) - 8,
             static_cast<int>(w.y) - 56 - static_cast<int>(rise), 10,
             Color{f.r, f.g, f.b, a});
  }
}

void Game::drawStatPanel() const {
  if (net_ == nullptr || !net_->welcomed) return;
  const OwnStatsWire& st = net_->ownStats;
  const int px = 1024 - 190;
  // kit block grows: mp row always, buff rows only while active (era chrome)
  const bool showBuffs = st.blessTicksLeft > 0 || st.ironskinTicksLeft > 0;
  const int ph = (st.statPoints > 0 ? 92 : 74) + 40 + (showBuffs ? 12 : 0);
  DrawRectangle(px, 8, 182, ph, Color{0, 0, 0, 170});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), 8, 182,
                                 static_cast<float>(ph)},
                       1.0f, Color{150, 30, 30, 200});
  char buf[128];
  const content::KitDef* kit = content::findKit(st.classId);
  std::snprintf(buf, sizeof buf, "LEVEL %u  %s", st.level,
                kit != nullptr ? kit->name : "?");
  DrawText(buf, px + 8, 14, 10, Color{230, 210, 190, 255});
  // xp bar
  const float frac =
      st.xpNext > 0 ? static_cast<float>(st.xp) / static_cast<float>(st.xpNext) : 0.0f;
  DrawRectangle(px + 8, 30, 166, 6, Color{30, 20, 20, 255});
  DrawRectangle(px + 8, 30, static_cast<int>(166.0f * frac), 6, Color{150, 30, 30, 255});
  std::snprintf(buf, sizeof buf, "xp %u / %u", st.xp, st.xpNext);
  DrawText(buf, px + 8, 40, 10, LIGHTGRAY);
  std::snprintf(buf, sizeof buf, "STR %u  VIT %u  DEX %u", st.str, st.vit, st.dex);
  DrawText(buf, px + 8, 54, 10, LIGHTGRAY);
  if (st.statPoints > 0) {
    std::snprintf(buf, sizeof buf, "+%u pts: F5 STR F6 VIT F7 DEX", st.statPoints);
    DrawText(buf, px + 8, 70, 10, Color{255, 200, 80, 255});
  }
  // T-053/54: mana bar + buff countdowns (kit resource legibility)
  const int by = (st.statPoints > 0 ? 86 : 70) + 2;
  const float mfrac = st.mpMax > 0 ? static_cast<float>(st.mp) / static_cast<float>(st.mpMax) : 0.0f;
  DrawRectangle(px + 8, by, 166, 6, Color{20, 20, 30, 255});
  DrawRectangle(px + 8, by, static_cast<int>(166.0f * mfrac), 6, Color{60, 70, 160, 255});
  std::snprintf(buf, sizeof buf, "mp %u / %u", st.mp, st.mpMax);
  DrawText(buf, px + 8, by + 10, 10, Color{170, 180, 235, 255});
  const char* al = st.karma < 0 ? "CHAOTIC" : (st.karma > 500 ? "lawful" : "neutral");
  std::snprintf(buf, sizeof buf, "%s %d%s", al, st.karma,
                st.karma < 0 ? "  -red: drops, no shops-" : "");
  DrawText(buf, px + 8, by + 24, 10,
           st.karma < 0 ? Color{235, 60, 50, 255} : Color{170, 180, 235, 255});
  buf[0] = 0;
  if (showBuffs) {
    const int byy = by + 38;
    char nb[96];
    std::snprintf(nb, sizeof nb, "%s%s%s    %us / %us",
                  st.blessTicksLeft > 0 ? "BLESS " : "",
                  st.blessTicksLeft > 0 && st.ironskinTicksLeft > 0 ? "+ " : "",
                  st.ironskinTicksLeft > 0 ? "IRONSKIN" : "",
                  st.blessTicksLeft / 20u, st.ironskinTicksLeft / 20u);
    DrawText(nb, px + 8, byy, 10, Color{200, 235, 170, 255});
  }
}

std::uint32_t Game::chanTarget() const {
  if (net_ != nullptr && targetId_ != 0) {
    for (const auto& m : net_->party)
      if (m.entityId == targetId_) return targetId_;  // party pick stays
  }
  return 0;  // server choirTarget(): 0 = self
}

void Game::drawPartyFrame() const {
  if (net_ == nullptr || !net_->welcomed || net_->partyId == 0 || net_->party.empty()) return;
  const int px = 8;
  const int py = 110;          // below chat/help chrome, left margin
  const int rowH = 24;
  const int pw = 150;
  const int ph = 18 + static_cast<int>(net_->party.size()) * rowH;
  DrawRectangle(px, py, pw, ph, Color{0, 0, 0, 160});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), static_cast<float>(py),
                                 static_cast<float>(pw), static_cast<float>(ph)},
                       1.0f, Color{150, 30, 30, 200});
  DrawText("PARTY", px + 8, py + 4, 10, Color{230, 210, 190, 255});
  char buf[96];
  int row = 0;
  for (const auto& m : net_->party) {
    const int ry = py + 18 + row * rowH;
    const bool leader = (m.entityId == net_->partyLeaderId);
    std::snprintf(buf, sizeof buf, "%s%s L%u", leader ? "*" : "", m.name.c_str(), m.level);
    DrawText(buf, px + 8, ry, 10,
             m.entityId == net_->ownId ? Color{255, 230, 120, 255} : Color{220, 220, 220, 255});
    const float frac = m.hpMax > 0 ? static_cast<float>(m.hp) / static_cast<float>(m.hpMax) : 0.0f;
    DrawRectangle(px + 8, ry + 12, 134, 5, Color{30, 20, 20, 255});
    DrawRectangle(px + 8, ry + 12, static_cast<int>(134.0f * frac), 5, Color{150, 30, 30, 255});
    ++row;
  }
}

void Game::drawTradeBanner() const {
  if (net_ == nullptr || net_->tradeWithId == 0) return;
  const int pw = 360, px = (1024 - pw) / 2, py = 640 - 44;
  DrawRectangle(px, py, pw, 36, Color{0, 0, 0, 190});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), static_cast<float>(py),
                                 static_cast<float>(pw), 36.0f},
                       1.0f, Color{150, 30, 30, 220});
  char buf[160];
  std::snprintf(buf, sizeof buf, "TRADE with %s", net_->tradeWithName.c_str());
  DrawText(buf, px + 10, py + 5, 10, Color{230, 210, 190, 255});
  std::snprintf(buf, sizeof buf,
                "my gold offer %ug | click items to offer | H +10g | P COMMIT | X cancel",
                net_->tradeGoldOffered);
  DrawText(buf, px + 10, py + 19, 10, Color{190, 190, 190, 255});
}

bool Game::vendorNear() const {
  if (net_ == nullptr || !net_->welcomed) return false;
  const auto own = rents_.find(net_->ownId);
  if (own == rents_.end()) return false;
  const Vector2 op = entRenderPos(own->second);
  for (const auto& kv : rents_) {
    if (kv.second.snap.kind == 64) {
      const Vector2 vp = entRenderPos(kv.second);
      if (std::fabs(vp.x - op.x) <= 3.0f && std::fabs(vp.y - op.y) <= 3.0f) return true;
    }
  }
  return false;
}

bool Game::anvilNear() const {
  if (net_ == nullptr || !net_->welcomed) return false;
  const auto own = rents_.find(net_->ownId);
  if (own == rents_.end()) return false;
  const Vector2 op = entRenderPos(own->second);
  for (const auto& kv : rents_) {
    if (kv.second.snap.kind == content::kWireKindAnvil) {
      const Vector2 vp = entRenderPos(kv.second);
      if (std::fabs(vp.x - op.x) <= 2.5f && std::fabs(vp.y - op.y) <= 2.5f) return true;
    }
  }
  return false;
}

void Game::drawInventoryPanel() const {
  if (net_ == nullptr || !showInv_) return;
  const int px = 1024 - 250, py = 150;
  DrawRectangle(px, py, 240, 220, Color{0, 0, 0, 185});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), static_cast<float>(py), 240, 220},
                       1.0f, Color{150, 30, 30, 220});
  DrawText("INVENTORY  (E=equip/use by clicking)", px + 8, py + 6, 10,
           Color{230, 210, 190, 255});
  int y = py + 24;
  int row = 0;
  for (const auto& kv : net_->inventory) {
    const content::ItemDef* d = content::findItem(kv.second.itemId);
    if (d == nullptr) continue;
    char buf[96];
    char auraMark[8] = "";
    if (kv.second.aura > 0) {
      std::snprintf(auraMark, sizeof auraMark, " +%s",
                    kv.second.aura == 1 ? "I"     : kv.second.aura == 2 ? "II"
                    : kv.second.aura == 3 ? "III"   : kv.second.aura == 4 ? "IV"
                                          : "V");
    }
    std::snprintf(buf, sizeof buf, "%s%s %s%s%s",
                  kv.second.equipped ? "[E] " : "    ",
                  kv.second.qty > 1 ? (std::to_string(kv.second.qty) + "x").c_str() : "",
                  d->name, d->slot == 0 ? " (weapon)" : d->slot == 1 ? " (armor)" : "",
                  auraMark);
    const Rectangle rr{static_cast<float>(px + 6), static_cast<float>(y - 2), 226, 14};
    if (row == invHover_) DrawRectangleRec(rr, Color{120, 30, 30, 120});
    const Color rowCol =
        kv.second.aura > 0 ? Color{120, 235, 235, 255}  // widow-blessed teal
        : kv.second.equipped ? Color{255, 200, 90, 255}
                             : Color{200, 195, 185, 255};
    DrawText(buf, px + 10, y, 10, rowCol);
    y += 16;
    ++row;
  }
  if (net_->inventory.empty()) {
    DrawText("empty. Go kill something soft.", px + 8, y, 10, GRAY);
  }
  char gold[64];
  std::snprintf(gold, sizeof gold, "gold %ug   sword skill %u", net_->ownStats.gold,
                net_->ownStats.swordSkill);
  DrawText(gold, px + 8, py + 200, 10, Color{255, 215, 120, 255});
}

void Game::drawAnvilPanel() const {
  if (net_ == nullptr || !showAnvil_) return;
  const int px = 12, py = 310;
  DrawRectangle(px, py, 268, 150, Color{8, 14, 16, 215});
  DrawRectangleLinesEx(Rectangle{px, py, 268, 150}, 1.0f, Color{90, 200, 210, 220});
  DrawText("THE WIDOW ANVIL", px + 8, py + 6, 10, Color{150, 240, 240, 255});
  // equipped weapon state
  std::uint8_t cur = 0;
  bool armed = false;
  std::string wname;
  for (const auto& kv : net_->inventory) {
    const content::ItemDef* d = content::findItem(kv.second.itemId);
    if (kv.second.equipped && d != nullptr && d->slot == 0) {
      cur = kv.second.aura;
      armed = true;
      wname = d->name;
      break;
    }
  }
  int y = py + 24;
  char buf[160];
  if (!armed) {
    DrawText("the Anvil decides only armed petitions.", px + 10, y, 10, GRAY);
    y += 16;
  } else {
    std::snprintf(buf, sizeof buf, "blade: %s  aura %u/5", wname.c_str(), cur);
    DrawText(buf, px + 10, y, 10, Color{200, 220, 225, 255});
    y += 16;
  }
  if (armed && cur < 5) {
    const content::AuraTier* t = content::findAuraTier(static_cast<std::uint8_t>(cur + 1));
    if (t != nullptr) {
      const content::ItemDef* pd = content::findItem(t->partItemId);
      std::snprintf(buf, sizeof buf, "tier %u rite: skill %u | %ux %s | %ug",
                    t->tier, t->reqSkill, t->partQty,
                    pd != nullptr ? pd->name : "offering", t->gold);
      DrawText(buf, px + 10, y, 10, Color{230, 220, 200, 255});
      y += 16;
      std::string law = t->failLaw == 1   ? "failure DROWNS the steel"
                        : t->failLaw == 0 ? "failure drinks the offering"
                                          : "the first rite of each tier is guaranteed";
      DrawText(law.c_str(), px + 10, y, 10,
               t->failLaw == 1 ? Color{235, 120, 120, 255} : Color{150, 210, 160, 255});
      y += 16;
    }
    DrawText("F - petition the Widow", px + 10, y + 4, 10, Color{150, 240, 240, 255});
  } else if (armed) {
    DrawText("the steel is fully wedded to the Widow.", px + 10, y, 10,
             Color{150, 210, 160, 255});
  }
}

void Game::drawVendorPanel() const {
  if (net_ == nullptr || !showVendor_) return;
  const int px = 12, py = 150;
  DrawRectangle(px, py, 250, 150, Color{12, 10, 8, 210});
  DrawRectangleLinesEx(Rectangle{px, py, 250, 150}, 1.0f, Color{180, 150, 90, 220});
  DrawText("MARTA the Quarterwidow  -  stock", px + 8, py + 6, 10,
           Color{235, 220, 200, 255});
  int y = py + 24;
  int i = 0;
  for (const std::uint32_t id : content::kVendorStock) {
    const content::ItemDef* d = content::findItem(id);
    if (d == nullptr) continue;
    char buf[96];
    std::snprintf(buf, sizeof buf, "F%d  %-14s %ug", i + 1, d->name, d->value);
    DrawText(buf, px + 10, y, 10, Color{220, 205, 185, 255});
    y += 16;
    ++i;
  }
  char buf[96];
  std::snprintf(buf, sizeof buf, "G  sell all junk (40%%)");
  DrawText(buf, px + 10, y + 2, 10, Color{190, 170, 150, 255});
  std::snprintf(buf, sizeof buf, "gold %ug", net_->ownStats.gold);
  DrawText(buf, px + 10, py + 130, 10, Color{255, 215, 120, 255});
}

void Game::drawDeathOverlay() const {
  if (net_ == nullptr) return;
  const auto own = net_->ents.find(net_->ownId);
  if (own == net_->ents.end() || own->second.hp > 0) return;
  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{60, 0, 0, 110});
  DrawText("YOU DIED", 1024 / 2 - 150, 340, 48, Color{200, 40, 40, 255});
  DrawText("crawling back to Thornwall...", 1024 / 2 - 100, 396, 14,
           Color{200, 170, 160, 230});
}

void Game::drawChat() const {
  if (chatLog_.empty() && !chatFocus_) return;
  const int lines = static_cast<int>(chatLog_.size());
  const int baseY = 768 - 26 - lines * 14;
  int y = baseY;
  for (const ChatLine& c : chatLog_) {
    const Color col = c.channel == 3   ? Color{190, 80, 80, 245}
                      : c.channel == 2 ? Color{150, 150, 160, 230}
                      : c.channel == 0 ? Color{200, 200, 200, 255}
                                       : Color{120, 200, 160, 255};
    const std::string line = c.from.empty() ? c.text : "<" + c.from + "> " + c.text;
    DrawText(line.c_str(), 12, y, 10, col);
    y += 14;
  }
  if (chatFocus_) {
    DrawRectangle(8, 768 - 24, 480, 20, Color{0, 0, 0, 200});
    DrawRectangleLinesEx(Rectangle{8, 768 - 24, 480, 20}, 1.0f, Color{150, 30, 30, 220});
    const std::string shown = "> " + chatBuf_ + "_";
    DrawText(shown.c_str(), 14, 768 - 19, 10, Color{235, 225, 210, 255});
  }
}

void Game::drawHud() const {
  DrawRectangle(8, 8, 470, 118, Color{0, 0, 0, 170});
  DrawRectangleLinesEx(Rectangle{8, 8, 470, 118}, 1.0f, Color{150, 30, 30, 200});
  char buf[224];
  if (net_ != nullptr) {
    const char* st = net_->state == NetClient::State::kInWorld    ? "ONLINE"
                     : net_->state == NetClient::State::kFailed   ? "FAILED"
                     : net_->state == NetClient::State::kConnecting ? "CONNECTING"
                                                                   : "AWAITING LOGIN";
    std::snprintf(buf, sizeof buf, "BLOODHOLLOW  |  %s", st);
    DrawText(buf, 16, 14, 10, Color{200, 180, 160, 255});
    std::snprintf(buf, sizeof buf, "tick %lld | srv tick %u | online %u",
                  static_cast<long long>(tick_), net_->serverTick, net_->online);
    DrawText(buf, 16, 30, 10, LIGHTGRAY);
    const float h = gameHour();
    std::snprintf(buf, sizeof buf,
                  "time %02d:%02d | ping %d ms | srv p99 %u us", static_cast<int>(h),
                  static_cast<int>((h - std::floor(h)) * 60.0f), net_->pingMs,
                  net_->serverP99Us);
    DrawText(buf, 16, 46, 10, LIGHTGRAY);
    std::snprintf(buf, sizeof buf, "entities visible: %zu", rents_.size());
    DrawText(buf, 16, 62, 10, LIGHTGRAY);
    {
      // moral readout (T-048): clean souls glow, drowned blades stain
      const char* kWord = net_->ownStats.karma > 0   ? "clean"
                          : net_->ownStats.karma < 0 ? "stained"
                                                     : "gray";
      const Color kCol = net_->ownStats.karma > 0   ? Color{150, 230, 170, 255}
                         : net_->ownStats.karma < 0 ? Color{235, 120, 120, 255}
                                                    : Color{180, 170, 160, 255};
      std::snprintf(buf, sizeof buf, "karma %d (%s)  |  sword %u  |  gold %u",
                    net_->ownStats.karma, kWord, net_->ownStats.swordSkill,
                    net_->ownStats.gold);
      DrawText(buf, 240, 62, 10, kCol);
    }
    DrawText("LMB walk/fight - 1 PowerSwing - Q sip - I bag - T trade (P commit/X cancel) - Enter chat", 16,
             82, 10, GRAY);
    DrawText("I inventory - V vendor - Space recenter - F3 grid - ESC quit", 16, 98, 10,
             Color{120, 110, 100, 255});
  } else {
    const char* mode = replay_ != nullptr ? "REPLAY" : (rec_ != nullptr ? "RECORDING" : "LIVE");
    std::snprintf(buf, sizeof buf, "BLOODHOLLOW  |  offline %s", mode);
    DrawText(buf, 16, 14, 10, Color{200, 180, 160, 255});
    std::snprintf(buf, sizeof buf, "tick %lld | hero (%.1f, %.1f)",
                  static_cast<long long>(tick_), static_cast<double>(walker_.fx()),
                  static_cast<double>(walker_.fy()));
    DrawText(buf, 16, 30, 10, LIGHTGRAY);
    const float h = gameHour();
    std::snprintf(buf, sizeof buf, "time %02d:%02d | fps %d", static_cast<int>(h),
                  static_cast<int>((h - std::floor(h)) * 60.0f), GetFPS());
    DrawText(buf, 16, 46, 10, LIGHTGRAY);
    std::snprintf(buf, sizeof buf, "spawners %zu | portals %zu", map_.spawners.size(),
                  map_.portals.size());
    DrawText(buf, 16, 62, 10, LIGHTGRAY);
    DrawText("LMB move - WASD step - Enter chat (online) - Z/wheel zoom - MMB pan", 16, 82,
             10, GRAY);
    DrawText("Space recenter - F3 grid - F4 path - ESC quit", 16, 98, 10,
             Color{120, 110, 100, 255});
  }
}

void Game::render(double /*interpAlpha*/) {
  rig_.input();
  Vector2 focus;
  if (net_ != nullptr) {
    const auto own = rents_.find(net_->ownId);
    focus = own != rents_.end()
                ? iso::tileToWorldF(entRenderPos(own->second), map_.tileW, map_.tileH)
                : iso::tileToWorld(map_.w / 2, map_.h / 3, map_.tileW, map_.tileH);
  } else {
    focus = iso::tileToWorldF(Vector2{walker_.fx(), walker_.fy()}, map_.tileW, map_.tileH);
  }
  rig_.follow(focus);

  BeginDrawing();
  ClearBackground(Color{8, 8, 12, 255});
  BeginMode2D(rig_.cam);
  drawGround();
  if (showPath_ && net_ == nullptr) drawPathPreview();
  drawCommandMarker();
  if (net_ != nullptr) drawFloaters();
  EndMode2D();

  const Color ov = nightOverlay(gameHour());
  if (ov.a > 0) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ov);
  drawChat();
  drawHud();
  drawStatPanel();
  drawPartyFrame();
  drawInventoryPanel();
  drawVendorPanel();
  drawAnvilPanel();
  drawTradeBanner();
  drawDeathOverlay();
  EndDrawing();
}

}  // namespace bh
