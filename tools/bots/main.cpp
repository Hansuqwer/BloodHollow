// bh_bots - N scripted headless clients hammering bh_server (ADR-008).
// Profiles: "wanderer" (login + random walks + chatter) and "fighter"
// (chases and kills nearest mob; combat soak). Sprint 5 addition.
#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <string>
#include <vector>

#include <enet/enet.h>

#include "protocol/messages_gen.h"
#include "content/auras.h"
#include "content/wirekind.h"
#include "sim/bhmap.h"
#include "sim/rng.h"
#include "sim/walker.h"

namespace {

struct SeenEnt {
  int x = 0, y = 0;
  std::uint8_t kind = 0;  // 0 = player
};

struct Bot {
  ENetPeer* peer = nullptr;
  bool welcomed = false;
  std::uint32_t ownId = 0;
  int tileX = 0, tileY = 0;
  int homeX = -1, homeY = -1;  // first login doubles as respawn anchor
  int visited = 0;
  double nextWanderDecideAt = 0.0;
  std::uint64_t deltasOnSelf = 0;
  double nextMoveAt = 0.0;
  std::string name{};
  // fighter profile + economy (S6)
  std::unordered_map<std::uint32_t, SeenEnt> ents{};
  std::uint32_t attackTarget = 0;
  double nextAttackAt = 0.0;
  std::uint64_t kills = 0;
  std::int32_t hp = 100, hpMax = 100;
  int level = 1;
  std::uint32_t gold = 0;
  struct InvRow { std::uint32_t itemId = 0; std::uint16_t qty = 0; bool equipped = false; std::uint8_t aura = 0; };
  std::unordered_map<std::uint8_t, InvRow> inv{};  // slot -> row
  double nextAnvilAt = 0.0;
  std::uint64_t anvilTries = 0;
  std::uint64_t dbgNoBlade = 0, dbgNoGold = 0, dbgNoPelts = 0, dbgNoAnvil = 0,
                dbgReady = 0;
  std::uint32_t dbgMaxGold = 0, dbgMaxPelts = 0;
  bool boughtVials = false;
  std::uint64_t potionsUsed = 0;
  std::uint64_t swings = 0;
  std::uint64_t deaths = 0;
  std::uint64_t shops = 0;
  std::uint64_t levelDrops = 0;
  double nextShopAt = 0.0;
  double nextPowerSwingAt = 0.0;
};

double nowSec() {
  using namespace std::chrono;
  return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

void sendProto(ENetPeer* peer, const std::vector<std::uint8_t>& bytes) {
  ENetPacket* p = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, p);
}

int run(int argc, char** argv) {
  std::string host = "127.0.0.1";
  std::uint16_t port = 7777;
  int count = 20;
  int secs = 30;
  std::string mapPath = "assets/maps/thornwall.bhmap";
  std::string prefix = "bot";
  std::string profile = "wander";
  bool reckless = false;  // M2 gate hires: no flasks, no shopping
  std::string visitGoal = "";

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next = [&](const char* dflt) { return i + 1 < argc ? argv[++i] : dflt; };
    if (a == "--host") host = next(host.c_str());
    else if (a == "--port") port = static_cast<std::uint16_t>(std::atoi(next("7777")));
    else if (a == "--count") count = std::atoi(next("20"));
    else if (a == "--secs") secs = std::atoi(next("30"));
    else if (a == "--map") mapPath = next(mapPath.c_str());
    else if (a == "--prefix") prefix = next(prefix.c_str());
    else if (a == "--profile") profile = next(profile.c_str());
    else if (a == "--reckless") reckless = true;
    else if (a == "--visit") visitGoal = next("");  // "x,y" anchor: path into reach
    else {
      std::fprintf(stderr,
                   "usage: bh_bots [--host H] [--port P] [--count N] [--secs S] [--map M] "
                   "[--prefix P] [--profile wander|fighter]\n");
      return 2;
    }
  }

