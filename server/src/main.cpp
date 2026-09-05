// bh_server - BLOODHOLLOW headless authoritative world server (Phase 1-2).
// One process, one zone (Thornwall); zones-in-process per ADR-003.
// 20 Hz fixed tick. Protocol v0 (shared/protocol/messages.md).
// Sprint 5 (P2): stats/XP/levels, melee resolve, mobs from map spawners.
#include <algorithm>
#include <map>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <enet/enet.h>

#include "persist.h"
#include "protocol/messages_gen.h"
#include "sim/combat.h"
#include "sim/clock.h"
#include "sim/tick.h"
#include "command.h"
#include "world.h"

namespace bh::server {
namespace {

constexpr int kAoiRadius = 26;          // tiles, Chebyshev (architecture docs/03 section 4)
constexpr std::int64_t kTickMicros = 1000000 / sim::kTickHz;  // 50 ms
constexpr int kMaxChatLen = 160;
constexpr int kChatMinGapTicks = 10;    // 0.5 s

struct Session {
  ENetPeer* peer = nullptr;
  bool authed = false;
  bool inWorld = false;
  std::string user{};
  std::int64_t charRowId = 0;
  std::uint32_t entityId = 0;
  std::deque<Command> cmdq{};
  std::unordered_set<std::uint32_t> interest{};
  sim::Tick lastChatTick = -kChatMinGapTicks;
};

struct Server {
  // --bless user=item:qty,... (smoke-test only, empty by default)
  std::map<std::string, std::string> bless;

  ENetHost* host = nullptr;
  World world{};
  Db db{};
  std::unordered_map<ENetPeer*, Session> sessions{};
  sim::Tick tick = 0;
  std::uint16_t mapId = 1;
  // world journal (M2 gate): record every world-mutating interaction + hashes
  std::string recordWorldPath{};
  std::string replayWorldPath{};
  FILE* journal = nullptr;
  std::unordered_map<ENetPeer*, std::uint32_t> loginIndexPerPeer{};
  std::vector<std::string> loginOrder{};  // name per index (stable)
  // soak metrics
  std::vector<std::int64_t> tickMicros{};
  std::int64_t startEpochSec = 0;
  std::int64_t soakSecs = 0;
  double p99BudgetMs = 10.0;
  std::uint64_t packetsIn = 0, packetsOut = 0;
};

void pushOwnStats(Server& s, Session& sess);
void pushInventory(Server& s, Session& sess);

void send(ENetPeer* peer, const std::vector<std::uint8_t>& bytes, Server& s) {
  ENetPacket* p = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, p);
  s.packetsOut++;
}

template <typename Msg>
void sendMsg(ENetPeer* peer, const Msg& m, Server& s) {
  send(peer, proto::pack(m), s);
}

// ---- world journal record helpers (M2) ------------------------------------
void journalTickHash(Server& s) {
  if (s.journal == nullptr) return;
  if (std::getenv("BH_DUMP_ENTS") != nullptr) {
    for (const auto& ent : s.world.entities()) {
      std::fprintf(stderr,
                   "[live-dump] t=%lld id=%u kind=%d wk=%u zone=%u qpos=%d:%d hp=%u skill=%u gold=%u mercy=%u karma=%d atk=%u psz=%zu ch=%lld sw=%lld mv=%d tgt=%d:%d\n",
                   static_cast<long long>(s.tick), ent.id, static_cast<int>(ent.kind),
                   ent.wireKind, ent.zoneId, ent.walker.x, ent.walker.y,
                   ent.hp, ent.swordSkill, ent.gold, ent.anvilMercyMask, ent.karma,
                   ent.attackTarget, ent.path.size(),
                   static_cast<long long>(ent.lastChaseTick), static_cast<long long>(ent.lastSwingTick),
                   ent.walker.moving ? 1 : 0, ent.walker.target.x, ent.walker.target.y);
    }
  }
  std::fprintf(s.journal, "h %lld %016llx\n", static_cast<long long>(s.tick),
               static_cast<unsigned long long>(s.world.worldHash()));
}

void journalLogin(Server& s, const Session& sess, const CharacterRow& row,
                  const Entity& e) {
  if (s.journal == nullptr) return;
  const std::uint32_t idx = static_cast<std::uint32_t>(s.loginOrder.size());
  s.loginIndexPerPeer[sess.peer] = idx;
  s.loginOrder.push_back(row.name);
  const sim::TilePos t = e.walker.tile();
  std::fprintf(s.journal,
               "l %lld %u %s %d %d %d %u %u %u %u %u %u %u %d %s\n",
               static_cast<long long>(s.tick + 1), idx, row.name.c_str(), t.x, t.y,
               static_cast<unsigned>(row.level), static_cast<unsigned>(row.xp),
               static_cast<unsigned>(row.str), static_cast<unsigned>(row.vit),
               static_cast<unsigned>(row.dex), static_cast<unsigned>(row.statPoints),
               static_cast<unsigned>(row.gold),
               static_cast<unsigned>(row.anvilMercy), row.karma,
               row.invBlob.empty() ? "-" : row.invBlob.c_str());
}

void journalDisconnect(Server& s, const Session& sess) {
  if (s.journal == nullptr) return;
  const auto it = s.loginIndexPerPeer.find(sess.peer);
  if (it == s.loginIndexPerPeer.end()) return;
  // pump-phase event: effect takes place BEFORE the next tick runs, so tag the
  // first tick where the world differs (replay applies before that world tick)
  std::fprintf(s.journal, "d %lld %u\n", static_cast<long long>(s.tick + 1),
               it->second);
}

void journalCommand(Server& s, const Session& sess, const Command& c) {
  if (s.journal == nullptr) return;
  const auto it = s.loginIndexPerPeer.find(sess.peer);
  if (it == s.loginIndexPerPeer.end()) return;
  // world-mutating kinds only; chat/ping carry no sim effect
  if (c.kind == Command::kChat || c.kind == Command::kPing) return;
  std::fprintf(s.journal, "c %lld %u %d %d %d %u\n",
               static_cast<long long>(s.tick), it->second, static_cast<int>(c.kind),
               c.a, c.b, static_cast<unsigned>(c.channel));
}

std::string sanitizeChat(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (const unsigned char c : in) {
    if (c >= 32 && c <= 126) out.push_back(static_cast<char>(c));
    if (out.size() >= kMaxChatLen) break;
  }
  // strip leading/trailing spaces
  const auto a = out.find_first_not_of(' ');
  const auto b = out.find_last_not_of(' ');
  if (a == std::string::npos) return "";
  return out.substr(a, b - a + 1);
}

void broadcastChat(Server& s, std::uint8_t channel, const std::string& from,
                   const std::string& text) {
  proto::ChatMsg msg;
  msg.channel = channel;
  msg.from = from;
  msg.text = text;
  const auto bytes = proto::pack(msg);
  if (channel == 0) {
    // say: AoI around the speaker
    const Entity* speaker = nullptr;
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld && kv.second.user == from) {
        speaker = s.world.find(kv.second.entityId);
        break;
      }
    }
    if (speaker == nullptr) return;
    const sim::TilePos sp = speaker->walker.tile();
    const auto ids = s.world.queryAoi(speaker->zoneId, sp.x, sp.y, kAoiRadius);
    const std::unordered_set<std::uint32_t> aoi(ids.begin(), ids.end());
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld && aoi.count(kv.second.entityId) > 0) {
        send(kv.first, bytes, s);
      }
    }
  } else {
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld) send(kv.first, bytes, s);
    }
  }
}

