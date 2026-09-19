#include "game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>

#include "assets/placeholder.h"
#include "content/items.h"
#include "content/kits.h"
#include "content/mobs.h"  // T-ART-05: mobId/slug for the atlas table
#include "content/auras.h"
#include "content/wirekind.h"
#include "render/daynight.h"
#include "render/iso.h"
#include "render/lightmask.h"
#include "render/overhead.h"  // T-ART-07 overhead tint priority
#include "render/refine_glow.h"  // T-ART-11 refine glow tiers (inventory rows)
#include "render/rarity_chrome.h"  // T-159f1.3 rarity marker (inventory rows)
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
      // T-169: intercept /help locally — never send to server
      if (chatBuf_ == "/help" || chatBuf_ == "help") {
        showHelp_ = !showHelp_;
      } else if (net_ != nullptr && net_->state == NetClient::State::kInWorld) {
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
  if (IsKeyPressed(KEY_F1)) showHelp_ = !showHelp_;  // T-169
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
  // T-167 creation panel (pre-world): 1/2/3 kit, M/F sex, Enter to answer.
  // Runs BEFORE chat-ENTER so Enter confirms instead of opening chat.
  if (net_->needsCreate && !net_->welcomed) {
    if (IsKeyPressed(KEY_ONE)) createClass_ = 1;
    if (IsKeyPressed(KEY_TWO)) createClass_ = 2;
    if (IsKeyPressed(KEY_THREE)) createClass_ = 3;
    if (IsKeyPressed(KEY_M)) createSex_ = 1;
    if (IsKeyPressed(KEY_F)) createSex_ = 2;
    if (IsKeyPressed(KEY_ENTER) && createClass_ != 0 && createSex_ != 0)
      net_->sendCharCreate(createClass_, createSex_);
    // T-ART-13 dev captures: --create preselects, auto-answer on arrival.
    if (createClass_ != 0 && createSex_ != 0 && debugAutoCreate_) {
      debugAutoCreate_ = false;  // once (server answers with Welcome)
      net_->sendCharCreate(createClass_, createSex_);
    }
    return;
  }
  if (IsKeyPressed(KEY_ENTER)) {
    chatFocus_ = true;
    return;
  }
  if (IsKeyPressed(KEY_F3)) showGrid_ = !showGrid_;
  if (IsKeyPressed(KEY_F4)) showPath_ = !showPath_;
  if (IsKeyPressed(KEY_F1)) showHelp_ = !showHelp_;  // T-169
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
              if (d->slot == 5) net_->sendUseItem(it->first);
              if (d->slot <= 4) net_->sendToggleEquip(it->first);
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
  if (net_->ownStats.statPoints > 0 && !showVendor_) {
    if (IsKeyPressed(KEY_F5)) net_->sendStat(0);
    if (IsKeyPressed(KEY_F6)) net_->sendStat(1);
    if (IsKeyPressed(KEY_F7)) net_->sendStat(2);
    // T-160 five-stat model: F8 INT, F9 MAG (vendor guard: F-keys buy there).
    if (IsKeyPressed(KEY_F8)) net_->sendStat(3);
    if (IsKeyPressed(KEY_F9)) net_->sendStat(4);
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
  // kit channels (T-054): 1-5 plain. T-161b pages: Shift+1..8 = ch11..18,
  // Ctrl+1..5 = ch19..23. Strikes take the attack target; self rites take
  // the choir target (server ignores it where unused). Wrong-kit pages are
  // quiet no-ops server-side — one layout serves all three kits.
  const bool shiftPage = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  const bool ctrlPage =
      IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (!shiftPage && !ctrlPage) {
    if (IsKeyPressed(KEY_ONE) && targetId_ != 0)
      net_->sendSkill(1, targetId_);
    if (IsKeyPressed(KEY_TWO)) net_->sendSkill(2, chanTarget());
    if (IsKeyPressed(KEY_THREE)) net_->sendSkill(3, chanTarget());
    if (IsKeyPressed(KEY_FOUR)) net_->sendSkill(4, chanTarget());
    if (IsKeyPressed(KEY_FIVE)) net_->sendSkill(5, targetId_);
  } else if (shiftPage && !ctrlPage) {
    if (IsKeyPressed(KEY_ONE)) net_->sendSkill(11, chanTarget());
    if (IsKeyPressed(KEY_TWO) && targetId_ != 0)
      net_->sendSkill(12, targetId_);
    if (IsKeyPressed(KEY_THREE)) net_->sendSkill(13, chanTarget());
    if (IsKeyPressed(KEY_FOUR)) net_->sendSkill(14, chanTarget());
    if (IsKeyPressed(KEY_FIVE) && targetId_ != 0)
      net_->sendSkill(15, targetId_);
    if (IsKeyPressed(KEY_SIX) && targetId_ != 0)
      net_->sendSkill(16, targetId_);
    if (IsKeyPressed(KEY_SEVEN) && targetId_ != 0)
      net_->sendSkill(17, targetId_);
    if (IsKeyPressed(KEY_EIGHT)) net_->sendSkill(18, chanTarget());
  } else if (ctrlPage && !shiftPage) {
    if (IsKeyPressed(KEY_ONE) && targetId_ != 0)
      net_->sendSkill(19, targetId_);
    if (IsKeyPressed(KEY_TWO) && targetId_ != 0)
      net_->sendSkill(20, targetId_);
    if (IsKeyPressed(KEY_THREE)) net_->sendSkill(21, chanTarget());
    if (IsKeyPressed(KEY_FOUR) && targetId_ != 0)
      net_->sendSkill(22, targetId_);
    if (IsKeyPressed(KEY_FIVE)) net_->sendSkill(23, chanTarget());
  }
  // T-161: 6 = Resurrect at the click-selected fallen (server validates
  // cultist/L20/range/window/rebate). Plain page only (Shift+6 = Wither).
  if (!shiftPage && !ctrlPage && IsKeyPressed(KEY_SIX) && targetId_ != 0)
    net_->sendSkill(10, targetId_);
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
    // T-071: Marta's stock is size-driven (torch + lantern joined in T-071);
    // Sable's crate follows on F8–F10 so the lanes never share a key.
    int fi = 0;
    for (const std::uint32_t id : content::kVendorStock) {
      if (fi < 12 && IsKeyPressed(KEY_F1 + fi)) net_->sendBuy(id, 1);
      ++fi;
    }
    // T-069: Sable's crate — same BuyRequest path the server routes by item.
    for (int i = 0; i < 3; ++i) {
      if (IsKeyPressed(KEY_F8 + i)) {
        net_->sendBuy(content::kFenceStock[static_cast<size_t>(i)], 1);
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
  noteVestiges();  // T-066: capture last pose BEFORE the erase
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
    // T-ART-04: same id, breathing again (respawn) — drop the death pose.
    if (s.hp > 0 && re.animState == EntAnimState::kDie) {
      re.animState = EntAnimState::kNone;
    }
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
    } else if (cp.kind == 9) {
      // T-064 Blood Bolt: Gravemother's ranged cast — violet crimson over the CASTER
      {
        auto ai2 = rents_.find(cp.attacker);
        if (ai2 != rents_.end()) {
          const Vector2 ap = entRenderPos(ai2->second);
          f.x = ap.x;
          f.y = ap.y;
        }
      }
      f.text = "BLOOD BOLT " + std::to_string(cp.amount);
      f.r = 235;
      f.g = 40;
      f.b = 160;
    } else if (cp.kind == 13) {
      // T-066/T-054b: chorus — molten-gold crown over each sung member
      {
        auto ti = rents_.find(cp.target);
        if (ti != rents_.end()) { const Vector2 tp = entRenderPos(ti->second); f.x = tp.x; f.y = tp.y; }
      }
      f.text = "CHORUS +5%";
      f.r = 255; f.g = 180; f.b = 30;
    } else if (cp.kind == 14) {
      // T-054b: haste — amber streak over the quickened
      {
        auto ti = rents_.find(cp.target);
        if (ti != rents_.end()) { const Vector2 tp = entRenderPos(ti->second); f.x = tp.x; f.y = tp.y; }
      }
      f.text = "HASTE -25%";
      f.r = 255; f.g = 140; f.b = 30;
    } else if (cp.kind == 10) {
      // T-066: bless — gold over the anointed
      {
        auto ti = rents_.find(cp.target);
        if (ti != rents_.end()) { const Vector2 tp = entRenderPos(ti->second); f.x = tp.x; f.y = tp.y; }
      }
      f.text = "BLESS +10%";
      f.r = 255; f.g = 205; f.b = 40;
    } else if (cp.kind == 11) {
      // T-066: ironskin — steel-white over the armored
      {
        auto ti = rents_.find(cp.target);
        if (ti != rents_.end()) { const Vector2 tp = entRenderPos(ti->second); f.x = tp.x; f.y = tp.y; }
      }
      f.text = "IRONSKIN";
      f.r = 210; f.g = 215; f.b = 230;
    } else if (cp.kind == 12) {
      // T-066: party-share shimmer — warm gold xp over each sharer
      {
        auto ti = rents_.find(cp.target);
        if (ti != rents_.end()) { const Vector2 tp = entRenderPos(ti->second); f.x = tp.x; f.y = tp.y; }
      }
      f.text = "+" + std::to_string(cp.amount) + " xp";
      f.r = 255; f.g = 220; f.b = 120;
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
    } else if (cp.kind == 15) {
      // T-091: telegraph wind-up — gold warning over the marked victim.
      f.text = "!";
      f.r = 220;
      f.g = 180;
      f.b = 60;
    } else if (cp.kind == 16) {
      // T-091: slam strike lands — red-capped rot damage.
      f.text = "SLAM " + std::to_string(cp.amount);
      f.r = 255;
      f.g = 60;
      f.b = 30;
    } else if (cp.kind == 17) {
      // T-161b.1: sanctuary hold — choir-green ground vow.
      f.text = "SANCTUARY";
      f.r = 140;
      f.g = 235;
      f.b = 150;
    } else if (cp.kind == 18) {
      // T-161b.2: weakness laid on — sallow debility.
      f.text = "WEAKENED";
      f.r = 190;
      f.g = 170;
      f.b = 90;
    } else if (cp.kind == 19) {
      // T-161b.3: thrall rises — grave-muster over the caster.
      f.text = "THRALL RISES";
      f.r = 170;
      f.g = 220;
      f.b = 170;
    } else if (cp.kind == 21) {
      // T-161b.5: terror takes hold — white-eyed flight.
      f.text = "TERROR";
      f.r = 235;
      f.g = 235;
      f.b = 245;
    } else if (cp.kind == 22) {
      // T-161b.5: ward raised — blue hush over the caster.
      f.text = "MANA SHIELD";
      f.r = 140;
      f.g = 190;
      f.b = 255;
    } else if (cp.kind == 23) {
      // T-161b.5: rot laid on — sallow-green decay.
      f.text = "WITHERED";
      f.r = 150;
      f.g = 200;
      f.b = 120;
    } else if (cp.kind == 24) {
      // T-161b.6: bull rush — dust-brown charge.
      f.text = "BULL RUSH";
      f.r = 220;
      f.g = 170;
      f.b = 100;
    } else if (cp.kind == 25) {
      // T-161b.6: sundered plate — rust-red rend.
      f.text = "SUNDERED";
      f.r = 220;
      f.g = 110;
      f.b = 70;
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
    playCallout(cp.kind);  // T-067
  }
  // T-ART-04 combat anim hook: attacker snaps (contact frame first — the
  // pulse IS the resolution tick), victim hurt/dies. Victim-side wins ties.
  // Furniture never fights. Render-side only; authority untouched.
  for (const CombatPulse& cp : net_->combatIn) {
    const bool swing = cp.kind == 1 || cp.kind == 2;
    const bool cast = cp.kind == 5 || cp.kind == 9 || cp.kind == 10 ||
                      cp.kind == 13 || cp.kind == 14 || cp.kind == 15 ||
                      cp.kind == 17 || cp.kind == 18 || cp.kind == 19 ||
                      cp.kind == 21 || cp.kind == 22 || cp.kind == 23 ||
                      cp.kind == 24 || cp.kind == 25;
    const bool hurt = cp.kind == 1 || cp.kind == 2 || cp.kind == 5 ||
                      cp.kind == 9 || cp.kind == 16;
    const bool slain = cp.kind == 3;
    if (swing || cast) {
      auto ai = rents_.find(cp.attacker);
      if (ai != rents_.end() && !content::wireIsFurniture(ai->second.snap.kind)) {
        setAnimState(ai->second, swing ? EntAnimState::kAttack : EntAnimState::kCast, now);
      }
    }
    if (hurt || slain) {
      auto ti = rents_.find(cp.target);
      if (ti != rents_.end() && !content::wireIsFurniture(ti->second.snap.kind)) {
        setAnimState(ti->second, slain ? EntAnimState::kDie : EntAnimState::kHurt, now);
      }
    }
  }
  noteDecals();  // T-ART-09: kills bleed onto the decal surface (render-only)
  noteVfx();     // T-ART-14b: combat pulses play VFX strips (render-only)
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

void Game::setAnimState(RenderEnt& e, EntAnimState st, double now) {
  e.animState = st;
  e.animStateAt = now;
  const int ticks = animStateDurationTicks(st);
  e.animStateUntil = ticks < 0 ? now + 3600.0 : now + ticks / 20.0;
}

// T-ART-07: party-green overhead needs roster membership (own id excluded,
// which renders in the own-cream color instead).
bool Game::isPartyMember(std::uint32_t id) const {
  if (net_ == nullptr || !net_->welcomed || net_->partyId == 0) return false;
  if (id == net_->ownId) return false;
  for (const auto& m : net_->party) {
    if (m.entityId == id) return true;
  }
  return false;
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

void Game::bakeAudio() {  // T-067: presence pass 1
  if (IsAudioDeviceReady()) kit_.init();
}

void Game::playCallout(std::uint8_t kind) {  // T-067
  if (!kit_.ready) return;
  switch (kind) {
    case 1:                                  // hit
    case 2: PlaySound(kit_.hit); break;      // crit (thud + "!")
    case 3: PlaySound(kit_.death); break;    // SLAIN
    case 5: PlaySound(kit_.swing); break;    // Power-Swing
    case 6: PlaySound(kit_.toll); break;     // anvil ceremony
    case 8:                                  // mend
    case 10: PlaySound(kit_.choir); break;   // bless
    case 9: PlaySound(kit_.bolt); break;     // BLOOD BOLT
    case 11: PlaySound(kit_.toll); break;    // ironskin (armor-set ring)
    case 13: PlaySound(kit_.choir); break;    // chorus (the whole song at once)
    case 17: PlaySound(kit_.choir); break;    // sanctuary (a held note)
    case 18: PlaySound(kit_.bolt); break;     // weakness (thin dark)
    case 19: PlaySound(kit_.bolt); break;     // thrall (the mud answers)
    case 21: PlaySound(kit_.bolt); break;     // terror (a cold draft)
    case 22: PlaySound(kit_.choir); break;    // ward (a held breath)
    case 23: PlaySound(kit_.bolt); break;     // wither (damp rot)
    case 24: PlaySound(kit_.swing); break;    // rush (shoulder-first)
    case 25: PlaySound(kit_.swing); break;    // sunder (plate screams)
    case 14: PlaySound(kit_.swing); break;   // haste (steel quickens)
    default: break;                          // misses/xp shimmer: quiet
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
  skin_.ensureFor(loadedMapId, map_);  // T-ART-12: no-op when baked
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
  skin_.drawLayer();  // T-ART-12: textured diamonds+edges over the flat underlay
  bool heroDrawn = false;
  if (net_ == nullptr) {
    const int heroRow = static_cast<int>(std::floor(walker_.fy() + 0.5f));
    for (int y = 0; y < map_.h; ++y) {
      for (int x = 0; x < map_.w; ++x) {
        const size_t i = static_cast<size_t>(y) * static_cast<size_t>(map_.w) +
                         static_cast<size_t>(x);
        if (map_.ground[i] != 2) continue;
        const Vector2 c = iso::tileToWorld(x, y, map_.tileW, map_.tileH);
        if (skin_.prismReady() && !skin_.flatForced())
          skin_.drawPrism(c.x, c.y, map_.tileW, map_.tileH, 28,
                          terrainVariant(x, y, 3));
        else
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
        if (skin_.prismReady() && !skin_.flatForced())
          skin_.drawPrism(c.x, c.y, map_.tileW, map_.tileH, 28,
                          terrainVariant(x, y, 3));
        else
          iso::drawPrism(c, map_.tileW, map_.tileH, 28, Color{120, 120, 128, 255},
                         Color{74, 74, 82, 255}, Color{92, 92, 100, 255});
      }
    }
    drawDecals();  // T-ART-09: decal surface sits under entities, never over
    drawEntitiesOnline();
  }
}

void Game::drawHero() {
  const Vector2 w = iso::tileToWorldF(Vector2{walker_.fx(), walker_.fy()}, map_.tileW,
                                      map_.tileH);
  const char* anim = walker_.moving ? "walk" : "idle";
  const Rectangle src = animFrame(heroAtlas_, anim, walker_.dir, animT_);
  if (src.width <= 0.0f) {
    DrawCircleV(w, 8.0f, RED);
    return;
  }
  DrawTexturePro(heroAtlas_.tex, src, Rectangle{w.x, w.y, src.width, src.height},
                 Vector2{src.width * 0.5f, animAnchorY(heroAtlas_, anim)}, 0.0f, WHITE);
}

void Game::drawRemoteEnt(const RenderEnt& e, bool isOwn) {
  const Vector2 rp = entRenderPos(e);
  const Vector2 w = iso::tileToWorldF(rp, map_.tileW, map_.tileH);
  if (content::wireIsFurniture(e.snap.kind)) {
    // T-ART-B5: if a sheet shipped for this furniture kind, draw it the same
    // way as mobs (atlasFor loads idle anim; feet on anchorY). Otherwise
    // fall back to the existing in-code rectangle stub (Marta 64, anvil 65,
    // bounty board 66 still use their hand-coded placeholders).
    const Atlas& at = atlasFor(e.snap.kind);
    if (at.ok) {
      Rectangle src = animFrame(at, "idle", static_cast<int>(e.snap.dir), animT_);
      if (src.width > 0.0f) {
        DrawTexturePro(at.tex, src,
                       Rectangle{w.x, w.y, src.width, src.height},
                       Vector2{src.width * 0.5f, animAnchorY(at, "idle")},
                       0.0f, WHITE);
        // T-ART-13 bitmap tag (R-TEXT-2 degrades NPC piles to badges).
        drawNameTag(w, e.snap.name, Color{190, 210, 220, 255}, -58, e.snap.id, true);
        return;
      }
    }
    // furniture band (T-047 constants): widow anvil 65 keeps its teal rig;
    // every other kind uses the generic stall rect + name label.
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
      // T-ART-13 bitmap tag (R-TEXT-2 degrades NPC piles to badges).
      drawNameTag(w, e.snap.name, Color{190, 210, 220, 255}, -52, e.snap.id, true);
    }
    return;
  }
  if (e.snap.id == targetId_) {
    DrawEllipseLines(static_cast<int>(w.x), static_cast<int>(w.y), 14.0f, 7.0f,
                     Color{220, 40, 40, 230});
  }
  const double now = GetTime();
  // T-ART-04: combat anim wins while its transient runs (die holds until the
  // corpse leaves); state clock starts at frame 0 on the resolution tick.
  EntAnimState st = e.animState;
  if (st != EntAnimState::kDie && st != EntAnimState::kNone && now > e.animStateUntil) {
    st = EntAnimState::kNone;
  }
  const char* anim = animNameFor(st, e.snap.moving);
  double animClock = animT_;
  if (st != EntAnimState::kNone) animClock = now - e.animStateAt;
  // T-ART-05: per-kind atlas (mob sheets; hero fallback). T-ART-10: feet
  // anchor rides the used anim (42.0 legacy default inside animAnchorY).
  // T-142: players (kind 0) re-sheet from classId/sex; mobs/furniture use
  // the mob atlases. Unknown class/sex falls back to hero (T-142b).
  const Atlas& at = [&]() -> const Atlas& {
    if (e.snap.kind == 0) return atlasForPlayer(e.snap.classId, e.snap.sex);
    return atlasFor(e.snap.kind);
  }();
  const char* fallback = e.snap.moving ? "walk" : "idle";
  Rectangle src = animFrame(at, anim, static_cast<int>(e.snap.dir), animClock);
  const char* used = anim;
  if ((src.width <= 0.0f) && st != EntAnimState::kNone) {
    // atlas predates combat frames: fall back to the locomotion frame.
    src = animFrame(at, fallback, static_cast<int>(e.snap.dir), animT_);
    used = fallback;
  }
  const Color tint = isOwn ? Color{255, 255, 255, 255} : Color{190, 190, 200, 255};
  // T-092: equipped refine glows in-world (tier rides snap.glowTier, alpha
  // capped by the T-ART-11 law; mythic reads paler + wider, not brighter).
  if (e.snap.glowTier > 0) {
    const bool mythic = e.snap.glowTier >= 2;
    DrawEllipse(static_cast<int>(w.x), static_cast<int>(w.y), mythic ? 16 : 12,
                mythic ? 7 : 5,
                mythic ? Color{255, 240, 200, 70} : Color{255, 210, 110, 70});
  }
  if (src.width > 0.0f) {
    DrawTexturePro(at.tex, src, Rectangle{w.x, w.y, src.width, src.height},
                   Vector2{src.width * 0.5f, animAnchorY(at, used)}, 0.0f, tint);
  } else {
    DrawCircleV(w, 8.0f, MAROON);
  }
  {
    std::string label = e.snap.name;
    if (!label.empty() && e.snap.level > 1) {
      label = label + " L" + std::to_string(e.snap.level);
    }
    if (!label.empty()) {
      Color nc = isOwn ? Color{230, 210, 190, 255} : Color{190, 190, 200, 255};
      if (!isOwn) {
        switch (resolveNameTint(false, e.snap.karmaBand, isPartyMember(e.snap.id))) {
          case NameTint::kChaotic: nc = Color{235, 60, 50, 255}; break;
          case NameTint::kParty: nc = Color{120, 235, 130, 255}; break;
          case NameTint::kLawful: nc = Color{170, 200, 255, 255}; break;
          case NameTint::kNeutral: break;
        }
      }
      // T-ART-13 bitmap names (5x7 small caps stay quiet — D4); the pledge
      // plate hangs off the measured width either way. R-TEXT-2 degrades
      // piles of 4+ (own name never degrades).
      const int tw =
          nameFont_.ok() ? nameFont_.measure(label) : MeasureText(label.c_str(), 10);
      drawNameTag(w, label, nc, nameFont_.ok() ? -58 : -52, e.snap.id, !isOwn);
      if (isPledgeMemberName(e.snap.name)) {
        const int bx = static_cast<int>(w.x) + tw / 2 + 4;
        const int by = static_cast<int>(w.y) - 62;
        DrawRectangle(bx, by, 10, 10, Color{160, 140, 90, 230});
        DrawRectangleLines(bx, by, 10, 10, Color{200, 180, 120, 255});
        char eb[4]; std::snprintf(eb, sizeof eb, "%u", net_->pledgeEmblem % 10);
        DrawText(eb, bx + 2, by + 1, 10, Color{40, 30, 20, 255});
      }
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
  ensureFonts();
  std::vector<const RenderEnt*> order;
  order.reserve(rents_.size());
  for (const auto& kv : rents_) order.push_back(&kv.second);
  std::sort(order.begin(), order.end(), [&](const RenderEnt* a, const RenderEnt* b) {
    return entRenderPos(*a).y < entRenderPos(*b).y;
  });
  // R-TEXT-2 pile test on tag rects (tags are wide centered texts — anchors
  // alone miss the overlap; same measure the tags draw with below).
  namePileCounts_.clear();
  struct TagRect {
    std::uint32_t id = 0;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  };
  std::vector<TagRect> tags;
  tags.reserve(order.size());
  for (const RenderEnt* e : order) {
    std::string label = e->snap.name;
    if (!content::wireIsFurniture(e->snap.kind) && !label.empty() && e->snap.level > 1)
      label = label + " L" + std::to_string(e->snap.level);
    if (label.empty()) continue;
    const Vector2 w = iso::tileToWorldF(entRenderPos(*e), map_.tileW, map_.tileH);
    const Vector2 s = GetWorldToScreen2D(w, rig_.cam);
    const float tw = nameFont_.ok() ? static_cast<float>(nameFont_.measure(label))
                                    : static_cast<float>(MeasureText(label.c_str(), 10));
    tags.push_back(TagRect{e->snap.id, s.x - tw / 2.0f - 4.0f, s.y - 62.0f,
                           s.x + tw / 2.0f + 4.0f, s.y - 48.0f});
  }
  for (const TagRect& a : tags) {
    int n = 0;
    for (const TagRect& b : tags) {
      if (a.id == b.id) continue;
      if (a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1) ++n;
    }
    namePileCounts_[a.id] = n;
  }
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

const Atlas& Game::atlasFor(std::uint8_t kind) {
  // T-ART-05 + T-ART-B5: per-kind atlas. Mob sheets live under aigen/mobs/<id>_<slug>/;
  // furniture/NPC sheets (T-ART-B5 procedural placeholders, later B5 AI plates)
  // live under aigen/npcs/<slug>/. Anything without a shipped sheet falls back
  // to the hero placeholder; the furniture rectangle stub in drawRemoteEnt is
  // the final fallback only when no atlas loaded.
  char png[160], js[160];
  bool have_path = mobSheetPaths(kind, png, sizeof png, js, sizeof js) ||
                   furnitureSheetPaths(kind, png, sizeof png, js, sizeof js);
  if (!have_path) return heroAtlas_;
  auto cached = mobAtlases_.find(kind);
  if (cached != mobAtlases_.end()) return cached->second;
  Atlas a;
  if (loadAtlas(png, js, a) && a.ok) {
    mobAtlases_.emplace(kind, std::move(a));
    return mobAtlases_.at(kind);
  }
  // No shipped sheet (or headless asset dir): hero fallback. The red-circle
  // QA marker path is unchanged for frames missing everywhere.
  return heroAtlas_;
}

const Atlas& Game::atlasForPlayer(std::uint8_t classId, std::uint8_t sex) {
  // T-142: per-(class,sex) player atlas. Ravager m/f ship today; unknown
  // class/sex or missing sheet falls back to hero (T-142b captures sex).
  char png[160], js[160];
  if (!playerSheetPaths(classId, sex, png, sizeof png, js, sizeof js)) return heroAtlas_;
  const std::uint16_t key = (static_cast<std::uint16_t>(classId) << 8) | sex;
  auto cached = playerAtlases_.find(key);
  if (cached != playerAtlases_.end()) return cached->second;
  Atlas a;
  if (loadAtlas(png, js, a) && a.ok) {
    playerAtlases_.emplace(key, std::move(a));
    return playerAtlases_.at(key);
  }
  return heroAtlas_;
}

const char* Game::mapFileFor(std::uint16_t mapId) {  switch (mapId) {
    case 2: return "assets/maps/fields_overflow.bhmap";
    case 3: return "assets/maps/thornwall_crypt.bhmap";
    case 4: return "assets/maps/bonehowl_mine.bhmap";   // T-ART-08
    case 5: return "assets/maps/drowned_crypt.bhmap";   // T-ART-08
    case 6: return "assets/maps/weeping_castle.bhmap";  // T-150: Weeping Castle
    default: return "assets/maps/thornwall.bhmap";
  }
}

std::uint16_t Game::mapIdForFile(const std::string& path) {
  if (path.find("thornwall_crypt") != std::string::npos) return 3;
  if (path.find("fields_overflow") != std::string::npos) return 2;
  if (path.find("bonehowl_mine") != std::string::npos) return 4;
  if (path.find("drowned_crypt") != std::string::npos) return 5;
  if (path.find("weeping_castle") != std::string::npos) return 6;
  return 1;  // thornwall.bhmap default (check crypt first: name contains it)
}

bool Game::zoneReload(std::uint16_t mapId) {  if (mapId == loadedMapId) return true;
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
  decals_.clear();  // T-ART-09: decals are per-zone surface state
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

// T-066 petrify-fade: on despawn, keep the departed silhouette 0.6s.
// Furniture (vendor/anvil/board) never ghosts — only combatants.
void Game::noteVestiges() {
  if (net_ == nullptr) return;
  const double now = GetTime();
  for (const std::uint32_t id : net_->despawnedIds) {
    auto it = rents_.find(id);
    if (it == rents_.end()) continue;
    const RenderEnt& re = it->second;
    if (re.snap.kind >= 64) continue;  // furniture floor: no ghosts
    Vestige v;
    const Vector2 pos = entRenderPos(re);  // tile-space, interpolated
    v.x = pos.x;
    v.y = pos.y;
    v.kind = re.snap.kind;
    v.at = now;
    vestiges_.push_back(v);
    while (vestiges_.size() > 16) vestiges_.pop_front();
  }
}

void Game::drawVestiges() {
  const double now = GetTime();
  while (!vestiges_.empty() && now - vestiges_.front().at > 0.6) vestiges_.pop_front();
  for (const Vestige& v : vestiges_) {
    const double age = now - v.at;
    const auto a = static_cast<unsigned char>(140 * (1.0 - age / 0.6));
    const Vector2 w =
        iso::tileToWorldF(Vector2{v.x, v.y}, map_.tileW, map_.tileH);
    // era ghost: a dim tombstone-grey silhouette circle settling into dark
    const float sink = static_cast<float>(age) * 6.0f;
    DrawEllipse(static_cast<int>(w.x), static_cast<int>(w.y) - 18 + static_cast<int>(sink),
                8, 12, Color{150, 150, 160, a});
    DrawEllipse(static_cast<int>(w.x), static_cast<int>(w.y), 12, 5,
                Color{90, 90, 100, a});
  }
}

void Game::ensureFonts() {
  if (fontsTried_) return;
  fontsTried_ = true;
  // Missing files -> !ok() -> legacy DrawText path (never holes).
  calloutFont_.load("assets/aigen/ui/font/callout_font_cap11.png",
                    "assets/aigen/ui/font/callout_font_cap11.json");
  nameFont_.load("assets/aigen/ui/font/callout_font_cap7.png",
                 "assets/aigen/ui/font/callout_font_cap7.json");
}

void Game::drawNameTag(Vector2 w, const std::string& name, Color nc, int yOff,
                         std::uint32_t id, bool canDegrade) {
  if (name.empty()) return;
  int pile = 0;
  auto pc = namePileCounts_.find(id);
  if (pc != namePileCounts_.end()) pile = pc->second;
  const int iy = static_cast<int>(w.y) + yOff;
  if (canDegrade && namePileDegrades(pile)) {
    DrawPoly(Vector2{w.x, static_cast<float>(iy)}, 4, 5, 0.0f, nc);
    return;
  }
  if (nameFont_.ok()) {
    const int tw = nameFont_.measure(name);
    nameFont_.drawText(name, static_cast<int>(w.x) - tw / 2, iy, nc);
  } else {
    const int tw = MeasureText(name.c_str(), 10);
    DrawText(name.c_str(), static_cast<int>(w.x) - tw / 2, iy, 10, nc);
  }
}

const Atlas* Game::vfxStrip(const std::string& name) {
  auto it = vfxAtlases_.find(name);
  if (it != vfxAtlases_.end()) return it->second.ok ? &it->second : nullptr;
  Atlas a;
  const std::string base = "assets/aigen/vfx/" + name + "/";
  if (loadAtlas(base + "strip.png", base + "strip.json", a) && a.ok) {
    auto em = vfxAtlases_.emplace(name, std::move(a));
    return &em.first->second;
  }
  vfxAtlases_.emplace(name, Atlas{});  // missing: negative cache, no retry spam
  return nullptr;
}

void Game::noteVfx() {
  if (net_ == nullptr) return;
  const double now = GetTime();
  for (const CombatPulse& cp : net_->combatIn) {
    // Tier-0 mapping (kinds follow the floater table above): melee 1/2 and
    // Power Swing 5 -> swing arc at the victim; Blood Bolt 9 -> impact at
    // the victim; mend 8 -> motes at the recipient; bless-group casts
    // 10/13/14 -> ground ring under the caster. Slain is already a decal.
    const char* strip = nullptr;
    bool atCaster = false;
    if (cp.kind == 1 || cp.kind == 2 || cp.kind == 5)
      strip = "swing_arc";
    else if (cp.kind == 9)
      strip = "firebolt_impact";
    else if (cp.kind == 8)
      strip = "mend_motes";
    else if (cp.kind == 10 || cp.kind == 13 || cp.kind == 14) {
      strip = "cast_ring";
      atCaster = true;
    }
    if (strip == nullptr) continue;
    const Atlas* at = vfxStrip(strip);
    if (at == nullptr) continue;
    auto it = at->anims.find("play");
    if (it == at->anims.end() || it->second.dirFrames[0].empty() || it->second.fps <= 0)
      continue;
    const auto& e = atCaster ? rents_.find(cp.attacker) : rents_.find(cp.target);
    if (e == rents_.end()) continue;
    const Vector2 p = entRenderPos(e->second);
    VfxPlay v;
    v.strip = strip;
    v.x = p.x;
    v.y = p.y;
    v.at = now;
    v.dur = static_cast<float>(it->second.dirFrames[0].size()) / it->second.fps;
    vfxPlays_.push_back(std::move(v));
    while (vfxPlays_.size() > 16) vfxPlays_.pop_front();
  }
}

void Game::drawVfxPlays() {
  if (vfxPlays_.empty()) return;
  const double now = GetTime();
  while (!vfxPlays_.empty() && now - vfxPlays_.front().at > vfxPlays_.front().dur)
    vfxPlays_.pop_front();
  for (const VfxPlay& v : vfxPlays_) {
    const Atlas* at = vfxStrip(v.strip);
    if (at == nullptr) continue;
    const Rectangle src = animFrame(*at, "play", 0, now - v.at);
    if (src.width <= 0.0f) continue;
    const Vector2 w = iso::tileToWorldF(Vector2{v.x, v.y}, map_.tileW, map_.tileH);
    DrawTexturePro(at->tex, src, Rectangle{w.x, w.y - 8.0f, src.width, src.height},
                   Vector2{src.width * 0.5f, src.height * 0.5f}, 0.0f, WHITE);
  }
}

void Game::drawVfxTest() {
  if (!vfxTest_) return;
  if (!testVfx_.ok) {
    Atlas a;
    if (!loadAtlas("assets/aigen/vfx/test/castring/strip.png",
                   "assets/aigen/vfx/test/castring/strip.json", a) ||
        !a.ok)
      return;
    testVfx_ = std::move(a);
  }
  Vector2 hero;
  if (net_ != nullptr) {
    const auto own = rents_.find(net_->ownId);
    hero = own != rents_.end()
               ? iso::tileToWorldF(entRenderPos(own->second), map_.tileW, map_.tileH)
               : iso::tileToWorld(map_.w / 2, map_.h / 3, map_.tileW, map_.tileH);
  } else {
    hero = iso::tileToWorldF(Vector2{walker_.fx(), walker_.fy()}, map_.tileW, map_.tileH);
  }
  // T-ART-14b: the tier-0 event strips play fanned around the hero (same
  // vfxStrip loader + animFrame playback the event feed uses).
  static const Vector2 kOffsets[] = {{-56, 0}, {56, 0}, {0, -44}, {0, 40}};
  static const char* kStrips[] = {"swing_arc", "firebolt_impact", "mend_motes",
                                  "cast_ring"};
  for (int i = 0; i < 4; ++i) {
    const Atlas* at = vfxStrip(kStrips[i]);
    if (at == nullptr) continue;
    const Rectangle src = animFrame(*at, "play", 0, animT_);
    if (src.width <= 0.0f) continue;
    const Vector2 c{hero.x + kOffsets[i].x, hero.y + kOffsets[i].y};
    DrawTexturePro(at->tex, src, Rectangle{c.x, c.y, src.width, src.height},
                   Vector2{src.width * 0.5f, src.height * 0.5f}, 0.0f, WHITE);
  }
  const Rectangle src = animFrame(testVfx_, "play", 0, animT_);
  if (src.width <= 0.0f) return;
  DrawTexturePro(testVfx_.tex, src, Rectangle{hero.x, hero.y, src.width, src.height},
                 Vector2{src.width * 0.5f, src.height * 0.5f}, 0.0f, WHITE);
}

void Game::drawFloaters() {
  ensureFonts();
  const double now = GetTime();
  while (!floaters_.empty() && now - floaters_.front().at > 0.9) floaters_.pop_front();
  for (const Floater& f : floaters_) {
    const double age = now - f.at;
    const float rise = static_cast<float>(age) * 28.0f;
    const auto a = static_cast<unsigned char>(255 * (1.0 - age / 0.9));
    const Vector2 w = iso::tileToWorldF(Vector2{f.x, f.y}, map_.tileW, map_.tileH);
    const Color col{f.r, f.g, f.b, a};
    if (calloutFont_.ok()) {
      const int tw = calloutFont_.measure(f.text);
      calloutFont_.drawText(f.text, static_cast<int>(w.x) - tw / 2,
                            static_cast<int>(w.y) - 56 - static_cast<int>(rise), col);
    } else {
      DrawText(f.text.c_str(), static_cast<int>(w.x) - 8,
               static_cast<int>(w.y) - 56 - static_cast<int>(rise), 10, col);
    }
  }
}

// T-ART-09: kills bleed onto the decal surface (render-only — the pulse is
// already authoritative; this only stains the ground). Victims already
// despawned this frame have no pos to stain and are skipped (stated).
void Game::noteDecals() {
  if (net_ == nullptr) return;
  for (const CombatPulse& cp : net_->combatIn) {
    if (cp.kind != 3 && cp.kind != 15 && cp.kind != 17) continue;  // slain + slam wind-up + sanctuary
    auto ti = rents_.find(cp.target);
    if (ti == rents_.end()) continue;
    if (content::wireIsFurniture(ti->second.snap.kind)) continue;
    const Vector2 p = entRenderPos(ti->second);
    Decal d;
    d.x = p.x;
    d.y = p.y;
    // T-091: wind-up stains stage 1 at the victim's tile; the draw path
    // advances 1-2-3 by age against the server's 60t fuse (shared clock).
    // T-161b.1: sanctuary rings a persistent circle at the holder's tile
    // (decal law: circles hold until zone reload).
    d.kind = cp.kind == 15 ? DecalKind::kTelegraph1
             : cp.kind == 17 ? DecalKind::kCircle
                             : DecalKind::kBlood;
    d.bornTick = static_cast<std::int64_t>(tick_);
    decals_.push_back(d);
    while (decals_.size() > kDecalCap) decals_.pop_front();
  }
}

void Game::addTelegraph(float x, float y, int stage) {
  Decal d;
  d.x = x;
  d.y = y;
  d.kind = stage <= 1 ? DecalKind::kTelegraph1
           : stage == 2 ? DecalKind::kTelegraph2
                        : DecalKind::kTelegraph3;
  d.bornTick = static_cast<std::int64_t>(tick_);
  decals_.push_back(d);
  while (decals_.size() > kDecalCap) decals_.pop_front();
}

void Game::addCircle(float x, float y) {
  Decal d;
  d.x = x;
  d.y = y;
  d.kind = DecalKind::kCircle;
  d.bornTick = static_cast<std::int64_t>(tick_);
  decals_.push_back(d);
  while (decals_.size() > kDecalCap) decals_.pop_front();
}

// T-ART-09: decal surface — expired decals drained, survivors y-sorted so
// blood sorts under entities the same way entities sort among themselves.
// Called inside drawGround() before drawEntitiesOnline (under, not over).
void Game::drawDecals() {
  const std::int64_t now = static_cast<std::int64_t>(tick_);
  // T-091: telegraphs drain on the 80t visible window (staging advances by
  // age); circles never drain; blood drains on its own TTL.
  while (!decals_.empty() && !decalVisible(decals_.front(), now) &&
         decals_.front().kind != DecalKind::kCircle)
    decals_.pop_front();
  std::vector<const Decal*> order;
  order.reserve(decals_.size());
  for (const Decal& d : decals_) {
    if (!decalVisible(d, now)) continue;
    order.push_back(&d);
  }
  std::sort(order.begin(), order.end(),
            [](const Decal* a, const Decal* b) { return a->y < b->y; });
  for (const Decal* d : order) {
    const auto a = static_cast<unsigned char>(decalAlpha(*d, now));
    const Vector2 w =
        iso::tileToWorldF(Vector2{d->x, d->y}, map_.tileW, map_.tileH);
    // T-091: wind-up decals draw by effective stage (1-2-3 by age), so one
    // journaled event animates the full fuse. Hand-placed stages (boss API)
    // draw as placed.
    int stage = 0;
    if (d->kind == DecalKind::kTelegraph1 || d->kind == DecalKind::kTelegraph2 ||
        d->kind == DecalKind::kTelegraph3)
      stage = telegraphStage(*d, now);
    DecalKind drawKind = d->kind;
    if (stage == 1) drawKind = DecalKind::kTelegraph1;
    if (stage == 2) drawKind = DecalKind::kTelegraph2;
    if (stage == 3) drawKind = DecalKind::kTelegraph3;
    switch (drawKind) {
      case DecalKind::kBlood:
        DrawEllipse(static_cast<int>(w.x), static_cast<int>(w.y), 10, 4,
                    Color{140, 20, 20, a});
        DrawEllipse(static_cast<int>(w.x) + 4, static_cast<int>(w.y) + 1, 5, 2,
                    Color{110, 15, 15, a});
        break;
      case DecalKind::kTelegraph1:
        DrawEllipseLines(static_cast<int>(w.x), static_cast<int>(w.y), 14, 7,
                         Color{220, 180, 60, a});
        break;
      case DecalKind::kTelegraph2:
        DrawEllipseLines(static_cast<int>(w.x), static_cast<int>(w.y), 14, 7,
                         Color{230, 90, 40, a});
        DrawEllipse(static_cast<int>(w.x), static_cast<int>(w.y), 14, 7,
                    Color{230, 90, 40, static_cast<unsigned char>(a / 4)});
        break;
      case DecalKind::kTelegraph3:
        DrawEllipse(static_cast<int>(w.x), static_cast<int>(w.y), 14, 7,
                    Color{255, 60, 30, a});
        break;
      case DecalKind::kCircle:
        DrawEllipseLines(static_cast<int>(w.x), static_cast<int>(w.y), 18, 9,
                         Color{150, 220, 220, a});
        DrawEllipseLines(static_cast<int>(w.x), static_cast<int>(w.y), 12, 6,
                         Color{150, 220, 220, a});
        break;
    }
  }
}

void Game::ensureSkillIcons() const {  if (skillIconsTried_) return;
  skillIconsTried_ = true;
  // Missing files -> placeholder plates below (never holes).
  try {
    const char* png = "assets/aigen/icons/skills/skill_icons.png";
    const char* js = "assets/aigen/icons/skills/skill_icons.json";
    if (!FileExists(png) || !FileExists(js)) return;
    Texture2D tex = LoadTexture(png);
    if (tex.id == 0) return;
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    std::ifstream f(js);
    nlohmann::json j;
    f >> j;
    for (auto it = j.at("icons").begin(); it != j.at("icons").end(); ++it) {
      const auto& c = it.value();
      const int channel = c.at("channel").get<int>();
      if (channel < 1 || channel > 23) continue;
      skillIconRects_.emplace(static_cast<std::uint8_t>(channel),
                              Rectangle{static_cast<float>(c.at("x").get<int>()),
                                        static_cast<float>(c.at("y").get<int>()),
                                        static_cast<float>(c.at("w").get<int>()),
                                        static_cast<float>(c.at("h").get<int>())});
    }
    skillIcons_ = tex;
  } catch (...) {
    if (skillIcons_.id != 0) UnloadTexture(skillIcons_);
    skillIcons_ = Texture2D{};
    skillIconRects_.clear();
  }
}

void Game::ensureItemIcons() const {
  if (itemIconsTried_) return;
  itemIconsTried_ = true;
  // Missing files -> rarity-plate fallback below (never holes).
  try {
    const char* png = "assets/aigen/icons/items/item_icons.png";
    const char* js = "assets/aigen/icons/items/item_icons.json";
    if (!FileExists(png) || !FileExists(js)) return;
    Texture2D tex = LoadTexture(png);
    if (tex.id == 0) return;
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    std::ifstream f(js);
    nlohmann::json j;
    f >> j;
    for (auto it = j.at("icons").begin(); it != j.at("icons").end(); ++it) {
      const auto& c = it.value();
      const std::uint32_t id = static_cast<std::uint32_t>(c.at("itemId").get<int>());
      itemIconRects_.emplace(id, Rectangle{static_cast<float>(c.at("x").get<int>()),
                                           static_cast<float>(c.at("y").get<int>()),
                                           static_cast<float>(c.at("w").get<int>()),
                                           static_cast<float>(c.at("h").get<int>())});
    }
    itemIcons_ = tex;
  } catch (...) {
    if (itemIcons_.id != 0) UnloadTexture(itemIcons_);
    itemIcons_ = Texture2D{};
    itemIconRects_.clear();
  }
}

bool Game::drawItemIcon(std::uint32_t itemId, int x, int y, int size) const {
  ensureItemIcons();
  if (itemIcons_.id == 0) return false;
  auto it = itemIconRects_.find(itemId);
  if (it == itemIconRects_.end()) return false;
  DrawTexturePro(itemIcons_, it->second,
                 Rectangle{static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(size), static_cast<float>(size)},
                 Vector2{0, 0}, 0.0f, WHITE);
  return true;
}

void Game::drawHotbar() const {
  // T-ART-15 skill bar: one row always visible online, following the live
  // input page (plain 1-6 / Shift+1-8 / Ctrl+1-5 — the T-161b layout in
  // handleInputOnline). Slots show lock state from kits.h unlocks at the
  // hero's kit+level (server re-validates casts; this never decides).
  // Icons ride the aigen sheet when it ships (placeholder plates before);
  // cooldown sweep needs cooldowns on the wire (not carried — owed).
  if (net_ == nullptr || !net_->welcomed) return;
  ensureSkillIcons();
  const bool shiftPage = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  const bool ctrlPage = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  std::uint8_t ch[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  const char* key[8] = {"", "", "", "", "", "", "", ""};
  const char* page = "";
  if (!shiftPage && !ctrlPage) {
    const std::uint8_t c[] = {1, 2, 3, 4, 5, 10, 0, 0};
    const char* k[] = {"1", "2", "3", "4", "5", "6", "", ""};
    for (int i = 0; i < 8; ++i) {
      ch[i] = c[i];
      key[i] = k[i];
    }
  } else if (shiftPage && !ctrlPage) {
    const char* k[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
    for (int i = 0; i < 8; ++i) {
      ch[i] = static_cast<std::uint8_t>(11 + i);
      key[i] = k[i];
    }
    page = "SHIFT";
  } else if (ctrlPage && !shiftPage) {
    const char* k[] = {"1", "2", "3", "4", "5", "", "", ""};
    for (int i = 0; i < 5; ++i) {
      ch[i] = static_cast<std::uint8_t>(19 + i);
      key[i] = k[i];
    }
    page = "CTRL";
  } else {
    return;  // Shift+Ctrl: no page (matches input — no casts fire)
  }
  // §11 backing families by channel (later channels grouped by effect).
  auto family = [](std::uint8_t c) -> Color {
    switch (c) {
      case 2:
      case 7:
      case 13:
      case 23: return Color{232, 228, 220, 255};  // bone: heal
      case 3:
      case 6:
      case 10:
      case 11: return Color{200, 170, 60, 255};  // gold: holy
      case 4:
      case 8:
      case 9:
      case 18: return Color{120, 140, 170, 255};  // ash: utility
      case 12:
      case 16:
      case 17: return Color{140, 100, 170, 255};  // violet: curse
      default: return Color{200, 60, 40, 255};    // iron: damage
    }
  };
  const std::uint8_t kit = net_->ownStats.classId;
  const std::uint16_t lvl = net_->ownStats.level;
  const int n = 8, sw = 30;
  const int x0 = (GetScreenWidth() - n * sw) / 2;
  const int y0 = GetScreenHeight() - 40;
  if (page[0] != '\0') DrawText(page, x0 - 44, y0 + 8, 10, Color{140, 140, 140, 255});
  for (int i = 0; i < n; ++i) {
    const int x = x0 + i * sw;
    DrawRectangle(x + 1, y0, 28, 28, Color{12, 10, 10, 220});
    DrawRectangleLines(x + 1, y0, 28, 28, Color{60, 50, 50, 255});
    if (ch[i] == 0) continue;
    // icon art when the sheet covers the channel, else the §11 plate.
    auto ic = skillIconRects_.find(ch[i]);
    if (skillIcons_.id != 0 && ic != skillIconRects_.end()) {
      DrawTexturePro(skillIcons_, ic->second,
                     Rectangle{static_cast<float>(x + 4), static_cast<float>(y0 + 3),
                               22, 22},
                     Vector2{0, 0}, 0.0f, WHITE);
    } else {
      DrawRectangle(x + 4, y0 + 3, 22, 22, Color{21, 16, 19, 255});
      DrawRectangleLines(x + 4, y0 + 3, 22, 22, family(ch[i]));
    }
    // key numeral stays legible over icon art (1px outline, same as fonts)
    DrawText(key[i], x + 5, y0 + 2, 10, Color{0, 0, 0, 255});
    DrawText(key[i], x + 4, y0 + 1, 10, Color{230, 210, 190, 255});
    const std::uint8_t req = content::kitSkillUnlock(kit, ch[i]);
    if (req == 0 || lvl < req) {
      DrawRectangle(x + 1, y0, 28, 28, Color{8, 8, 10, 150});  // locked dim
      if (req > 0) {
        char rl[8];
        std::snprintf(rl, sizeof rl, "%u", static_cast<unsigned>(req));
        DrawText(rl, x + 18, y0 + 16, 10, Color{150, 140, 130, 255});
      }
    }
  }
}

void Game::drawStatPanel() const {
  if (net_ == nullptr || !net_->welcomed) return;
  const OwnStatsWire& st = net_->ownStats;
  const int px = 1024 - 190;
  // kit block grows: mp row always, buff rows only while active (era chrome)
  const bool showBuffs = st.blessTicksLeft > 0 || st.ironskinTicksLeft > 0;
  const bool showCurse = st.curseTicksLeft > 0;  // T-070: own row, violet
  const int ph =
      (st.statPoints > 0 ? 106 : 88) + 40 + (showBuffs ? 12 : 0) + (showCurse ? 12 : 0);
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
  std::snprintf(buf, sizeof buf, "INT %u  MAG %u", st.intg, st.mag);  // T-160
  DrawText(buf, px + 8, 66, 10, LIGHTGRAY);
  if (st.statPoints > 0) {
    std::snprintf(buf, sizeof buf, "+%u pts: F5 STR F6 VIT F7 DEX F8 INT F9 MAG", st.statPoints);
    DrawText(buf, px + 8, 80, 10, Color{255, 200, 80, 255});
  }
  // T-053/54: mana bar + buff countdowns (kit resource legibility)
  const int by = (st.statPoints > 0 ? 100 : 84) + 2;
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
  // T-070: the thin blood reads violet — a second row, only while cursed.
  if (showCurse) {
    char cb[64];
    std::snprintf(cb, sizeof cb, "CURSE -25%% heal    %us", st.curseTicksLeft / 20u);
    DrawText(cb, px + 8, by + (showBuffs ? 50 : 38), 10, Color{190, 110, 235, 255});
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

bool Game::isPledgeMemberName(const std::string& n) const {
  if (net_ == nullptr || net_->pledgeId == 0 || n.empty()) return false;
  for (const auto& m : net_->pledgeMembers) if (m.name == n) return true;
  return false;
}
void Game::drawSiegePanel() const {
  if (net_ == nullptr || !net_->welcomed) return;
  const auto& s = net_->siege;
  const int px = 1024 - 190;
  const int py = 380; // below inventory (774,150 240x220 ends 370) — avoids overlap
  const int ph = 110;
  DrawRectangle(px, py, 182, ph, Color{28, 22, 14, 215});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), static_cast<float>(py), 182, static_cast<float>(ph)}, 1.0f, Color{160, 140, 90, 220});
  DrawText("WEEPING CASTLE", px + 8, py + 6, 10, Color{235, 220, 190, 255});
  char buf[96];
  const char* holder = s.holderName.empty() ? "unclaimed" : s.holderName.c_str();
  std::string hp = s.holderPledgeName.empty() ? std::string(holder) : std::string(holder) + " [" + s.holderPledgeName + "]";
  std::snprintf(buf, sizeof buf, "castle: %s", hp.c_str());
  DrawText(buf, px + 8, py + 22, 10, Color{200, 200, 180, 255});
  std::snprintf(buf, sizeof buf, "vault: %ug (%u crowns)", s.vaultGold, s.crowns);
  DrawText(buf, px + 8, py + 36, 10, Color{210, 190, 150, 255});
  const char* phs = s.phase == 4 ? "crowning" : s.phase == 3 ? "attuned" : s.phase == 2 ? "battle" : s.phase == 1 ? "window" : "quiet";
  std::snprintf(buf, sizeof buf, "battle: %s  bands %u/8", phs, s.bandCount);
  DrawText(buf, px + 8, py + 50, 10, s.phase >= 2 ? Color{235, 160, 120, 255} : Color{180, 170, 160, 255});
  std::snprintf(buf, sizeof buf, "gates %u/%u  heart %u/1200%s", s.gateHp0, s.gateHp1, s.heartProgress, s.heartAttuned ? " attuned" : "");
  DrawText(buf, px + 8, py + 64, 10, Color{180, 200, 190, 255});
  if (s.crownOwnerId != 0) {
    std::snprintf(buf, sizeof buf, "crown: %s (%u)", s.crownOwnerName.c_str(), s.crownDeadline);
    DrawText(buf, px + 8, py + 78, 10, Color{255, 210, 120, 255});
  } else {
    std::snprintf(buf, sizeof buf, "window ends %u  battle %u", s.windowEndTick, s.battleEndTick);
    DrawText(buf, px + 8, py + 78, 10, Color{140, 140, 140, 255});
  }
  DrawText("gm siege readout — wire 117", px + 8, py + 92, 10, Color{120, 110, 100, 255});
}
void Game::drawPledgePanel() const {
  if (net_ == nullptr || !net_->welcomed) return;
  const int px = 1024 - 190;
  const int py = 500;
  const int pw = 182;
  const int rows = static_cast<int>(net_->pledgeMembers.size());
  const int ph = net_->pledgeId == 0 ? 54 : 42 + std::min(rows, 8) * 14 + 18;
  DrawRectangle(px, py, pw, ph, Color{28, 22, 14, 215});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), static_cast<float>(ph)}, 1.0f, Color{160, 140, 90, 220});
  if (net_->pledgeId == 0) {
    DrawText("OATH: Unsworn.", px + 8, py + 8, 10, Color{200, 200, 190, 255});
    DrawText("/pledge create <name>", px + 8, py + 22, 10, Color{140, 140, 140, 255});
    DrawText("at the registrar", px + 8, py + 36, 10, Color{140, 140, 140, 255});
    return;
  }
  char buf[96];
  std::snprintf(buf, sizeof buf, "%s  emblem %u", net_->pledgeName.c_str(), net_->pledgeEmblem);
  DrawText(buf, px + 8, py + 6, 10, Color{235, 220, 190, 255});
  std::snprintf(buf, sizeof buf, "vault %ug  %zu members", net_->pledgeVault, net_->pledgeMembers.size());
  DrawText(buf, px + 8, py + 20, 10, Color{210, 190, 150, 255});
  int y = py + 36;
  const char* rankN[4] = {"", "Initiate", "Bloodsworn", "Liege"};
  for (size_t i = 0; i < net_->pledgeMembers.size() && i < 8; ++i) {
    const auto& m = net_->pledgeMembers[i];
    std::snprintf(buf, sizeof buf, "%s %s L%u%s", m.name.c_str(), rankN[m.rank < 4 ? m.rank : 1], m.level, m.online ? "" : " (off)");
    DrawText(buf, px + 8, y, 10, m.online ? Color{200, 235, 170, 255} : Color{120, 120, 120, 255});
    y += 14;
  }
  if (rows > 8) DrawText("...", px + 8, y, 10, Color{140, 140, 140, 255});
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
    // T-069: Marta's stall (64) opens the panel; Sable's fence (69) opens
    // the same panel with her crate appended (server routes by item).
    if (kv.second.snap.kind == 64 ||
        kv.second.snap.kind == content::kWireKindFence) {
      const Vector2 vp = entRenderPos(kv.second);
      if (std::fabs(vp.x - op.x) <= 3.0f && std::fabs(vp.y - op.y) <= 3.0f) return true;
    }
  }
  return false;
}

bool Game::fenceNear() const {
  if (net_ == nullptr || !net_->welcomed) return false;
  const auto own = rents_.find(net_->ownId);
  if (own == rents_.end()) return false;
  const Vector2 op = entRenderPos(own->second);
  for (const auto& kv : rents_) {
    if (kv.second.snap.kind == content::kWireKindFence) {
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
    const bool gear = d->slot <= 1;
    char duraMark[16] = "";
    if (gear) {  // T-058 churn readout: durability on weapons/armor
      if (kv.second.durability == 0)
        std::snprintf(duraMark, sizeof duraMark, " [WORN]");
      else if (kv.second.durability < 100)
        std::snprintf(duraMark, sizeof duraMark, " %du", kv.second.durability);
    }
    char refineMark[8] = "";
    if (kv.second.refine > 0)
      std::snprintf(refineMark, sizeof refineMark,
                    refineGlowTier(kv.second.refine) == RefineGlow::kMythic ? " +%u**"
                    : refineGlowTier(kv.second.refine) == RefineGlow::kGlow  ? " +%u*"
                                                                            : " +%u",
                    static_cast<unsigned>(kv.second.refine));
    // ADR-0016: up to three affix names ride the row (rarity chrome first,
    // affix names after — T-159f1.3 order kept).
    std::string affixNames;
    for (const std::uint8_t ax : {kv.second.affix, kv.second.affix2, kv.second.affix3})
      if (ax > 0 && ax <= content::kAffixCount) affixNames += std::string(" ") + content::kAffixNames[ax];
    const char* affixName = affixNames.c_str();
    std::snprintf(buf, sizeof buf, "%s%s %s%s %s%s%s%s%s",
                  kv.second.equipped ? "[E] " : "    ",
                  kv.second.qty > 1 ? (std::to_string(kv.second.qty) + "x").c_str() : "",
                  rarityMarker(kv.second.rarity),
                  d->name, d->slot == 0 ? " (weapon)" : d->slot == 1 ? " (armor)" : "",
                  auraMark, duraMark, refineMark,
                  affixName[0] ? (std::string(" ") + affixName).c_str() : "");
    const Rectangle rr{static_cast<float>(px + 6), static_cast<float>(y - 2), 226, 14};
    const bool dormant = gear && kv.second.durability == 0;
    // T-ART-11: refine glow rides the inventory row (incl. equipped =
    // in-hand). Dormant wins (a worn-out +5 does not glow); otherwise glow
    // beats aura/equipped (rarest state reads first). In-world sprite
    // overlays need per-entity gear on the wire (no snapshot carries
    // refine) — deliberately out of scope, non-wire card.
    // T-159f1.3: rarity chrome joins the chain — Unique beats aura (a named
    // row is the rarest thing in the bag), aura still beats Rare/Magic
    // (blessed work reads first), rarity beats equipped (the [E] tag keeps
    // the equipped read). Colours flagged: Magic blue / Rare yellow (GDD §7
    // tiers) / Unique ember-orange.
    const RefineGlow glow = refineGlowTier(kv.second.refine);
    if (row == invHover_) DrawRectangleRec(rr, Color{120, 30, 30, 120});
    if (!dormant && glow != RefineGlow::kNone)
      DrawRectangleRec(
          rr, Color{255, 210, 110,
                    static_cast<unsigned char>(refineGlowAlpha(kv.second.refine))});
    const Color rowCol =
        dormant ? Color{110, 105, 100, 255}  // dormant: dust-grey (T-058)
        : glow != RefineGlow::kNone
            ? (glow == RefineGlow::kMythic ? Color{255, 240, 200, 255}
                                           : Color{255, 225, 150, 255})
        : kv.second.rarity == 3 ? Color{255, 160, 60, 255}
        : kv.second.aura > 0 ? Color{120, 235, 235, 255}  // widow-blessed teal
        : kv.second.rarity == 2 ? Color{255, 235, 90, 255}
        : kv.second.rarity == 1 ? Color{140, 180, 255, 255}
        : kv.second.equipped ? Color{255, 200, 90, 255}
                             : Color{200, 195, 185, 255};
    // T-ART-15 icon slot: real item art when the sheet covers the id,
    // else the rarity plate (rarity always reads even when glow/aura win).
    const Color plateFill =
        kv.second.rarity == 3   ? Color{200, 98, 42, 255}
        : kv.second.rarity == 2 ? Color{168, 138, 74, 255}
        : kv.second.rarity == 1 ? Color{74, 110, 168, 255}
                                : Color{42, 36, 38, 255};
    int textX = px + 20;
    if (drawItemIcon(kv.second.itemId, px + 6, y + 1, 14)) {
      textX = px + 24;
    } else {
      DrawRectangle(px + 8, y + 2, 8, 8, plateFill);
      DrawRectangleLines(px + 8, y + 2, 8, 8, Color{20, 16, 18, 255});
    }
    DrawText(buf, textX, y, 10, rowCol);
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
  // T-069: the crate section appears only at Sable's fence; the panel grows.
  // T-071: Marta's rows are stock-driven (7 since the torch + lantern).
  const bool fence = fenceNear();
  int rows = 0;
  for (const std::uint32_t id : content::kVendorStock) {
    if (content::findItem(id) != nullptr) ++rows;
  }
  if (fence) {
    for (const std::uint32_t id : content::kFenceStock) {
      if (content::findItem(id) != nullptr) ++rows;
    }
    rows += 1;  // crate header
  }
  const int ph = 56 + rows * 16 + 22;
  const int px = 12, py = 150;
  DrawRectangle(px, py, 250, ph, Color{12, 10, 8, 210});
  DrawRectangleLinesEx(Rectangle{(float)px, (float)py, 250.0f, (float)ph}, 1.0f,
                       Color{180, 150, 90, 220});
  DrawText("MARTA the Quarterwidow  -  stock", px + 8, py + 6, 10,
           Color{235, 220, 200, 255});
  int y = py + 24;
  int i = 0;
  for (const std::uint32_t id : content::kVendorStock) {
    const content::ItemDef* d = content::findItem(id);
    if (d == nullptr) continue;
    char buf[96];
    std::snprintf(buf, sizeof buf, "F%d  %-14s %ug", i + 1, d->name, d->value);
    // T-ART-15: item art when the sheet covers the id (else plain text).
    if (drawItemIcon(id, px + 8, y + 1, 14))
      DrawText(buf, px + 26, y, 10, Color{220, 205, 185, 255});
    else
      DrawText(buf, px + 10, y, 10, Color{220, 205, 185, 255});
    y += 16;
    ++i;
  }
  char buf[96];
  if (fence) {
    DrawText("SABLE the Fence  -  no questions", px + 8, y + 2, 10,
             Color{235, 120, 110, 255});
    y += 16;
    int j = 0;
    for (const std::uint32_t id : content::kFenceStock) {
      const content::ItemDef* d = content::findItem(id);
      if (d == nullptr) continue;
      const std::uint32_t price = d->value * content::kFenceMarkupPct / 100;
      std::snprintf(buf, sizeof buf, "F%d  %-14s %ug", j + 8, d->name, price);
      if (drawItemIcon(id, px + 8, y + 1, 14))
        DrawText(buf, px + 26, y, 10, Color{220, 170, 160, 255});
      else
        DrawText(buf, px + 10, y, 10, Color{220, 170, 160, 255});
      y += 16;
      ++j;
    }
    std::snprintf(buf, sizeof buf, "G  sell all junk (60%%, no questions)");
  } else {
    std::snprintf(buf, sizeof buf, "G  sell all junk (40%%)");
  }
  DrawText(buf, px + 10, y + 2, 10, Color{190, 170, 150, 255});
  std::snprintf(buf, sizeof buf, "gold %ug", net_->ownStats.gold);
  DrawText(buf, px + 10, py + ph - 20, 10, Color{255, 215, 120, 255});
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
    DrawText("LMB walk/fight - 1 PowerSwing - Q sip - I bag - T trade (P commit/X cancel) - Enter chat - F1 help", 16,
             82, 10, GRAY);
    DrawText("I inventory - V vendor - Space recenter - F3 grid - F1 help", 16, 98, 10,
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

void Game::drawLightPool(Vector2 tileFrac, int radiusTiles) const {
  if (radiusTiles <= 0) return;
  const Vector2 w = iso::tileToWorldF(tileFrac, map_.tileW, map_.tileH);
  const Vector2 sp = GetWorldToScreen2D(w, rig_.cam);
  const float radius = static_cast<float>(radiusTiles) * kLightTilePx;
  const auto peak = static_cast<unsigned char>(kLightGlowAlpha);
  DrawCircleGradient(static_cast<int>(sp.x), static_cast<int>(sp.y), radius,
                     Color{255, 190, 110, peak}, Color{255, 190, 110, 0});
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
  if (debugZoom_ > 0.0f) rig_.cam.zoom = debugZoom_;  // T-ART-12: dev captures
  if (debugCam_)  // T-ART-14b: pin the frame on the grind (dev captures)
    rig_.cam.target = iso::tileToWorldF(Vector2{debugCamTx_, debugCamTy_}, map_.tileW,
                                        map_.tileH);

  BeginDrawing();
  ClearBackground(Color{8, 8, 12, 255});
  BeginMode2D(rig_.cam);
  drawGround();
  drawVfxTest();  // T-ART-14 dev visual (no-op unless --vfx-test)
  drawVfxPlays();  // T-ART-14b event VFX (no-op offline / empty)
  if (showPath_ && net_ == nullptr) drawPathPreview();
  drawCommandMarker();
  if (net_ != nullptr) drawVestiges();   // T-066 petrify-fade silhouettes
  if (net_ != nullptr) drawFloaters();
  EndMode2D();

  const Color ov = nightOverlay(gameHour());
  if (ov.a > 0) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ov);
  // T-071 night light: carried pools are painted back additively-in-effect —
  // warm gradient circles over lit entities (own player + lit AoI). Normal
  // alpha blending toward warm can only lighten the overlay beneath, so the
  // T-062 tint floor (alpha 150) is never pushed darker. Render-only: the
  // server owns lightRadius; the mask reads snap.light.
  if (ov.a > 0 && net_ != nullptr) {
    for (const auto& kv : rents_) {
      const std::uint8_t lr = kv.second.snap.light;
      if (lr == 0) continue;
      drawLightPool(entRenderPos(kv.second), lr);
    }
  }
  if (lampRadius_ > 0 && net_ == nullptr) {
    // T-164 dev matrix: one shipped-style pool at the offline hero.
    drawLightPool(Vector2{walker_.fx(), walker_.fy()}, lampRadius_);
  }
  drawChat();
  drawHud();
  drawStatPanel();
  drawPartyFrame();
  drawHotbar();  // T-ART-15 skill slots (online; follows the input page)
  drawSiegePanel();
  drawPledgePanel();
  drawInventoryPanel();
  drawVendorPanel();
  drawAnvilPanel();
  drawTradeBanner();
  drawDeathOverlay();
  if (showHelp_) drawHelpPanel();  // T-169: F1 or /help
  if (net_ != nullptr && net_->needsCreate && !net_->welcomed) drawCreatePanel();  // T-167
  EndDrawing();
}

// T-167: pre-world creation picker — keyboard only, era parchment, no art
// dependency beyond text. 1/2/3 kit, M/F sex, Enter answers once.
void Game::drawCreatePanel() const {
  const int pw = 420, ph = 210, px = (1024 - pw) / 2, py = 200;
  DrawRectangle(px, py, pw, ph, Color{28, 22, 14, 235});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(px), static_cast<float>(py),
                                 static_cast<float>(pw), static_cast<float>(ph)},
                       1.0f, Color{160, 140, 90, 220});
  DrawText("WHO ENTERS THE HOLLOW?", px + 16, py + 12, 12, Color{235, 220, 190, 255});
  const char* kits[3] = {"1  Ravager      (steel first)",
                         "2  Gravecaller  (plague and fire)",
                         "3  Cultist      (mend and rite)"};
  for (int i = 0; i < 3; ++i) {
    const bool sel = createClass_ == i + 1;
    DrawText(kits[i], px + 16, py + 44 + i * 20, 10,
             sel ? Color{255, 210, 120, 255} : Color{180, 170, 160, 255});
  }
  DrawText(createSex_ == 1 ? "> M  male" : "  M  male", px + 16, py + 112, 10,
           createSex_ == 1 ? Color{255, 210, 120, 255} : Color{180, 170, 160, 255});
  DrawText(createSex_ == 2 ? "> F  female" : "  F  female", px + 130, py + 112, 10,
           createSex_ == 2 ? Color{255, 210, 120, 255} : Color{180, 170, 160, 255});
  const bool ready = createClass_ != 0 && createSex_ != 0;
  DrawText(ready ? "Enter  to step through the gate" : "1/2/3 + M/F, then Enter",
           px + 16, py + 150, 10,
           ready ? Color{200, 235, 170, 255} : Color{140, 140, 140, 255});
  DrawText("one soul per account for alpha (T-167)", px + 16, py + 172, 10,
           Color{120, 110, 100, 255});
}

// T-169: in-client discoverability — F1 or /help toggles this panel.
void Game::drawHelpPanel() const {
  const int px = 200, py = 80, pw = 624, ph = 608;
  DrawRectangle(px, py, pw, ph, Color{12, 10, 8, 220});
  DrawRectangleLinesEx(Rectangle{(float)px, (float)py, (float)pw, (float)ph},
                       1.0f, Color{180, 150, 90, 220});
  int y = py + 10;
  auto line = [&](const char* txt, Color c) {
    DrawText(txt, px + 12, y, 10, c);
    y += 14;
  };
  auto header = [&](const char* txt) {
    DrawText(txt, px + 12, y, 11, Color{220, 180, 120, 255});
    y += 16;
  };
  auto blank = [&]() { y += 6; };

  header("BLOODHOLLOW — COMMAND REFERENCE");
  line("(close: F1 or type /help again)", Color{100, 90, 80, 255});
  blank();

  header("COMBAT / MOVEMENT");
  line("LMB click    walk / attack", GRAY);
  line("WASD         instant step", GRAY);
  line("1-5          hotbar skills (6 = Resurrect @selected)", GRAY);
  line("Shift+1..8   deeper rites (Sanctuary..Ward @selected)", GRAY);
  line("Ctrl+1..5    ravager rites (Sunder..Second Wind)", GRAY);
  line("Q            sip potion", GRAY);
  line("F5/F6/F7     +STR/+VIT/+DEX (stat point)", GRAY);
  line("F8/F9        +INT/+MAG (stat point)", GRAY);
  line("F            open anvil (near anvil NPC)", GRAY);
  blank();

  header("CHAT / SOCIAL");
  line("Enter        open chat", GRAY);
  line("/invite      invite to party", GRAY);
  line("/accept      accept party invite", GRAY);
  line("/leave       leave party", GRAY);
  line("/kick <name> kick from party", GRAY);
  line("/duel <name> consent duel", GRAY);
  line("/forfeit     end duel", GRAY);
  line("/confess     pay gold to repent (clear karma)", GRAY);
  blank();

  header("PROGRESSION / CRAFT");
  line("/kit <name>  choose class (ravager/gravecaller/cultist)", GRAY);
  line("/refine <slot> upgrade gear at anvil", GRAY);
  line("/repair      repair all equipped gear", GRAY);
  line("/mine        harvest ore (near node, pick equipped)", GRAY);
  blank();

  header("PLEDGE / SIEGE");
  line("/pledge      create bloodpledge (L>=10, 10k gold)", GRAY);
  line("/oath        swear town loyalty (L19+)", GRAY);
  line("/siege-reg   register for siege", GRAY);
  line("/breach      attack siege gate", GRAY);
  line("/crown       channel the throne (crown phase)", GRAY);
  blank();

  header("VENDOR / TRADE");
  line("I            inventory", GRAY);
  line("V            vendor (near NPC)", GRAY);
  line("T            trade offer (click player)", GRAY);
  line("P            commit trade", GRAY);
  line("X            cancel trade", GRAY);
  line("G            sell junk (near NPC)", GRAY);
  line("F1-F12       buy from vendor slot", GRAY);
  blank();

  header("SYSTEM");
  line("Space        recenter camera", GRAY);
  line("F3           toggle grid", GRAY);
  line("F4           toggle path overlay", GRAY);
  line("H/N          shift hour (debug)", GRAY);
  line("ESC          quit", GRAY);
  blank();

  line("Type /help in chat or press F1 to toggle this panel.", Color{140, 130, 120, 255});
}

}  // namespace bh
