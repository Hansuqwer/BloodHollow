#include "net_client.h"

#include <chrono>

#include "protocol/messages_gen.h"

namespace bh {

NetClient::~NetClient() {
  if (peer_ != nullptr) enet_peer_disconnect_now(peer_, 0);
  if (host_ != nullptr) {
    enet_host_destroy(host_);
    enet_deinitialize();
  }
}

std::uint32_t NetClient::nowMs32() const {
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();
  return static_cast<std::uint32_t>(ms & 0xFFFFFFFFu);
}

bool NetClient::connectTo(const std::string& host, std::uint16_t port,
                          const std::string& user, const std::string& pass,
                          std::string* err) {
  user_ = user;
  pass_ = pass;
  if (enet_initialize() != 0) {
    if (err) *err = "enet_initialize failed";
    return false;
  }
  host_ = enet_host_create(nullptr, 1, 2, 0, 0);
  if (host_ == nullptr) {
    if (err) *err = "enet_host_create failed";
    return false;
  }
  ENetAddress addr;
  if (enet_address_set_host(&addr, host.c_str()) != 0) {
    if (err) *err = "cannot resolve " + host;
    return false;
  }
  addr.port = port;
  peer_ = enet_host_connect(host_, &addr, 2, 0);
  if (peer_ == nullptr) {
    if (err) *err = "connect allocation failed";
    return false;
  }
  // blocking wait for the connect event (max 5 s)
  ENetEvent ev;
  const int rc = enet_host_service(host_, &ev, 5000);
  if (rc > 0 && ev.type == ENET_EVENT_TYPE_CONNECT) {
    proto::Hello h;
    h.protoVersion = proto::kProtocolVersion;
    h.username = user_;
    h.password = pass_;
    const auto bytes = proto::pack(h);
    ENetPacket* pkt = enet_packet_create(bytes.data(), bytes.size(),
                                         ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 0, pkt);
    enet_host_flush(host_);
    state = State::kAwaitingLogin;
    return true;
  }
  if (err) *err = "no answer from " + host + ":" + std::to_string(port);
  state = State::kFailed;
  return false;
}

void NetClient::sendPath(int goalX, int goalY) {
  if (state != State::kInWorld) return;
  proto::InputPath m;
  m.goalX = goalX;
  m.goalY = goalY;
  {
    const auto bytes = proto::pack(m);
    enet_peer_send(peer_, 0, enet_packet_create(bytes.data(), bytes.size(),
                                                ENET_PACKET_FLAG_RELIABLE));
  }
}

void NetClient::sendStep(int dx, int dy) {
  if (state != State::kInWorld) return;
  proto::InputStep m;
  m.dx = static_cast<std::int8_t>(dx);
  m.dy = static_cast<std::int8_t>(dy);
  const auto bytes = proto::pack(m);
  enet_peer_send(peer_, 0,
                 enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendChat(std::uint8_t channel, const std::string& text) {
  if (state != State::kInWorld) return;
  proto::ChatSend m;
  m.channel = channel;
  m.text = text;
  const auto bytes = proto::pack(m);
  enet_peer_send(peer_, 0,
                 enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendAttack(std::uint32_t targetId) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::AttackRequest m;
  m.targetId = targetId;
  const auto bytes = proto::pack(m);
  enet_peer_send(peer_, 0,
                 enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendStat(std::uint8_t stat) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::StatAssign m;
  m.stat = stat;
  const auto bytes = proto::pack(m);
  enet_peer_send(peer_, 0,
                 enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendUseItem(std::uint8_t slot) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::UseItem m;
  m.slot = slot;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}
void NetClient::sendToggleEquip(std::uint8_t slot) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::ToggleEquip m;
  m.slot = slot;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}
void NetClient::sendSkill(std::uint8_t skill, std::uint32_t targetId) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::SkillUse m;
  m.skill = skill;
  m.targetId = targetId;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}
void NetClient::sendBuy(std::uint32_t itemId, std::uint16_t qty) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::BuyRequest m;
  m.itemId = itemId;
  m.qty = qty;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}
void NetClient::sendSellJunk() {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::SellJunk m;
  m.unused = 0;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendAnvilOp(std::uint8_t tier) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::AnvilOp m;
  m.tier = tier;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendTradeOpen(std::uint32_t targetId) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::TradeOpen m;
  m.targetId = targetId;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendTradeOfferItem(std::uint32_t itemId, std::uint16_t qty) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::TradeOfferItem m;
  m.itemId = itemId;
  m.qty = qty;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendTradeOfferGold(std::uint32_t gold) {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::TradeOfferGold m;
  m.gold = gold;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
  tradeGoldOffered = gold;
}

void NetClient::sendTradeCommit() {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::TradeCommit m;
  m.unused = 0;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::sendTradeCancel() {
  if (peer_ == nullptr || state != State::kInWorld) return;
  proto::TradeCancel m;
  m.unused = 0;
  const auto b = proto::pack(m);
  enet_peer_send(peer_, 0, enet_packet_create(b.data(), b.size(), ENET_PACKET_FLAG_RELIABLE));
  tradeWithId = 0;
  tradeGoldOffered = 0;
}

void NetClient::sendPing() {
  if (peer_ == nullptr || state < State::kAwaitingLogin) return;
  proto::Ping m;
  m.clientTimeMs = nowMs32();
  const auto bytes = proto::pack(m);
  enet_peer_send(peer_, 0,
                 enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE));
}

void NetClient::poll() {
  if (host_ == nullptr) return;
  ENetEvent ev;
  while (enet_host_service(host_, &ev, 0) > 0) {
    switch (ev.type) {
      case ENET_EVENT_TYPE_RECEIVE: {
        const auto pv = proto::view(static_cast<std::uint8_t*>(ev.packet->data),
                                    ev.packet->dataLength);
        if (pv.ok) {
          using namespace proto;
          switch (pv.id) {
            case kIdLoginResult: {
              LoginResult r;
              if (r.deserialize(pv.body)) {
                if (r.ok == 0) {
                  state = State::kFailed;
                  failReason = "login refused (reason " + std::to_string(r.reason) + ")";
                }
              }
              break;
            }
            case kIdWelcome: {
              Welcome w;
              if (!w.deserialize(pv.body)) break;
              state = State::kInWorld;
              welcomed = true;
              ownId = w.entityId;
              mapId = w.mapId;
              serverTick = w.tick;
              welcomeHour = static_cast<float>(w.hourCenti) / 100.0f;
              welcomeTickLocal = w.tick;
              NetEntSnapshot s0;
              s0.id = w.entityId;
              s0.x = w.x;
              s0.y = w.y;
              s0.hp = 100;
              s0.hpMax = 100;
              ents[w.entityId] = s0;  // name filled by its EntitySpawn
              break;
            }
            case kIdEntitySpawn: {
              EntitySpawn m;
              if (!m.deserialize(pv.body)) break;
              NetEntSnapshot s0;
              s0.id = m.id;
              s0.kind = m.kind;
              s0.x = m.x;
              s0.y = m.y;
              s0.dir = m.dir;
              s0.hp = m.hp;
              s0.hpMax = m.hpMax;
              s0.level = m.level;
              s0.name = m.name;
              s0.karmaBand = m.kind == 0 ? m.karmaBand : 1;  // players only
              ents[m.id] = s0;
              spawnedIds.push_back(m.id);
              break;
            }
            case kIdEntityDelta: {
              EntityDelta d;
              if (!d.deserialize(pv.body)) break;
              auto it = ents.find(d.id);
              if (it == ents.end()) break;  // spawn not seen yet; its spawn will carry pos
              it->second.x = d.x;
              it->second.y = d.y;
              it->second.dir = d.dir;
              it->second.moving = d.moving != 0;
              it->second.hp = d.hp;
              break;
            }
            case kIdEntityDespawn: {
              EntityDespawn d;
              if (!d.deserialize(pv.body)) break;
              ents.erase(d.id);
              despawnedIds.push_back(d.id);
              break;
            }
            case kIdChatMsg: {
              ChatMsg m;
              if (!m.deserialize(pv.body)) break;
              chatIn.push_back(ChatLine{m.channel, m.from, m.text});
              break;
            }
            case kIdCombatEvent: {
              CombatEvent m;
              if (!m.deserialize(pv.body)) break;
              combatIn.push_back(CombatPulse{m.attackerId, m.targetId, m.kind, m.amount});
              break;
            }
            case kIdOwnStats: {
              OwnStats m;
              if (!m.deserialize(pv.body)) break;
              ownStats.level = m.level;
              ownStats.xp = m.xp;
              ownStats.xpNext = m.xpNext == 0 ? 1 : m.xpNext;
              ownStats.statPoints = m.statPoints;
              ownStats.str = m.str;
              ownStats.vit = m.vit;
              ownStats.dex = m.dex;
              ownStats.swordSkill = m.swordSkill;
              ownStats.gold = m.gold;
              ownStats.karma = m.karma;
              ownStats.classId = m.classId;
              ownStats.mp = m.mp;
              ownStats.mpMax = m.mpMax == 0 ? 1 : m.mpMax;
              ownStats.blessTicksLeft = m.blessTicksLeft;
              ownStats.ironskinTicksLeft = m.ironskinTicksLeft;
              break;
            }
            case kIdPartyReset: {
              PartyReset m;
              if (!m.deserialize(pv.body)) break;
              partyId = m.partyId;
              partyLeaderId = m.leaderId;
              party.clear();
              break;
            }
            case kIdPartyMember: {
              PartyMember m;
              if (!m.deserialize(pv.body)) break;
              party.push_back(PartyMemberWire{m.entityId, m.name, m.level, m.hp,
                                              m.hpMax == 0 ? 1 : m.hpMax, m.zoneId});
              break;
            }
            case kIdInventoryReset: {
              InventoryReset m;
              if (!m.deserialize(pv.body)) break;
              inventory.clear();
              break;
            }
            case kIdItemSlot: {
              ItemSlot m;
              if (!m.deserialize(pv.body)) break;
              inventory[m.slot] = InvSlotWire{m.itemId, m.qty, m.equipped != 0};
              break;
            }
            case kIdPong: {
              Pong p;
              if (!p.deserialize(pv.body)) break;
              pingMs = static_cast<int>(nowMs32() - p.clientTimeMs);
              serverTick = p.serverTick;
              break;
            }
            case kIdKickNotice: {
              KickNotice k;
              if (k.deserialize(pv.body)) failReason = k.reason;
              break;
            }
            case kIdServerStats: {
              ServerStats st;
              if (!st.deserialize(pv.body)) break;
              online = st.online;
              serverP99Us = st.tickMicrosP99;
              break;
            }
            default:
              break;
          }
        }
        enet_packet_destroy(ev.packet);
        break;
      }
      case ENET_EVENT_TYPE_DISCONNECT:
        if (state != State::kFailed) state = State::kDisconnected;
        break;
      default:
        break;
    }
  }
}

}  // namespace bh