void dropSession(Server& s, Session& sess) {
  if (sess.inWorld) {
    const Entity* e = s.world.find(sess.entityId);
    if (e != nullptr && e->charRowId != 0) {
      const sim::TilePos p = e->walker.tile();
      s.db.savePosition(e->charRowId, e->zoneId, p.x, p.y);  // T-039: zone travels
      {
        std::string blob;
        for (const InvSlot& sl : e->inv) {
          blob += std::to_string(sl.itemId) + ":" + std::to_string(sl.qty) + ":" +
                  (sl.equipped ? "1" : "0") + ";";
        }
        s.db.saveProgress(e->charRowId, e->level, e->xp, e->str, e->vit, e->dex,
                          e->statPoints, static_cast<int>(e->gold), blob,
                          e->anvilMercyMask, e->karma);
      }
      std::printf("[net] %-16s saved at (%d,%d)\n", e->name.c_str(), p.x, p.y);
    }
    s.world.despawn(sess.entityId);
    broadcastChat(s, 2, "", sess.user + " has left the world.");
    const sim::Tick now = s.tick;
    (void)now;
  }
  s.sessions.erase(sess.peer);
}

void handlePacket(Server& s, Session& sess, const proto::PacketView& pv) {
  using namespace proto;
  switch (pv.id) {
    case kIdHello: {
      Hello h;
      if (!h.deserialize(pv.body)) return;
      if (h.protoVersion != kProtocolVersion) {
        LoginResult r;
        r.ok = 0;
        r.reason = 4;  // protocol mismatch
        sendMsg(sess.peer, r, s);
        enet_peer_disconnect_later(sess.peer, 0);
        return;
      }
      CharacterRow row;
      std::uint8_t reason = 0;
      std::string err;
      if (!s.db.loginOrCreate(h.username, h.password, &row, &reason, &err)) {
        LoginResult r;
        r.ok = 0;
        r.reason = reason;
        sendMsg(sess.peer, r, s);
        return;
      }
      LoginResult r;
      r.ok = 1;
      r.reason = 0;
      sendMsg(sess.peer, r, s);

      sess.authed = true;
      sess.inWorld = true;
      sess.user = row.name;
      sess.charRowId = row.id;

      std::optional<sim::TilePos> at;
      if (row.x != 0 || row.y != 0) at = sim::TilePos{row.x, row.y};
      const std::uint16_t loginZone =
          static_cast<std::uint16_t>(row.mapId > 0 ? row.mapId : 1);
      Entity& e = s.world.spawn(row.name, row.id, at, loginZone);
      sess.entityId = e.id;
      journalLogin(s, sess, row, e);
      {
        // apply persisted progression (schema v2)
        Entity* pe = s.world.find(e.id);
        pe->level = static_cast<std::uint8_t>(row.level < 1 ? 1 : (row.level > 25 ? 25 : row.level));
        pe->xp = static_cast<std::uint32_t>(row.xp < 0 ? 0 : row.xp);
        pe->str = static_cast<std::uint8_t>(row.str);
        pe->vit = static_cast<std::uint8_t>(row.vit);
        pe->dex = static_cast<std::uint8_t>(row.dex);
        pe->statPoints = static_cast<std::uint8_t>(row.statPoints);
        pe->gold = static_cast<std::uint32_t>(row.gold < 0 ? 0 : row.gold);
        // hpMax follows level/VIT; heal in full on login
        pe->hpMax = sim::playerHpMax(pe->level, pe->vit);
        pe->hp = pe->hpMax;
        // inventory blob "itemId:qty:equipped;..."
        if (!row.invBlob.empty()) {
          size_t pos = 0;
          while (pos < row.invBlob.size()) {
            const size_t end = row.invBlob.find(';', pos);
            const std::string rec =
                row.invBlob.substr(pos, end == std::string::npos ? end : end - pos);
            pos = end == std::string::npos ? row.invBlob.size() : end + 1;
            const size_t c1 = rec.find(':');
            const size_t c2 = rec.rfind(':');
            if (c1 == std::string::npos || c2 == c1) continue;
            InvSlot sl;
            sl.itemId = static_cast<std::uint32_t>(std::stoul(rec.substr(0, c1)));
            sl.qty = static_cast<std::uint16_t>(std::stoul(rec.substr(c1 + 1, c2 - c1 - 1)));
            // 3-field (legacy) or 4-field (v5 aura) tail
            const std::string tail = rec.substr(c2 + 1);
            const size_t c3 = tail.find(':');
            sl.equipped = (c3 == std::string::npos ? tail : tail.substr(0, c3)) == "1";
            if (c3 != std::string::npos)
              sl.aura = static_cast<std::uint8_t>(std::stoul(tail.substr(c3 + 1)));
            pe->inv.push_back(sl);
          }
        }
        pe->anvilMercyMask = static_cast<std::uint32_t>(row.anvilMercy);
        pe->karma = row.karma;
        const auto bit = s.bless.find(row.name);
        if (bit != s.bless.end()) {
          // parse "id:qty,id:qty"
          size_t bpos = 0;
          while (bpos < bit->second.size()) {
            const size_t bend = bit->second.find(',', bpos);
            const std::string pr = bit->second.substr(
                bpos, bend == std::string::npos ? std::string::npos : bend - bpos);
            bpos = (bend == std::string::npos) ? bit->second.size() : bend + 1;
            const size_t colon = pr.find(':');
            if (colon == std::string::npos) continue;
            if (pr.substr(0, colon) == "skill") {
              const std::uint32_t sk =
                  static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
              // combat derives swordSkill from swingLands/kSkillLandsPerPoint: seed the counter
              pe->swingLands = sk * 20u;
              pe->swordSkill = sk;
              continue;
            }
            if (pr.substr(0, colon) == "gold") {
              pe->gold = static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
              continue;
            }
            s.world.debugGive(*pe,
                              static_cast<std::uint32_t>(std::stoul(pr.substr(0, colon))),
                              static_cast<std::uint16_t>(std::stoul(pr.substr(colon + 1))));
          }
          std::printf("[bless] %s <- %s (skill now %u, gold %u)\n", row.name.c_str(),
                      bit->second.c_str(), static_cast<unsigned>(pe->swordSkill),
                      static_cast<unsigned>(pe->gold));
          for (const auto& sl : pe->inv)
            std::printf("[bless]   inv %u:%u eq=%u aura=%u\n", sl.itemId, sl.qty,
                        sl.equipped ? 1u : 0u, static_cast<unsigned>(sl.aura));
          if (s.journal != nullptr) {
            std::fprintf(s.journal, "b %lld %s %s\n",
                         static_cast<long long>(s.tick + 1), row.name.c_str(),
                         bit->second.c_str());
          }
        }
      }

      Welcome w;
      w.entityId = e.id;
      w.mapId = loginZone;
      w.x = e.walker.x;
      w.y = e.walker.y;
      w.tick = static_cast<std::uint32_t>(s.tick % 0xFFFFFFFFu);
      w.hourCenti = static_cast<std::uint32_t>(sim::hourAt(s.tick) * 100.0f);
      sendMsg(sess.peer, w, s);
      pushOwnStats(s, sess);
      pushInventory(s, sess);
      std::printf("[net] %-16s entered the world (entity %u, online %zu)\n",
                  row.name.c_str(), sess.entityId, s.sessions.size());
      broadcastChat(s, 2, "", row.name + " has entered Thornwall.");
      break;
    }
    case kIdInputPath: {
      InputPath m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kPath;
      c.a = m.goalX;
      c.b = m.goalY;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdInputStep: {
      InputStep m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kStep;
      c.a = m.dx;
      c.b = m.dy;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdChatSend: {
      ChatSend m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kChat;
      c.channel = m.channel;
      c.text = std::move(m.text);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdAttackRequest: {
      AttackRequest m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kAttack;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdStatAssign: {
      StatAssign m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kStat;
      c.channel = m.stat;  // reuse byte slot
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdUseItem: {
      UseItem m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kUseItem;
      c.channel = m.slot;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdToggleEquip: {
      ToggleEquip m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kEquip;
      c.channel = m.slot;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdSkillUse: {
      SkillUse m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kSkill;
      c.channel = m.skill;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdBuyRequest: {
      BuyRequest m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kBuy;
      c.a = static_cast<std::int32_t>(m.itemId);
      c.b = m.qty;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdSellJunk: {
      SellJunk m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kSellJunk;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeOpen: {
      TradeOpen m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeOpen;
      c.a = static_cast<std::int32_t>(m.targetId);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeOfferItem: {
      TradeOfferItem m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeItem;
      c.a = static_cast<std::int32_t>(m.itemId);
      c.b = m.qty;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeOfferGold: {
      TradeOfferGold m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeGold;
      c.a = static_cast<std::int32_t>(m.gold);
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeCommit: {
      TradeCommit m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeCommit;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdTradeCancel: {
      TradeCancel m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kTradeCancel;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdAnvilOp: {
      AnvilOp m;
      if (!m.deserialize(pv.body) || !sess.inWorld) return;
      Command c;
      c.kind = Command::kAnvil;
      c.channel = m.tier;
      if (sess.cmdq.size() < 32) sess.cmdq.push_back(std::move(c));
      break;
    }
    case kIdPing: {
      Ping m;
      if (!m.deserialize(pv.body)) return;
      Pong p;
      p.clientTimeMs = m.clientTimeMs;
      p.serverTick = static_cast<std::uint32_t>(s.tick % 0xFFFFFFFFu);
      sendMsg(sess.peer, p, s);
      break;
    }
    default:
      break;
  }
}

void processCommand(Server& s, Session& sess, const Command& c) {
  journalCommand(s, sess, c);
  if (c.kind == Command::kChat) {
    if (s.tick - sess.lastChatTick < kChatMinGapTicks) return;  // rate limit
    sess.lastChatTick = s.tick;
    const std::string text = sanitizeChat(c.text);
    if (text.empty()) return;
    broadcastChat(s, c.channel == 0 ? 0 : 1, sess.user, text);
    return;
  }
  if (c.kind == Command::kPing) return;
  Entity* e = s.world.find(sess.entityId);
  if (e == nullptr) return;
  applyWorldCommand(s.world, *e, c);  // single source of truth (T-049)
}

void pushInventory(Server& s, Session& sess) {
  const Entity* e = s.world.find(sess.entityId);
  if (e == nullptr) return;
  proto::InventoryReset r;
  r.count = static_cast<std::uint8_t>(e->inv.size());
  sendMsg(sess.peer, r, s);
  for (size_t i = 0; i < e->inv.size(); ++i) {
    proto::ItemSlot m;
    m.slot = static_cast<std::uint8_t>(i);
    m.itemId = e->inv[i].itemId;
    m.qty = e->inv[i].qty;
    m.equipped = e->inv[i].equipped ? 1 : 0;
    m.aura = e->inv[i].aura;
    sendMsg(sess.peer, m, s);
  }
}

void pushOwnStats(Server& s, Session& sess) {
  const Entity* e = s.world.find(sess.entityId);
  if (e == nullptr || e->kind != EntityKind::kPlayer) return;
  proto::OwnStats m;
  m.level = e->level;
  m.xp = e->xp;
  m.xpNext = sim::xpNext(e->level);
  m.statPoints = e->statPoints;
  m.str = e->str;
  m.vit = e->vit;
  m.dex = e->dex;
  m.intg = 0;
  m.mag = 0;
  m.swordSkill = e->swordSkill;
  m.gold = e->gold;
  m.karma = e->karma;
  sendMsg(sess.peer, m, s);
}

// Distribute world events after each sim tick.
void distributeEvents(Server& s) {
  for (const WorldEvent& ev : s.world.events()) {
    if (ev.chatCh == 255 && !ev.chatText.empty()) {
      proto::ChatMsg m;
      m.channel = 2;
      m.from = "";
      m.text = ev.chatText;
      const auto bytes = proto::pack(m);
      for (auto& kv : s.sessions) {
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) {
          send(kv.first, bytes, s);
        }
      }
    } else if (ev.chatCh != 0 && !ev.chatText.empty()) {
      broadcastChat(s, ev.chatCh, "", ev.chatText);
    }
    if (ev.zoneChanged && ev.aboutId != 0) {
      for (auto& kv : s.sessions) {
        Session& sess = kv.second;
        if (!sess.inWorld || sess.entityId != ev.aboutId) continue;
        const Entity* e = s.world.find(ev.aboutId);
        if (e == nullptr) continue;
        sess.interest.clear();  // force full respawn burst for the new zone
        proto::Welcome w;
        w.entityId = e->id;
        w.mapId = e->zoneId;
        w.x = e->walker.x;
        w.y = e->walker.y;
        w.tick = static_cast<std::uint32_t>(s.tick % 0xFFFFFFFFu);
        w.hourCenti = static_cast<std::uint32_t>(sim::hourAt(s.tick) * 100.0f);
        sendMsg(sess.peer, w, s);
        pushOwnStats(s, sess);
        pushInventory(s, sess);
        s.db.savePosition(e->charRowId, e->zoneId, e->walker.tile().x, e->walker.tile().y);
        std::printf("[zone] %s -> map %u at (%d,%d)\n", e->name.c_str(),
                    static_cast<unsigned>(e->zoneId), e->walker.tile().x,
                    e->walker.tile().y);
        std::fflush(stdout);
      }
    }
    if (ev.invChanged && ev.aboutId != 0) {
      for (auto& kv : s.sessions) {
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) {
          pushInventory(s, kv.second);
        }
      }
    }
    if (ev.statsChanged && ev.aboutId != 0) {
      for (auto& kv : s.sessions) {
        if (kv.second.inWorld && kv.second.entityId == ev.aboutId) {
          pushOwnStats(s, kv.second);
        }
      }
    }
    if (ev.attacker != 0 || ev.target != 0) {
      // combat pulse: to everyone whose AoI contains either endpoint
      proto::CombatEvent m;
      m.attackerId = ev.attacker;
      m.targetId = ev.target;
      m.kind = ev.kind;
      m.amount = ev.amount;
      const auto bytes = proto::pack(m);
      for (auto& kv : s.sessions) {
        Session& viewer = kv.second;
        if (!viewer.inWorld) continue;
        if (viewer.interest.count(ev.attacker) > 0 ||
            viewer.interest.count(ev.target) > 0) {
          send(viewer.peer, bytes, s);
        }
      }
    }
  }
}

void tickServer(Server& s) {
  const auto t0 = std::chrono::steady_clock::now();
  ++s.tick;

  // 1) one queued command per session per tick (fair-share intents)
  for (auto& kv : s.sessions) {
    if (!kv.second.cmdq.empty()) {
      processCommand(s, kv.second, kv.second.cmdq.front());
      kv.second.cmdq.pop_front();
    }
  }

  // 2) simulate (movement + combat + respawns)
  s.world.tick();
  distributeEvents(s);

  // 3) AoI deltas per session
  for (auto& kv : s.sessions) {
    Session& sess = kv.second;
    if (!sess.inWorld) continue;
    const Entity* me = s.world.find(sess.entityId);
    if (me == nullptr) continue;
    const sim::TilePos p = me->walker.tile();
    const auto ids = s.world.queryAoi(me->zoneId, p.x, p.y, kAoiRadius);
    std::unordered_set<std::uint32_t> now(ids.begin(), ids.end());

    for (const std::uint32_t id : now) {
      const Entity* e = s.world.find(id);
      if (e == nullptr) continue;
      if (sess.interest.count(id) == 0) {
        proto::EntitySpawn m;
        m.id = e->id;
        m.kind = e->wireKind;
        m.dir = static_cast<std::uint8_t>(e->walker.dir);
        m.x = e->walker.x;
        m.y = e->walker.y;
        m.hp = e->hp;
        m.hpMax = e->hpMax;
        m.level = e->kind == EntityKind::kPlayer ? e->level : e->mobLevel;
        m.name = e->name;
        sendMsg(sess.peer, m, s);
      }
      proto::EntityDelta d;
      d.id = e->id;
      d.x = e->walker.x;
      d.y = e->walker.y;
      d.dir = static_cast<std::uint8_t>(e->walker.dir);
      d.moving = e->walker.moving ? 1 : 0;
      d.hp = e->hp;
      sendMsg(sess.peer, d, s);
    }
    for (const std::uint32_t id : sess.interest) {
      if (now.count(id) == 0) {
        proto::EntityDespawn dm;
        dm.id = id;
        sendMsg(sess.peer, dm, s);
      }
    }
    sess.interest = std::move(now);
  }

  // 4) periodic stats + soak bookkeeping
  const auto t1 = std::chrono::steady_clock::now();
  s.tickMicros.push_back(
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
  if (s.tickMicros.size() > 600) s.tickMicros.erase(s.tickMicros.begin());

  if (s.tick % 20 == 0) {
    std::vector<std::int64_t> sorted = s.tickMicros;
    std::sort(sorted.begin(), sorted.end());
    const std::int64_t p99 = sorted.empty() ? 0 : sorted[sorted.size() * 99 / 100];
    proto::ServerStats st;
    st.online = static_cast<std::uint32_t>(s.sessions.size());
    st.tickMicrosP99 = static_cast<std::uint32_t>(p99);
    for (auto& kv : s.sessions) {
      if (kv.second.inWorld) sendMsg(kv.first, st, s);
    }
  }

  {
    static std::int64_t cadence = -1;
    if (cadence < 0) {
      const char* e = std::getenv("BH_HASH_CADENCE");
      cadence = (e != nullptr && std::atoi(e) > 0) ? std::atoi(e) : 100;
    }
    if (s.journal != nullptr && s.tick % cadence == 0) journalTickHash(s);
  }

  if (s.tick % 600 == 0) {  // every 30 s
    std::vector<std::int64_t> sorted = s.tickMicros;
    std::sort(sorted.begin(), sorted.end());
    const std::int64_t p50 = sorted.empty() ? 0 : sorted[sorted.size() / 2];
    const std::int64_t p99 = sorted.empty() ? 0 : sorted[sorted.size() * 99 / 100];
    std::printf(
        "[soak] tick=%lld online=%zu entities=%zu tickUs p50=%lld p99=%lld worldHash=%016llx "
        "pkts=%zu/%zu\n",
        static_cast<long long>(s.tick), s.sessions.size(), s.world.count(),
        static_cast<long long>(p50), static_cast<long long>(p99),
        static_cast<unsigned long long>(s.world.worldHash()),
        static_cast<size_t>(s.packetsIn), static_cast<size_t>(s.packetsOut));
    std::fflush(stdout);
  }
}

// ---- replay-world: deterministic verification runner (M2 wipe repro) ------
// Reads a .bwj journal: l=login(tick,name,idx,x,y,level,xp,str,vit,dex,sp,gold,inv);
// c=command(tick,loginIdx,kind,a,b,channel); h=tick hash markers every 100.
// Replays entity creation + world commands on a fresh world (fixed seed), then
// verifies every hash marker. Exit 0 = perfect replay (wipe+all), 3 = mismatch.
int runReplayWorld(const std::string& path, const std::string& mapPath) {
  FILE* f = std::fopen(path.c_str(), "r");
  if (f == nullptr) {
    std::fprintf(stderr, "bh_server: cannot open journal %s\n", path.c_str());
    return 1;
  }
  std::string err;
  World world;
  if (!world.load(mapPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    std::fclose(f);
    return 1;
  }
  {  // mirror the server's boot set (any zone asset that's actually there)
    for (const auto& [zid, zpath] : {std::pair<std::uint16_t, const char*>{2, "assets/maps/fields_overflow.bhmap"},
                                     {3, "assets/maps/thornwall_crypt.bhmap"}}) {
      std::string zerr;
      (void)world.loadZone(zid, zpath, &zerr);
    }
  }
  struct ReplayEnt {
    std::uint32_t id = 0;
  };
  std::vector<ReplayEnt> entities;
  struct QueuedLogin {
    sim::Tick tick;
    std::uint32_t idx;
    std::string name;
    int x, y;
    unsigned level, xp, st_, vit, dex, sp, gold;
    std::uint32_t mercyMask = 0;
    std::int32_t karma = 0;
    std::string inv;
  };
  struct QueuedBless { sim::Tick tick; std::string name; std::string spec; };
  
  struct QueuedCmd {
    sim::Tick tick;
    std::uint32_t idx;
    int kind;
    std::int32_t a, b;
    std::uint32_t channel;
  };
  struct QueuedHash {
    sim::Tick tick;
    std::uint64_t expected;
  };
  std::vector<QueuedLogin> logins;
  std::vector<QueuedCmd> cmds;
  std::vector<QueuedHash> hashes;
  std::vector<QueuedBless> queuedBlesses;
  std::vector<QueuedCmd> drops;  // idx-only, kind = -1
  sim::Tick lastTick = 0;
  char line[1024];
  while (std::fgets(line, sizeof line, f) != nullptr) {
    if (line[0] == 'l') {
      // l tick idx name x y level xp str vit dex sp gold inv
      char name[64], inv[768] = "-";
      long long tick;
      unsigned idx, level, xp, st_, vit, dex, sp, gold, mercy = 0;
      int x, y, karma = 0;
      const int n = std::sscanf(line, "l %lld %u %63s %d %d %u %u %u %u %u %u %u %u %d %767s",
                                &tick, &idx, name, &x, &y, &level, &xp, &st_, &vit,
                                &dex, &sp, &gold, &mercy, &karma, inv);
      if (n != 15 && n != 13) {
        std::fprintf(stderr, "[replay] malformed l-line: %s", line);
        std::fclose(f);
        return 2;
      }
      if (n == 13) { mercy = 0; karma = 0; }  // pre-v6 journals
      logins.push_back(QueuedLogin{static_cast<sim::Tick>(tick), idx, name, x, y,
                                   level, xp, st_, vit, dex, sp, gold, mercy, karma,
                                   inv});
      if (tick > lastTick) lastTick = tick;
    } else if (line[0] == 'b') {
      long long tick;
      char nm[64], spec[256];
      if (std::sscanf(line, "b %lld %63s %255s", &tick, nm, spec) == 3) {
        queuedBlesses.push_back(QueuedBless{static_cast<sim::Tick>(tick), nm, spec});
        if (tick > lastTick) lastTick = tick;
      }
    } else if (line[0] == 'c') {
      long long tick;
      unsigned idx, ch;
      int kind, a, b;
      if (std::sscanf(line, "c %lld %u %d %d %d %u", &tick, &idx, &kind, &a, &b, &ch) == 6) {
        cmds.push_back(QueuedCmd{static_cast<sim::Tick>(tick), idx, kind, a, b, ch});
        if (tick > lastTick) lastTick = tick;
      }
    } else if (line[0] == 'd') {
      long long tick;
      unsigned idx;
      if (std::sscanf(line, "d %lld %u", &tick, &idx) == 2) {
        drops.push_back(QueuedCmd{static_cast<sim::Tick>(tick), idx, -1, 0, 0, 0});
        if (tick > lastTick) lastTick = tick;
      }
    } else if (line[0] == 'h') {
      long long tick;
      unsigned long long h;
      if (std::sscanf(line, "h %lld %llx", &tick, &h) == 2) {
        hashes.push_back(QueuedHash{static_cast<sim::Tick>(tick), h});
        if (tick > lastTick) lastTick = tick;
      }
    }
  }
  std::fclose(f);

  size_t ci = 0, hi = 0, li = 0, di = 0;
  std::int64_t checked = 0, bad = 0;
  auto applyLogin = [&](const QueuedLogin& L) {
    Entity& e = world.spawn(L.name, 0, sim::TilePos{L.x, L.y});
    Entity* pe = world.find(e.id);
    pe->level = static_cast<std::uint8_t>(L.level < 1 ? 1 : (L.level > 25 ? 25 : L.level));
    pe->xp = L.xp;
    pe->str = static_cast<std::uint8_t>(L.st_);
    pe->vit = static_cast<std::uint8_t>(L.vit);
    pe->dex = static_cast<std::uint8_t>(L.dex);
    pe->statPoints = static_cast<std::uint8_t>(L.sp);
    pe->gold = L.gold;
    pe->anvilMercyMask = L.mercyMask;
    pe->karma = L.karma;
    // replay bless grants (debug lane recorded as b-lines)
    for (const auto& qb : queuedBlesses) {
      if (qb.name != L.name) continue;
      size_t bpos = 0;
      while (bpos < qb.spec.size()) {
        const size_t bend = qb.spec.find(',', bpos);
        const std::string pr = qb.spec.substr(
            bpos, bend == std::string::npos ? std::string::npos : bend - bpos);
        bpos = (bend == std::string::npos) ? qb.spec.size() : bend + 1;
        const size_t colon = pr.find(':');
        if (colon == std::string::npos) continue;
        if (pr.substr(0, colon) == "skill") {
          const std::uint32_t sk =
              static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
          pe->swingLands = sk * 20u;
          pe->swordSkill = sk;
          continue;
        }
        if (pr.substr(0, colon) == "gold") {
          pe->gold = static_cast<std::uint32_t>(std::stoul(pr.substr(colon + 1)));
          continue;
        }
        world.debugGive(*pe,
                        static_cast<std::uint32_t>(std::stoul(pr.substr(0, colon))),
                        static_cast<std::uint16_t>(std::stoul(pr.substr(colon + 1))));
      }
    }
    pe->hpMax = sim::playerHpMax(pe->level, pe->vit);
    pe->hp = pe->hpMax;
    if (L.inv != "-") {
      size_t pos = 0;
      const std::string& blob = L.inv;
      while (pos < blob.size()) {
        const size_t end = blob.find(';', pos);
        const std::string rec =
            blob.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        unsigned iid = 0, qty = 0, eqf = 0, aur = 0;
        if (std::sscanf(rec.c_str(), "%u:%u:%u:%u", &iid, &qty, &eqf, &aur) >= 3 &&
            qty > 0) {
          if (world.debugGive(*pe, iid, static_cast<std::uint16_t>(qty)) &&
              content::findItem(iid) != nullptr) {
            for (auto& sl : pe->inv) {
              if (sl.itemId == iid) {
                sl.aura = static_cast<std::uint8_t>(aur);
                if (eqf) sl.equipped = true;
                break;
              }
            }
          }
        }
        if (end == std::string::npos) break;
        pos = end + 1;
      }
    }
    entities.push_back(ReplayEnt{e.id});
  };
  // 1-based ticks match tickServer(): ++s.tick lands BEFORE cmds+world.tick()
  for (sim::Tick t = 1; t <= lastTick + 20; ++t) {
    while (li < logins.size() && logins[li].tick <= t) applyLogin(logins[li++]);
    while (di < drops.size() && drops[di].tick <= t) {
      if (drops[di].idx < entities.size()) world.despawn(entities[drops[di].idx].id);
      ++di;
    }
    while (ci < cmds.size() && cmds[ci].tick <= t) {
      const QueuedCmd& q = cmds[ci++];
      if (q.idx >= entities.size()) continue;
      Entity* e = world.find(entities[q.idx].id);
      if (e == nullptr) continue;
      // Journal lines carry the exact original Command fields; replay through
      // the same single application path as the live server (T-049).
      Command cmd;
      cmd.kind = static_cast<Command::Kind>(q.kind);
      cmd.a = q.a;
      cmd.b = q.b;
      cmd.channel = q.channel;
      applyWorldCommand(world, *e, cmd);
    }
    world.tick();
    if (std::getenv("BH_DUMP_ENTS") != nullptr) {
      for (const auto& ent : world.entities()) {
        std::fprintf(stderr,
                     "[replay-dump] t=%lld id=%u kind=%d wk=%u zone=%u qpos=%d:%d hp=%u skill=%u gold=%u mercymask=%u karma=%d atk=%u psz=%zu ch=%lld sw=%lld mv=%d tgt=%d:%d\n",
                     static_cast<long long>(t), ent.id, static_cast<int>(ent.kind),
                     ent.wireKind, ent.zoneId, ent.walker.x, ent.walker.y, ent.hp,
                     ent.swordSkill, ent.gold, ent.anvilMercyMask, ent.karma,
                     ent.attackTarget, ent.path.size(),
                     static_cast<long long>(ent.lastChaseTick), static_cast<long long>(ent.lastSwingTick),
                     ent.walker.moving ? 1 : 0, ent.walker.target.x, ent.walker.target.y);
      }
    }
    while (hi < hashes.size() && hashes[hi].tick <= t) {
      const QueuedHash& qh = hashes[hi++];
      ++checked;
      if (world.worldHash() != qh.expected) {
        ++bad;
        std::fprintf(stdout, "[replay] MISMATCH tick=%lld expected=%016llx got=%016llx\n",
                     static_cast<long long>(qh.tick),
                     static_cast<unsigned long long>(qh.expected),
                     static_cast<unsigned long long>(world.worldHash()));
        if (bad == 1) {
          
          for (const auto& ent : world.entities()) {
            {
              std::fprintf(stderr,
                           "[replay-dump] id=%u kind=%d wk=%u zone=%u qpos=%d:%d hp=%u skill=%u gold=%u mercymask=%u karma=%d\n",
                           ent.id, static_cast<int>(ent.kind), ent.wireKind, ent.zoneId,
                           ent.walker.x, ent.walker.y, ent.hp,
                           ent.swordSkill, ent.gold, ent.anvilMercyMask, ent.karma);
            }
          }
          std::fprintf(stderr, "[replay-dump] total entities=%zu tickCnt=%llu\n",
                       world.entities().size(),
                       static_cast<unsigned long long>(world.tickCount()));
        }
      }
    }
  }
  std::printf("[replay] %s ticks=%lld sessionCmds=%zu hashes=%lld mismatches=%lld "
              "entities=%zu\n",
              bad == 0 ? "OK" : "FAIL", static_cast<long long>(lastTick), cmds.size(),
              static_cast<long long>(checked), static_cast<long long>(bad),
              world.count());
  return bad == 0 ? 0 : 3;
}

int run(int argc, char** argv) {
  std::string mapPath = "assets/maps/thornwall.bhmap";
  std::string dbPath = "var/bh_server.db";
  std::uint16_t port = 7777;
  Server s;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next = [&](const char* dflt) -> std::string {
      return i + 1 < argc ? argv[++i] : dflt;
    };
    if (a == "--map") mapPath = next(mapPath.c_str());
    else if (a == "--db") dbPath = next(dbPath.c_str());
    else if (a == "--port") port = static_cast<std::uint16_t>(std::atoi(next("7777").c_str()));
    else if (a == "--soak-secs") s.soakSecs = std::atoll(next("0").c_str());
    else if (a == "--record-world") s.recordWorldPath = next("");
    else if (a == "--bless") {
      const std::string spec = next("");
      const size_t eq = spec.find('=');
      if (eq != std::string::npos) s.bless[spec.substr(0, eq)] = spec.substr(eq + 1);
    }
    else if (a == "--replay-world") s.replayWorldPath = next("");
    else if (a == "--p99-budget-ms") s.p99BudgetMs = std::atof(next("10").c_str());
    else {
      std::fprintf(stderr,
                   "usage: bh_server [--map M] [--db D] [--port P] [--soak-secs S] "
                   "[--p99-budget-ms N]\n");
      return 2;
    }
  }

  std::string err;
  if (!s.world.load(mapPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    return 1;
  }
  // T-036/T-035: load every zone that's present (optional on old deployments)
  for (const auto& [zid, zpath] : {std::pair<std::uint16_t, const char*>{2, "assets/maps/fields_overflow.bhmap"},
                                   {3, "assets/maps/thornwall_crypt.bhmap"}}) {
    std::string zerr;
    if (s.world.loadZone(zid, zpath, &zerr)) {
      const sim::Map* zm = s.world.zoneMap(zid);
      std::printf("[boot] zone %u (%s: %dx%d) online\n", static_cast<unsigned>(zid),
                  zpath, zm ? zm->w : 0, zm ? zm->h : 0);
      std::fflush(stdout);
    } else {
      std::printf("[boot] zone %u (%s) unavailable: %s — its portals will refuse\n",
                  static_cast<unsigned>(zid), zpath, zerr.c_str());
    }
  }
  if (!s.db.open(dbPath, &err)) {
    std::fprintf(stderr, "bh_server: %s\n", err.c_str());
    return 1;
  }
  if (!s.replayWorldPath.empty()) {
    return runReplayWorld(s.replayWorldPath, mapPath);  // offline deterministic mode
  }
  if (!s.recordWorldPath.empty()) {
    s.journal = std::fopen(s.recordWorldPath.c_str(), "w");
    if (s.journal == nullptr) {
      std::fprintf(stderr, "bh_server: cannot open journal '%s'\n",
                   s.recordWorldPath.c_str());
      return 1;
    }
    std::fprintf(stderr, "[journal] recording world to %s\n", s.recordWorldPath.c_str());
  }
  if (enet_initialize() != 0) {
    std::fprintf(stderr, "bh_server: enet_initialize failed\n");
    return 1;
  }
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = port;
  s.host = enet_host_create(&addr, 1024, 2, 0, 0);
  if (s.host == nullptr) {
    std::fprintf(stderr, "bh_server: enet_host_create failed\n");
    return 1;
  }
  std::printf("[boot] bh_server on 0.0.0.0:%u | map '%s' | db '%s' | tick 20Hz | aoi %d\n",
              port, mapPath.c_str(), dbPath.c_str(), kAoiRadius);
  std::fflush(stdout);

  s.startEpochSec = ::time(nullptr);
  auto nextTick = std::chrono::steady_clock::now();
  auto startTime = nextTick;

  for (;;) {
    // pump network with a small budget so ticks stay on schedule
    const auto now = std::chrono::steady_clock::now();
    auto msLeft = std::chrono::duration_cast<std::chrono::milliseconds>(nextTick - now).count();
    int svcMs = msLeft > 2 ? 2 : (msLeft > 0 ? static_cast<int>(msLeft) : 0);
    ENetEvent ev;
    while (enet_host_service(s.host, &ev, svcMs) > 0) {
      svcMs = 0;
      switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT:
          s.sessions.emplace(ev.peer, Session{});
          s.sessions[ev.peer].peer = ev.peer;
          break;
        case ENET_EVENT_TYPE_RECEIVE: {
          ++s.packetsIn;
          Session& sess = s.sessions[ev.peer];
          const auto pv = proto::view(static_cast<std::uint8_t*>(ev.packet->data),
                                      ev.packet->dataLength);
          if (pv.ok) handlePacket(s, sess, pv);
          enet_packet_destroy(ev.packet);
          break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
          auto it = s.sessions.find(ev.peer);
          if (it != s.sessions.end()) {
            journalDisconnect(s, it->second);
            dropSession(s, it->second);
          }
          break;
        }
        default:
          break;
      }
    }

    const auto now2 = std::chrono::steady_clock::now();
    if (now2 >= nextTick) {
      tickServer(s);
      nextTick += std::chrono::microseconds(kTickMicros);
      if (nextTick < now2 - std::chrono::milliseconds(250)) {
        std::fprintf(stderr, "[warn] tick budget blown; resynchronizing\n");
        nextTick = now2 + std::chrono::microseconds(kTickMicros);
      }
    }

    if (s.soakSecs > 0 &&
        now2 - startTime >= std::chrono::seconds(s.soakSecs)) {
      break;
    }
  }

  // ---- soak verdict (M1 gate) ----
  std::vector<std::int64_t> sorted = s.tickMicros;
  std::sort(sorted.begin(), sorted.end());
  const std::int64_t p50 = sorted.empty() ? 0 : sorted[sorted.size() / 2];
  const std::int64_t p99 = sorted.empty() ? 0 : sorted[sorted.size() * 99 / 100];
  const double p99ms = static_cast<double>(p99) / 1000.0;
  std::printf("[soak] FINISH ticks=%lld online=%zu entities=%zu tickUs p50=%lld p99=%lld\n",
              static_cast<long long>(s.tick), s.sessions.size(), s.world.count(),
              static_cast<long long>(p50), static_cast<long long>(p99));
  const bool ok = p99ms <= s.p99BudgetMs;
  std::printf("[soak] %s p99=%.2fms budget=%.2fms\n", ok ? "OK" : "FAIL", p99ms, s.p99BudgetMs);
  // balancer report (T-031): one line per mob kind, sorted by level
  for (const content::MobDef& d : content::kMobs) {
    const auto it = s.world.killStats.find(d.mobId);
    if (it == s.world.killStats.end() || it->second.kills == 0) continue;
    const double ttkSec = it->second.emaTtkTicks * 0.2;
    std::printf("[balance] %-16s L%-2d kills=%-5lld TTK(25t hps)=%.1fs xp=%u\n",
                d.name, d.level, static_cast<long long>(it->second.kills),
                ttkSec, d.xp);
  }
  std::fflush(stdout);
  std::fflush(stdout);

  for (auto& kv : s.sessions) {
    if (kv.second.inWorld) journalDisconnect(s, kv.second);  // deterministic close
    enet_peer_disconnect_now(kv.first, 0);
  }
  if (s.journal != nullptr) {
    journalTickHash(s);  // final marker: the wipe/final state
    std::fclose(s.journal);
    s.journal = nullptr;
  }
  enet_host_destroy(s.host);
  enet_deinitialize();
  return ok ? 0 : 2;
}

}  // namespace
}  // namespace bh::server

int main(int argc, char** argv) { return bh::server::run(argc, argv); }
