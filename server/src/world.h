#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>

#include "content/items.h"
#include "content/mobs.h"
#include "sim/bhmap.h"
#include "sim/rng.h"
#include "sim/spatial.h"
#include "sim/tick.h"
#include "sim/walker.h"

namespace bh::server {

enum class EntityKind : std::uint8_t { kPlayer = 0, kMob = 1 };

// Generic world event emitted by the sim for the net layer to distribute.
// kind 0=miss,1=hit,2=crit,3=kill,4=heal,5=skillhit mirror proto::CombatEvent.
// chatCh 0 = none, 255 = directed system line to the session owning aboutId.
struct WorldEvent {
  std::uint32_t attacker = 0;
  std::uint32_t target = 0;
  std::uint8_t kind = 0;
  std::uint16_t amount = 0;
  std::uint8_t chatCh = 0;     // 0 = none; 255 = directed; else chat channel
  std::string chatText{};      // server-originated (system/death) lines
  bool zoneChanged = false;  // player crossed a portal (client reloads map)
  std::uint32_t aboutId = 0;   // stat push / level-up / inventory target
  bool statsChanged = false;
  bool invChanged = false;
  bool partyChanged = false;  // T-050: target=partyId (0 => aboutId left/kicked)
};

// one inventory stack; item schema lives in shared/content/items.h
struct InvSlot {
  std::uint32_t itemId = 0;
  std::uint16_t qty = 0;
  bool equipped = false;
  std::uint8_t aura = 0;   // T-042: applied aura tier (0=none, 1..5 per RFC 0001)
};

struct Entity {
  std::uint32_t id = 0;
  EntityKind kind = EntityKind::kPlayer;
  sim::Walker walker{};
  std::deque<sim::TilePos> path{};
  std::uint32_t hp = 100;
  std::uint32_t hpMax = 100;
  std::string name{};
  std::int64_t charRowId = 0;  // persistence link (0 = transient)

  // progression (players)
  std::uint8_t level = 1;
  std::uint32_t xp = 0;
  std::uint8_t str = 8, vit = 8, dex = 8;
  std::uint8_t statPoints = 0;

  // combat
  std::uint32_t attackTarget = 0;
  sim::Tick lastSwingTick = -1000;
  sim::Tick lastHurtTick = -1000;
  bool dead = false;
  sim::Tick respawnAt = 0;

  // inventory / economy (players)
  std::vector<InvSlot> inv{};
  std::uint32_t gold = 50;
  std::uint8_t swordSkill = 0;  // use-based (Soma adoption): +1 per lands
  std::uint32_t swingLands = 0;
  sim::Tick lastSipTick = -1000;
  sim::Tick lastPowerTick = -1000;
  sim::Tick lastChaseTick = -1000;
  sim::Tick firstHurtTick = -1;   // mob TTK probe (balancer, T-031)
  std::uint32_t anvilMercyMask = 0;
  std::int32_t karma = 0;  // moral economy (T-046): >0 = +15% XP, <0 = +15% gold loot  // bit per tier: used/not-used (S9 T-043 persist)
  std::uint16_t zoneId = 1;        // T-036: which zone this entity lives in
  sim::Tick lastPortalTick = -1000;  // arrival grace against portal ping-pong

  std::uint32_t partyId = 0;  // 0 = unaffiliated (T-050 party core)

  // class kit (T-053/T-054): kit id (content/kits.h), mana, buff stamps, cast CDs
  std::uint8_t classId = 1;     // kKitRavager default: pre-kit chars unchanged
  std::uint8_t intg = 0, mag = 0;
  std::uint32_t mp = 0, mpMax = 30;
  sim::Tick blessUntil = -1;    // +10% hit&dmg (T-054)
  sim::Tick ironskinUntil = -1; // +20% DR (T-054)
  sim::Tick lastMendTick = -1000;
  sim::Tick lastBlessTick = -1000;
  sim::Tick lastIronskinTick = -1000;
  sim::Tick lastFireboltTick = -1000;

  // trade (T-029): intents only until BOTH commit; swap validated at commit.
  std::uint32_t tradeWith = 0;
  std::vector<std::pair<std::uint32_t, std::uint16_t>> tradeOfferItems{};  // itemId, qty
  std::uint32_t tradeOfferGold = 0;
  bool tradeCommitted = false;