  std::string err;
  auto map = bh::sim::loadBhmap(mapPath, &err);
  if (!map) {
    std::fprintf(stderr, "bh_bots: %s\n", err.c_str());
    return 1;
  }

  if (enet_initialize() != 0) {
    std::fprintf(stderr, "bh_bots: enet_initialize failed\n");
    return 1;
  }
  ENetHost* chost = enet_host_create(nullptr, static_cast<size_t>(count), 2, 0, 0);
  if (chost == nullptr) {
    std::fprintf(stderr, "bh_bots: enet_host_create failed\n");
    return 1;
  }

  bh::sim::Rng rng(0xB07L);
  std::vector<Bot> bots(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    Bot& b = bots[static_cast<size_t>(i)];
    char name[32];
    std::snprintf(name, sizeof name, "%s_%02d", prefix.c_str(), i);
    b.name = name;
    ENetAddress addr;
    enet_address_set_host(&addr, host.c_str());
    addr.port = port;
    b.peer = enet_host_connect(chost, &addr, 2, 0);
    if (b.peer == nullptr) {
      std::fprintf(stderr, "bh_bots: connect allocation failed for %s\n", name);
      continue;
    }
    b.peer->data = reinterpret_cast<void*>(static_cast<intptr_t>(i));
    b.nextMoveAt = nowSec() + 1.0 + rng.unit() * 4.0;
  }

  const double t0 = nowSec();
  const double tEnd = t0 + secs;
  std::uint64_t packetsRx = 0;
  std::uint64_t bytesRx = 0;

  while (nowSec() < tEnd) {
    ENetEvent ev;
    while (enet_host_service(chost, &ev, 2) > 0) {
      switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT: {
          const int i = static_cast<int>(reinterpret_cast<intptr_t>(ev.peer->data));
          bh::proto::Hello h;
          h.protoVersion = bh::proto::kProtocolVersion;
          h.username = bots[static_cast<size_t>(i)].name;
          h.password = "wander";
          sendProto(ev.peer, bh::proto::pack(h));
          break;
        }
        case ENET_EVENT_TYPE_RECEIVE: {
          ++packetsRx;
          bytesRx += ev.packet->dataLength;
          const int i = static_cast<int>(reinterpret_cast<intptr_t>(ev.peer->data));
          Bot& b = bots[static_cast<size_t>(i)];
          const auto pv = bh::proto::view(
              static_cast<std::uint8_t*>(ev.packet->data), ev.packet->dataLength);
          if (pv.ok) {
            if (pv.id == bh::proto::kIdWelcome) {
              bh::proto::Welcome w;
              if (w.deserialize(pv.body)) {
                if (b.welcomed) {  // repeated Welcome = zone transfer (T-036)
                  b.ents.clear();
                  b.attackTarget = 0;
                }
                b.welcomed = true;
                b.ownId = w.entityId;
                b.tileX = w.x / bh::sim::Walker::kUnitsPerTile;
                b.tileY = w.y / bh::sim::Walker::kUnitsPerTile;
                if (b.homeX < 0) { b.homeX = b.tileX; b.homeY = b.tileY; }
              }
            } else if (pv.id == bh::proto::kIdEntitySpawn) {
              bh::proto::EntitySpawn m;
              if (m.deserialize(pv.body)) {
                SeenEnt se;
                se.x = m.x / bh::sim::Walker::kUnitsPerTile;
                se.y = m.y / bh::sim::Walker::kUnitsPerTile;
                se.kind = m.kind;
                b.ents[m.id] = se;
                if (m.id == b.ownId) {
                  b.hp = static_cast<std::int32_t>(m.hp);
                  b.hpMax = static_cast<std::int32_t>(m.hpMax);
                }
              }
            } else if (pv.id == bh::proto::kIdEntityDelta) {
              bh::proto::EntityDelta d;
              if (d.deserialize(pv.body)) {
                const int tx = d.x / bh::sim::Walker::kUnitsPerTile;
                const int ty = d.y / bh::sim::Walker::kUnitsPerTile;
                if (d.id == b.ownId) {
                  ++b.deltasOnSelf;
                  b.tileX = tx;
                  b.tileY = ty;
                  b.hp = static_cast<std::int32_t>(d.hp);
                } else {
                  auto it = b.ents.find(d.id);
                  if (it != b.ents.end()) {
                    it->second.x = tx;
                    it->second.y = ty;
                  }
                }
              }
            } else if (pv.id == bh::proto::kIdEntityDespawn) {
              bh::proto::EntityDespawn d;
              if (d.deserialize(pv.body)) b.ents.erase(d.id);
              if (b.attackTarget == d.id) b.attackTarget = 0;
            } else if (pv.id == bh::proto::kIdCombatEvent) {
              bh::proto::CombatEvent m;
              if (m.deserialize(pv.body) && m.kind == 3) {
                if (m.attackerId == b.ownId) ++b.kills;
                if (m.targetId == b.ownId) ++b.deaths;  // T-040: authoritative source
              }
            } else if (pv.id == bh::proto::kIdOwnStats) {
              bh::proto::OwnStats m;
              if (m.deserialize(pv.body)) {
                if (m.level + 1 == b.level) ++b.levelDrops;  // XP-debt de-level
                b.level = m.level;
                b.gold = m.gold;
              }
            } else if (pv.id == bh::proto::kIdInventoryReset) {
              bh::proto::InventoryReset m;
              m.deserialize(pv.body);
              b.inv.clear();
            } else if (pv.id == bh::proto::kIdItemSlot) {
              bh::proto::ItemSlot m;
              if (m.deserialize(pv.body)) {
                b.inv[m.slot] = Bot::InvRow{m.itemId, m.qty, m.equipped != 0, m.aura};
              }
            }
          }
          enet_packet_destroy(ev.packet);
          break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
          // mark as failed; soak summary will catch it
          break;
        }
        default:
          break;
      }
    }

