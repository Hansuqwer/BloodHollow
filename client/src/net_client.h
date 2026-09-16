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
  std::uint8_t karmaBand = 1;  // T-057: 0 lawful / 1 neutral / 2 chaotic (red)
  std::uint8_t light = 0;      // T-071: carried light radius (tiles)
  std::uint8_t glowTier = 0;   // T-092: equipped-weapon refine glow (0/1/2)
  std::uint8_t classId = 0;    // T-142: kit id (0 unsworn/none, 1 Ravager, 2 Gravecaller, 3 Cultist)
  std::uint8_t sex = 0;        // T-142: 0 unknown (T-142b captures), 1 m, 2 f
};

// Server-pushed progression snapshot (OwnStats message).
struct OwnStatsWire {
  std::uint16_t level = 1;
  std::uint32_t xp = 0;
  std::uint32_t xpNext = 100;
  std::uint8_t statPoints = 0;
  std::uint8_t str = 8, vit = 8, dex = 8;
  std::uint8_t intg = 0, mag = 0;  // T-160: carried by OwnStats since kits
  std::uint16_t swordSkill = 0;
  std::uint32_t gold = 0;
  std::int32_t karma = 0;
  std::uint8_t classId = 1;       // kit (kits.h) — stat panel label
  std::uint32_t mp = 0, mpMax = 30;
  std::uint16_t blessTicksLeft = 0, ironskinTicksLeft = 0;
  std::uint16_t curseTicksLeft = 0;  // T-070 thin blood readout
};

struct PartyMemberWire {
  std::uint32_t entityId = 0;
  std::string name;
  std::uint16_t level = 1;
  std::uint32_t hp = 0;
  std::uint32_t hpMax = 1;
  std::uint16_t zoneId = 0;
};

struct SiegeStateWire {
  std::uint32_t holderPledgeId = 0;
  std::string holderPledgeName;
  std::string holderName;
  std::uint32_t windowEndTick = 0;
  std::uint32_t battleEndTick = 0;
  std::uint16_t gateHp0 = 0;
  std::uint16_t gateHp1 = 0;
  std::uint16_t heartProgress = 0;
  std::uint8_t heartAttuned = 0;
  std::uint32_t crownOwnerId = 0;
  std::string crownOwnerName;
  std::uint32_t crownDeadline = 0;
  std::uint8_t bandCount = 0;
  std::uint8_t phase = 0;
  std::uint32_t vaultGold = 0;
  std::uint32_t crowns = 0;
};

struct PledgeMemberWire {
  std::string name;
  std::uint8_t rank = 0;
  std::uint16_t level = 1;
  std::uint8_t online = 0;
};

struct InvSlotWire {
  std::uint32_t itemId = 0;
  std::uint16_t qty = 0;
  bool equipped = false;
  std::uint8_t aura = 0;  // 0..5 (tint in inventory rows)
  std::uint8_t durability = 100;  // T-058: 0 = dormant
  std::uint8_t affix = 0;  // T-059: 0 none, 1 whet, 2 ward, 3 leech; T-159 adds 11..20
  std::uint8_t refine = 0;  // T-060: 0..3, T-079: to +7 (T-ART-11 glows at 5+)
  std::uint8_t rarity = 0;  // T-159: 0 common, 1 magic, 2 rare, 3 unique
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
  // T-167 creation answer (pre-world: allowed while needsCreate, not inWorld)
  void sendCharCreate(std::uint8_t classId, std::uint8_t sex);
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
  bool needsCreate = false;  // T-167: CharCreatePrompt arrived, answer once
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
  SiegeStateWire siege{};  // T-151 castle memory
  std::uint32_t pledgeId = 0;
  std::string pledgeName;
  std::uint8_t pledgeEmblem = 0;
  std::uint32_t pledgeVault = 0;
  std::vector<PledgeMemberWire> pledgeMembers{};
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