  // mobs
  std::uint32_t mobId = 0;
  sim::TilePos anchor{-1, -1};
  std::uint8_t aggroRadius = 0;
  std::uint8_t wanderRadius = 0;
  std::uint8_t leashRadius = 0;
  std::uint16_t atkCdTicks = 16;
  std::uint32_t xpValue = 0;
  std::uint8_t mobLevel = 1;
  std::uint8_t wireKind = 0;  // on the wire: 0 player, 1..63 mob, 64 vendor
  size_t spawnerIdx = SIZE_MAX;
};

// rolling per-mob-kind kill stats for the balancer (T-031)
struct MobKillStat {
  std::int64_t kills = 0;
  double emaTtkTicks = 0.0;
};

class World {
 public:
  // T-036 zones-in-process (public: also used by test harnesses)
  struct SpawnerLive {
    sim::SpawnDef def{};
    sim::Tick respawnReadyAt = 0;  // one mob respawns at a time when due
  };
  struct Zone {
    sim::Map map{};
    sim::CostGrid grid{};
    sim::SpatialGrid<8> spatial{};
    std::vector<SpawnerLive> spawners{};
    sim::TilePos spawnPoint{1, 1};
    bool vendorSeeded = false;  // Marta lives in zone 1 only (S6 scope)
  };
  Zone& zoneOf(const Entity& e) { return zones_.at(e.zoneId); }
  const Zone& zoneOf(const Entity& e) const { return zones_.at(e.zoneId); }

  bool load(const std::string& mapPath, std::string* err);
  bool loadFrom(sim::Map map);  // tests: in-memory map, spawner-driven mobs spawn
  bool loadZone(std::uint16_t mapId, const std::string& mapPath,
                std::string* err);  // T-036: additional zone(s) in-process
  const sim::Map* zoneMap(std::uint16_t mapId) const;
  bool loadZoneFrom(std::uint16_t mapId, sim::Map map);  // tests + loadZone impl
  // portal transfer; returns false when no portal/zone available
  bool checkPortals(Entity& e);
  bool transferToZone(Entity& e, std::uint16_t mapId, sim::TilePos at);

  Entity& spawn(const std::string& name, std::int64_t charRowId,
                std::optional<sim::TilePos> at, std::uint16_t zoneId = 1,
                std::uint8_t classId = 1);  // kKitRavager default (freeze)
  void despawn(std::uint32_t id);

  Entity* find(std::uint32_t id);
  const Entity* find(std::uint32_t id) const;

  void queuePath(Entity& e, sim::TilePos goal);

  // combat / progression entry points (also cancel movement-attack locks)
  void setAttack(Entity& self, std::uint32_t targetId);
  bool assignStat(Entity& e, std::uint8_t stat);  // 0 str, 1 vit, 2 dex
  bool useItem(Entity& e, std::uint8_t slot);        // consumable (sip cd 0.5s)
  bool toggleEquip(Entity& e, std::uint8_t slot);
  void trySkill(Entity& e, std::uint8_t skill, std::uint32_t targetId);
  bool kitChoose(Entity& e, std::uint8_t kitId);  // T-053 one-time swear
  std::uint32_t effAcc(const Entity& e) const;    // bless-adjusted accuracy
  std::uint32_t effDmgBase(const Entity& e) const;
  std::uint32_t effDef(const Entity& e) const;    // ironskin-adjusted mitigation

 private:
  void tryMend(Entity& e, std::uint32_t targetId);      // T-054 (chan 2)
  void tryBless(Entity& e, std::uint32_t targetId);     // T-054 (chan 3)
  void tryIronskin(Entity& e, std::uint32_t targetId);  // T-054 (chan 4)
  void tryFirebolt(Entity& e, std::uint32_t targetId);  // T-054 (chan 5)

 public:
  bool vendorBuy(Entity& e, std::uint32_t itemId, std::uint16_t qty);
  // ---- party (T-050/T-051): roster + XP share -----------------------------
  struct Party {
    std::uint32_t id = 0;
    std::uint32_t leaderId = 0;
    std::vector<std::uint32_t> members;
  };
  static constexpr int kPartyMaxMembers = 8;      // M3-era raid-ish cap for later
  static constexpr int kPartyXpRadius = 12;       // share window, tiles (Chebyshev)
  static constexpr std::uint32_t kPartyXpBonusPct = 12;  // per extra in-range member
  bool partyInvite(Entity& inviter, Entity& target);   // creates party when needed
  bool partyAccept(Entity& e);                         // consume pending invite
  bool partyLeave(Entity& e);                          // also called from despawn
  bool partyKick(Entity& leader, std::uint32_t targetId);
  const Party* partyOf(std::uint32_t entityId) const;
  void emitPartyMsg(std::uint32_t partyId, std::uint32_t aboutId,
                    const std::string& text);
  const std::deque<Party>& parties() const { return parties_; }