    // behavior
    const double t = nowSec();
    for (Bot& b : bots) {
      if (!b.welcomed || b.peer == nullptr || t < b.nextMoveAt) continue;
      if (!visitGoal.empty() && b.visited == 0) {
        int vx = b.homeX, vy = b.homeY;
        const size_t comma = visitGoal.find(',');
        if (comma != std::string::npos) {
          vx = std::atoi(visitGoal.c_str());
          vy = std::atoi(visitGoal.c_str() + comma + 1);
        }
        if (std::abs(b.tileX - vx) <= 1 && std::abs(b.tileY - vy) <= 1) {
          b.visited = 1;  // arrived: keep wandering around here
          visitGoal.clear();
        } else if (t >= b.nextWanderDecideAt) {
          bh::proto::InputPath ip;
          ip.goalX = vx;
          ip.goalY = vy;
          sendProto(b.peer, bh::proto::pack(ip));
          b.nextWanderDecideAt = t + 1.2;
        }
      }
      const bool pilgrimRites = (profile == "pilgrim");
      if (profile == "fighter" || pilgrimRites) {
        // nearest mob within 10 tiles -> chase / attack
        std::uint32_t bestId = 0;
        int bestD = 100;
        for (const auto& kv : b.ents) {
          if (kv.second.kind == 0 || bh::content::wireIsFurniture(kv.second.kind)) continue;  // players, furniture
          const int d = std::max(std::abs(kv.second.x - b.tileX),
                                 std::abs(kv.second.y - b.tileY));
          if (d < bestD) {
            bestD = d;
            bestId = kv.first;
          }
        }
        // economy (v2 grinder): at login (near vendor Marta) buy a vial; later,
        // whenever back near spawn with junk, pawn it and upgrade gear.
        if (b.nextAnvilAt == 0.0 && t > 30.0) b.nextAnvilAt = t + 1.0;
        if (!b.boughtVials && b.welcomed) {
          b.boughtVials = true;
          if (!reckless && !pilgrimRites) {
            bh::proto::BuyRequest buy;
            buy.itemId = 3001;
            buy.qty = 1;  // 30g, start gold is 50
            sendProto(b.peer, bh::proto::pack(buy));
          }
        }
        // junk count + near-town check
        int junk = 0;
        std::uint32_t pelts = 0;
        bool hasBlade = false;
        bool bladeArmed = false;
        std::uint8_t bladeAura = 0;
        std::uint8_t bladeSlot = 255;
        for (const auto& kv : b.inv) {
          if (kv.second.itemId == 4001) pelts += kv.second.qty;
          if (kv.second.itemId == 4001 || kv.second.itemId == 4002 ||
              kv.second.itemId == 4003) ++junk;
          if (kv.second.itemId == 2002 || kv.second.itemId == 2001) {
            hasBlade = true; bladeSlot = kv.first;
            bladeArmed = kv.second.equipped; bladeAura = kv.second.aura;
          }
        }
        if (pilgrimRites) {
          if (b.gold > b.dbgMaxGold) b.dbgMaxGold = b.gold;
          if (pelts > b.dbgMaxPelts) b.dbgMaxPelts = pelts;
          if (!bladeArmed) ++b.dbgNoBlade;
          else if (b.gold < 120) ++b.dbgNoGold;
          else if (pelts < 30) ++b.dbgNoPelts;
          else ++b.dbgReady;
        }
        bool anvilAdj = false;
        for (const auto& kv2 : b.ents) {
          if (kv2.second.kind == bh::content::kWireKindAnvil) {
            const int ax = std::abs(b.tileX - kv2.second.x);
            const int ay = std::abs(b.tileY - kv2.second.y);
            if (std::max(ax, ay) <= 2) { anvilAdj = true; break; }
          }
        }
        const bool nearTown = b.homeX >= 0 && std::abs(b.tileX - b.homeX) < 4 &&
                              std::abs(b.tileY - b.homeY) < 4;
        // tier-table driven rite math (T-047): parts+gold for the NEXT tier
        std::uint32_t needGold = 120, haveParts = 0, wantParts = 0;
        const auto* nextTier = bh::content::findAuraTier(
            static_cast<std::uint8_t>(bladeAura + 1));
        if (nextTier != nullptr) {
          needGold = nextTier->gold;
          wantParts = nextTier->partQty;
          for (const auto& kv : b.inv)
            if (kv.second.itemId == nextTier->partItemId) haveParts += kv.second.qty;
        }
        if (reckless) { /* no economy: the morgue is the bank */ }
        else if (nearTown && junk >= 4 &&
                 (!bladeArmed || bladeAura >= 1 || pelts >= 34 || !pilgrimRites) &&
                 t >= b.nextShopAt) {
          b.nextShopAt = t + 2.0;
          bh::proto::SellJunk sj;
          sj.unused = 0;
          sendProto(b.peer, bh::proto::pack(sj));
          ++b.shops;
        } else if (nearTown && !hasBlade && pilgrimRites && b.gold >= 110 &&
                   t >= b.nextShopAt) {
          b.nextShopAt = t + 2.0;
          bh::proto::BuyRequest buy;
          buy.itemId = 2001;  // Rusty Shank: cheap rite-metal
          buy.qty = 1;
          sendProto(b.peer, bh::proto::pack(buy));
          ++b.shops;
        } else if (nearTown && !hasBlade && !pilgrimRites && b.gold >= 260 && t >= b.nextShopAt) {
          b.nextShopAt = t + 2.0;
          bh::proto::BuyRequest buy;
          buy.itemId = 2002;  // Pit Blade
          buy.qty = 1;
          sendProto(b.peer, bh::proto::pack(buy));
          ++b.shops;
        } else if (bladeArmed && b.gold >= needGold && haveParts >= wantParts &&
                   nextTier != nullptr && pilgrimRites && !anvilAdj) {
          ++b.dbgNoAnvil;  // ready but the Widow's bench not in reach
        } else if (anvilAdj && bladeArmed && b.gold >= needGold &&
                   haveParts >= wantParts &&
                   nextTier != nullptr && pilgrimRites && t >= b.nextAnvilAt) {
          // pilgrim: pay tribute at the anvil for the first widow graft
          b.nextAnvilAt = t + 4.0;
          bh::proto::AnvilOp aop;
          aop.tier = static_cast<std::uint8_t>(bladeAura + 1);
          sendProto(b.peer, bh::proto::pack(aop));
          ++b.anvilTries;
        } else if (hasBlade && !bladeArmed && t >= b.nextShopAt) {
          b.nextShopAt = t + 3.0;
          for (const auto& kv : b.inv) {
            if (kv.first == bladeSlot) {
              bh::proto::ToggleEquip te;
              te.slot = kv.first;
              sendProto(b.peer, bh::proto::pack(te));  // equip when not worn
              break;
            }
          }
        }
        // sip when hurt and holding a vial (reckless hires never sip)
        if (!reckless && b.hpMax > 0 && b.hp * 2 < b.hpMax) {
          for (const auto& kv : b.inv) {
            if (kv.second.itemId == 3001) {
              bh::proto::UseItem u;
              u.slot = kv.first;
              sendProto(b.peer, bh::proto::pack(u));
              ++b.potionsUsed;
              break;
            }
          }
        }
        if (pilgrimRites && bladeArmed && bladeAura < 1 &&
            (pelts >= 30 ? (b.gold < 120) : (pelts >= 6 && b.gold >= 120))) {
          // half-funded: keep hunting (fall through to combat)
        } else if (pilgrimRites && bladeArmed && bladeAura < 3 &&
                   ((bladeAura < 1 && pelts >= 6 && b.gold >= 120) ||
                    (nextTier != nullptr && haveParts >= wantParts &&
                     b.gold >= needGold)) &&
                   !nearTown) {
          bh::proto::InputPath ip;
          ip.goalX = b.homeX;
          ip.goalY = b.homeY;
          sendProto(b.peer, bh::proto::pack(ip));
          b.nextMoveAt = t + 1.2;
          continue;  // walk home for the rite
        }
        if (bestId != 0 && bestD <= 12) {
          if (bestD > 1) {
            b.attackTarget = 0;
            const SeenEnt& se = b.ents[bestId];
            bh::proto::InputPath ip;
            ip.goalX = se.x;
            ip.goalY = se.y;
            sendProto(b.peer, bh::proto::pack(ip));
            b.nextMoveAt = t + 0.5;
          } else {
            if (b.attackTarget != bestId || t >= b.nextAttackAt) {
              bh::proto::AttackRequest ar;
              ar.targetId = bestId;
              sendProto(b.peer, bh::proto::pack(ar));
              b.attackTarget = bestId;
              b.nextAttackAt = t + 0.9;  /* ~server 16t cd; old 1.5s lag inflated TTK measurements */
            }
            if (t >= b.nextPowerSwingAt) {  // Power Swing when off cooldown-ish
              b.nextPowerSwingAt = t + 2.5;
              bh::proto::SkillUse su;
              su.skill = 1;
              su.targetId = bestId;
              sendProto(b.peer, bh::proto::pack(su));
              ++b.swings;
            }
            b.nextMoveAt = t + 0.3;  // reconsider quickly while in melee
          }
          continue;
        }
        // nothing near: wander toward a random mob-area direction anyway
      } else if (profile != "wander" && profile != "pilgrim") {
        std::fprintf(stderr, "bh_bots: unknown profile '%s'\n", profile.c_str());
        return 2;
      }
      const int rx = b.tileX + static_cast<int>(rng.range(-10, 10));
      const int ry = b.tileY + static_cast<int>(rng.range(-10, 10));
      if (map->inBounds(rx, ry) && !map->isBlocked(rx, ry)) {
        bh::proto::InputPath ip;
        ip.goalX = rx;
        ip.goalY = ry;
        sendProto(b.peer, bh::proto::pack(ip));
        if (rng.chance(0.08)) {
          bh::proto::InputStep st;
          st.dx = static_cast<std::int8_t>(rng.range(-1, 1));
          st.dy = static_cast<std::int8_t>(rng.range(-1, 1));
          sendProto(b.peer, bh::proto::pack(st));
        }
        if (rng.chance(0.03)) {
          bh::proto::ChatSend cs;
          cs.channel = 0;
          cs.text = rng.chance(0.5) ? "fresh meat." : "the fog is thick tonight.";
          sendProto(b.peer, bh::proto::pack(cs));
        }
      }
      b.nextMoveAt = t + 1.5 + rng.unit() * 4.0;
    }
  }

  int welcomed = 0;
  int moved = 0;
  std::uint64_t minDeltas = UINT64_MAX;
  std::uint64_t kills = 0;
  std::uint64_t pots = 0;
  std::uint64_t swings = 0;
  std::uint64_t deaths = 0;
  std::uint64_t shops = 0;
  std::uint64_t levelDrops = 0;
  std::uint64_t anvilTries = 0;
  std::uint64_t dbgNoBlade = 0, dbgNoGold = 0, dbgNoPelts = 0, dbgNoAnvil = 0,
                dbgReady = 0;
  std::uint32_t mxGold = 0, mxPelts = 0;
  int maxLevel = 1;
  for (const Bot& b : bots) {
    if (b.welcomed) ++welcomed;
    if (b.deltasOnSelf > 0) ++moved;
    minDeltas = std::min(minDeltas, b.deltasOnSelf);
    kills += b.kills;
    pots += b.potionsUsed;
    swings += b.swings;
    deaths += b.deaths;
    shops += b.shops;
    levelDrops += b.levelDrops;
    anvilTries += b.anvilTries;
    dbgNoBlade += b.dbgNoBlade; dbgNoGold += b.dbgNoGold;
    dbgNoPelts += b.dbgNoPelts; dbgNoAnvil += b.dbgNoAnvil; dbgReady += b.dbgReady;
    if (b.dbgMaxGold > mxGold) mxGold = b.dbgMaxGold;
    if (b.dbgMaxPelts > mxPelts) mxPelts = b.dbgMaxPelts;
    maxLevel = std::max(maxLevel, b.level);
  }
  std::printf("[bots] SUMMARY welcomed=%d/%d moved=%d/%d minDeltas=%" PRIu64
              " kills=%" PRIu64 " pots=%" PRIu64 " swings=%" PRIu64" deaths=%" PRIu64 " shops=%" PRIu64 " anvilTries=%" PRIu64 " levelDrops=%" PRIu64
              " maxLevel=%d pkts=%" PRIu64 " bytes=%" PRIu64
              " gates b/g/p/a/r=%" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 " mGold=%u mPelts=%u\n",
              welcomed, count, moved, count, minDeltas == UINT64_MAX ? 0 : minDeltas,
              kills, pots, swings, deaths, shops, anvilTries, levelDrops, maxLevel,
              packetsRx, bytesRx, dbgNoBlade, dbgNoGold, dbgNoPelts, dbgNoAnvil,
              dbgReady, mxGold, mxPelts);

  for (Bot& b : bots) {
    if (b.peer != nullptr) enet_peer_disconnect_now(b.peer, 0);
  }
  enet_host_destroy(chost);
  enet_deinitialize();
  const bool ok = welcomed == count && moved == count && minDeltas > 0 &&
                  (profile != "fighter" || kills > 0);
  std::printf("[bots] %s\n", ok ? "OK" : "FAIL");
  return ok ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) { return run(argc, argv); }
