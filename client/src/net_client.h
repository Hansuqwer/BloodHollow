#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <enet/enet.h>

namespace bh {

// Latest authoritative snapshot of one entity (no interpolation here —
// Game owns presentation smoothing).
struct NetEntSnapshot {
  std::uint32_t id = 0;
  std::uint8_t kind = 0;
  int x = 0;      // Q10 fixed-point tile-space
  int y = 0;
  std::uint8_t dir = 2;
  bool moving = false;
  std::uint32_t hp = 0;
  std::uint32_t hpMax = 0;
  std::uint8_t level = 1;
  std::string name{};
};

// Server-pushed progression snapshot (OwnStats message).
struct OwnStatsWire {
  std::uint16_t level = 1;
  std::uint32_t xp = 0;
  std::uint32_t xpNext = 100;
  std::uint8_t statPoints = 0;
  std::uint8_t str = 8, vit = 8, dex = 8;
  std::uint16_t swordSkill = 0;
  std::uint32_t gold = 0;
  std::int32_t karma = 0;
};

struct PartyMemberWire {
  std::uint32_t entityId = 0;
  std::string name;
  std::uint16_t level = 1;
  std::uint32_t hp = 0;
  std::uint32_t hpMax = 1;
  std::uint16_t zoneId = 0;
};

struct InvSlotWire {
  std::uint32_t itemId = 0;
  std::uint16_t qty = 0;
  bool equipped = false;
  std::uint8_t aura = 0;  // 0..5 (tint in inventory rows)
};

// One combat pulse (hits, misses, kills) for floaters/flash.
struct CombatPulse {
  std::uint32_t attacker = 0;
  std::uint32_t target = 0;
  std::uint8_t kind = 0;  // 0 miss, 1 hit, 2 crit, 3 kill
  std::uint16_t amount = 0;
};

struct ChatLine {
  std::uint8_t channel = 0;  // 0 say, 1 global, 2 system
  std::string from{};
  std::string text{};
};

// Client-side ENet peer. Game consumes public state after each poll().
class NetClient {
 public:
  enum class State { kConnecting, kAwaitingLogin, kInWorld, kFailed, kDisconnected };

  ~NetClient();
  bool connectTo(const std::string& host, std::uint16_t port, const std::string& user,
                 const std::string& pass, std::string* err);
  void poll();  // drain network events; update state + pulse lists

  void sendPath(int goalX, int goalY);
  void sendStep(int dx, int dy);
  void sendChat(std::uint8_t channel, const std::string& text);
  void sendPing();
  void sendAttack(std::uint32_t targetId);
  void sendStat(std::uint8_t stat);
  void sendUseItem(std::uint8_t slot);
  void sendToggleEquip(std::uint8_t slot);
  void sendSkill(std::uint8_t skill, std::uint32_t targetId);
  void sendBuy(std::uint32_t itemId, std::uint16_t qty);
  void sendSellJunk();
  // anvil (T-041)
  void sendAnvilOp(std::uint8_t tier);
  // trade (T-029)
  void sendTradeOpen(std::uint32_t targetId);
  void sendTradeOfferItem(std::uint32_t itemId, std::uint16_t qty);
  void sendTradeOfferGold(std::uint32_t gold);
  void sendTradeCommit();
  void sendTradeCancel();

  // consumed once per frame by Game (then cleared via clearPulse()).
  State state = State::kConnecting;
  std::string failReason{};
  bool welcomed = false;
  std::uint32_t ownId = 0;
  // trade UI state (best-effort mirror; server chat lines are the truth)
  std::uint32_t tradeWithId = 0;
  std::string tradeWithName;
  std::uint32_t tradeGoldOffered = 0;
  std::uint32_t mapId = 0;
  std::uint32_t serverTick = 0;
  float welcomeHour = 8.0f;
  std::uint32_t welcomeTickLocal = 0;
  std::unordered_map<std::uint32_t, NetEntSnapshot> ents{};
  std::vector<std::uint32_t> spawnedIds{};
  std::vector<std::uint32_t> despawnedIds{};
  std::vector<ChatLine> chatIn{};
  std::vector<CombatPulse> combatIn{};
  OwnStatsWire ownStats{};
  std::unordered_map<std::uint8_t, InvSlotWire> inventory{};  // slot -> item
  std::uint32_t partyId = 0;
  std::uint32_t partyLeaderId = 0;
  std::vector<PartyMemberWire> party{};  // T-052 party frame data
  std::uint32_t online = 0;
  std::uint32_t serverP99Us = 0;
  int pingMs = -1;

  void clearPulse() {
    spawnedIds.clear();
    despawnedIds.clear();
    chatIn.clear();
    combatIn.clear();
  }

 private:
  std::uint32_t nowMs32() const;

  ENetHost* host_ = nullptr;
  ENetPeer* peer_ = nullptr;
  std::string user_{};
  std::string pass_{};
};

}  // namespace bh