  // anvil (T-041/T-042): proximity-gated aura attempts, atomic part+gold tolls
  bool tryAnvil(Entity& e, std::uint8_t tier);
  static constexpr std::int32_t kKarmaAnvilOk = 2;      // craft tithe
  static constexpr std::int32_t kKarmaAnvilDestroy = -6;  // the Widow collects
  bool nearAnvil(const Entity& e) const;
  std::uint32_t vendorSellJunk(Entity& e);  // returns gold gained
  void spawnVendor();                        // called from load
  void spawnVendor(Zone& zone);              // zone 1 town only
  void spawnAnvils();                        // plaza (z1) + bone barrow (z3)
  std::uint8_t anvilTilesAllowed(std::uint16_t zoneId) const;
  // trade window (transactional by construction; see ADR-0011)
  bool tradeOpen(Entity& a, std::uint32_t partnerId);
  void tradeOffer(Entity& e, std::uint32_t itemId, std::uint16_t qty);
  void tradeOfferGold(Entity& e, std::uint32_t gold);
  void tradeCommit(Entity& e);   // executes the swap when both committed
  void tradeCancel(Entity& e, const char* why);
  // test/debug knob (gm tooling later): grant an item without checks
  bool debugGive(Entity& e, std::uint32_t itemId, std::uint16_t qty) {
    return addItem(e, itemId, qty);
  }
  std::unordered_map<std::uint32_t, MobKillStat> killStats;
  // T-047: tier-III cleave defers killMob() so deque references stay valid
  std::vector<std::uint32_t> pendingCleaveKills_{};
  std::uint32_t pendingCleaveAttacker_ = 0;
  std::uint16_t pendingCleaveDmg_ = 0;  // mobId -> stats

  // test/gm introspection
  std::uint32_t debugWeaponDmg(const Entity& e) const { return equippedWeaponDmg(e); }
  void debugKillPlayer(Entity& e) { killPlayer(e, nullptr); }
  void debugAwardXp(Entity& e, std::uint32_t amt) { awardXp(e, amt); }
  void debugKillMob(Entity& mob, Entity* killer) { killMob(mob, killer); }
  Entity& debugSpawnMob(const content::MobDef& def, sim::TilePos at,
                        std::uint16_t zoneId = 1) {
    return spawnMob(def, at, /*spawnerIdx=*/SIZE_MAX, zoneId);
  }

  void tick();  // one 20 Hz step: movement, mob AI, combat, respawns

  std::vector<std::uint32_t> queryAoi(std::uint16_t zoneId, int x, int y,
                                      int radiusTiles) const;

  const std::vector<WorldEvent>& events() const { return events_; }
  std::deque<Entity>& entities() { return entities_; }
  // legacy single-zone accessors (zone 1) used by main/tests/bots
  const sim::Map& map() const { return zones_.at(1).map; }
  const sim::CostGrid& grid() const { return zones_.at(1).grid; }
  sim::TilePos spawnPoint() const { return zones_.at(1).spawnPoint; }
  // zone-aware variants
  const sim::Map& mapOf(const Entity& e) const { return zones_.at(e.zoneId).map; }
  const sim::CostGrid& gridOf(const Entity& e) const { return zones_.at(e.zoneId).grid; }
  std::uint64_t worldHash() const;  // soak-mode deterministic evidence
  size_t count() const { return entities_.size(); }
  sim::Tick tickCount() const { return tick_; }
  sim::Rng& rng() { return rng_; }

 private:

  void initialMobSpawns(Zone& zone, std::uint16_t zoneId);
  Entity& spawnMob(const content::MobDef& def, sim::TilePos at, size_t spawnerIdx,
                   std::uint16_t zoneId);
  Entity& insertEntity(Entity e);
  void mobThink(Entity& mob);
  void trySwing(Entity& att, Entity& def);
  void killMob(Entity& mob, Entity* killer);
  void killPlayer(Entity& victim, Entity* killer);
  void awardXp(Entity& player, std::uint32_t amount);
  std::uint32_t recomputeHpMax(Entity& e) const;
  void respawnTick();
  std::vector<Entity*> playersNear(Zone& zone, int x, int y, int radius);
  // inventory helpers (sorted by itemId; equips occupy their slot domain)
  bool addItem(Entity& e, std::uint32_t itemId, std::uint16_t qty);
  std::uint32_t equippedWeaponDmg(const Entity& e) const;
  std::uint32_t equippedArmorDef(const Entity& e) const;
  void syncIndxPush(Entity& e) {  // mark inventory changed for the net layer
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.invChanged = true;
    events_.push_back(std::move(ev));
  }

  std::unordered_map<std::uint16_t, Zone> zones_{};
  std::deque<Entity> entities_{};
  std::deque<Party> parties_;
  std::uint32_t nextPartyId_ = 1;
  // pending invites: invitee entity id -> (expiryTick, inviter entity id)
  std::vector<std::pair<std::uint32_t, std::pair<sim::Tick, std::uint32_t>>> invites_;
  std::uint32_t nextId_ = 1;
  sim::Rng rng_{0xB100D11A33ULL};
  sim::Tick tick_ = 0;
  std::vector<WorldEvent> events_{};

};

}  // namespace bh::server
