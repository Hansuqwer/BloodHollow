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
#include "content/kits.h"
#include "content/mobs.h"
#include "content/wirekind.h"
#include "sim/bhmap.h"
#include "sim/rng.h"
#include "sim/walker.h"

namespace {

struct SeenEnt {
  int x = 0, y = 0;
  std::uint8_t kind = 0;  // 0 = player
  int level = 0;          // mob level (EntitySpawn); 0 = unknown
};

// Route v5: whether a mob can pile onto us at all.  wireKind is the 1-based
// kMobs index (server/src/world.cpp:189), so the passive starters -- Marsh Rat
// has aggroRadius 0 -- read false here and stay diveable at L1; everything
// that actually aggroes counts toward the pack math.
bool entAggressive(std::uint8_t kind) {
  if (kind == 0 || kind > bh::content::kMobKindCount) return false;
  return bh::content::kMobs[kind - 1].aggroRadius > 0;
}

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
  std::unordered_map<int, std::uint64_t> deathByKillerLevel{};  // diagnostic: killer mob level -> deaths
  int lastDeathX = -1, lastDeathY = -1;
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
  // campaign profile (T-034/M2b): player-paced L1->8 cross-zone route
  std::uint32_t mapId = 1;
  double campaignT0 = -1.0;
  int announcedLevel = 0;
  bool campaignDone = false;
  double restUntil = 0.0, nextRestAt = 15.0;  // human pacing: fight 60-90s, idle 6-11s
  int campX = -1, campY = -1;                  // current hunting-camp waypoint
  bool campIsPortal = false;  // camp sits on a portal rect: land EXACTLY, no mill
  bool retreating = false;    // campaign: broke off at low hp, sip + return
  bool regearHome = false;    // T-034d: walking home for the next gear tier
  std::uint64_t regearTrips = 0;
  std::uint8_t partyTries = 0;    // S13: formation attempts (10 s cadence, cap 4)
  double nextPartyTryAt = 0.0;
  // T-054/55 choir: roster mirror + kit state (classId/mp from OwnStats)
  struct RosterRow { std::string name; std::uint32_t hp = 1, hpMax = 1; int level = 1; std::uint16_t zoneId = 0; };
  std::unordered_map<std::uint32_t, RosterRow> party{};
  std::uint32_t partyLeaderId = 0;
  int kitClass = 1;           // 1 ravager / 2 gravecaller / 3 cultist
  std::uint32_t mp = 0, mpMax = 30;
  bool kitSworn = false;      // /kit cultist sent
  double nextMendAt = 0.0, nextBlessAt = 0.0;
  // T-054b choir-bot v2: kit-v2 channel pacing (ch6 Chorus / ch7 Mass Mend /
  // ch8 Haste) + per-channel cast counters for the SUMMARY line
  double nextChorusAt = 0.0, nextMassMendAt = 0.0, nextHasteAt = 0.0;
  std::uint64_t chorusCasts = 0, massCasts = 0, hasteCasts = 0;
  std::uint64_t blessCasts = 0, mendCasts = 0, mendNoSee = 0, mendHurtCnt = 0;
  std::uint64_t rcvReset = 0, rcvMember = 0; std::uint32_t lastResetPid = 0;
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
  int targetLevel = 8;  // campaign profile: stop condition (OwnStats level)
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
    else if (a == "--target-level") targetLevel = std::atoi(next("8"));
    else {
      std::fprintf(stderr,
                   "usage: bh_bots [--host H] [--port P] [--count N] [--secs S] [--map M] "
                   "[--prefix P] [--profile wander|fighter|pilgrim|campaign] [--target-level N]\n");
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
  bool campaignAllDone = false;
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
                b.mapId = w.mapId;
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
                se.level = m.level;
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
                if (m.targetId == b.ownId) {  // T-040: authoritative source
                  ++b.deaths;
                  int kl = 0;
                  const auto ki = b.ents.find(m.attackerId);
                  if (ki != b.ents.end()) kl = ki->second.level;
                  ++b.deathByKillerLevel[kl];
                  b.lastDeathX = b.tileX;
                  b.lastDeathY = b.tileY;
                }
              }
            } else if (pv.id == bh::proto::kIdOwnStats) {
              bh::proto::OwnStats m;
              if (m.deserialize(pv.body)) {
                if (m.level + 1 == b.level) ++b.levelDrops;  // XP-debt de-level
                b.level = m.level;
                if (profile == "campaign" && b.level > b.announcedLevel &&
                    b.campaignT0 >= 0.0) {
                  b.announcedLevel = b.level;
                  std::printf("[campaign] %s reached L%d at t=%.1fs (map %u)\n",
                              b.name.c_str(), b.level, nowSec() - b.campaignT0,
                              b.mapId);
                  std::fflush(stdout);
                  if (b.level >= targetLevel && !b.campaignDone) {
                    b.campaignDone = true;
                    std::printf("[campaign] %s TARGET L%d DONE in %.1fs\n",
                                b.name.c_str(), b.level, nowSec() - b.campaignT0);
                    std::fflush(stdout);
                  }
                }
                b.gold = m.gold;
                b.kitClass = m.classId;
                b.mp = m.mp;
                b.mpMax = m.mpMax;
              }
            } else if (pv.id == bh::proto::kIdPartyReset) {
              bh::proto::PartyReset m;
              if (m.deserialize(pv.body)) {
                b.partyLeaderId = m.leaderId;
                ++b.rcvReset;
                b.lastResetPid = m.partyId;
                b.party.clear();
              }
            } else if (pv.id == bh::proto::kIdPartyMember) {
              bh::proto::PartyMember m;
              if (m.deserialize(pv.body)) {
                ++b.rcvMember;
                b.party[m.entityId] =
                    Bot::RosterRow{m.name, m.hp, m.hpMax == 0 ? 1u : m.hpMax,
                                   m.level, m.zoneId};
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
    size_t bi = 0;
    for (Bot& b : bots) {
      const size_t botIdx = bi++;
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
      const bool campaign = (profile == "campaign");
      if (campaign) {
        if (b.campaignT0 < 0.0) {
          b.campaignT0 = t;
          // S13 auto-party: sibling pair-up so party-shared legs gate M2b-final.
          // Even-indexed bot invites its successor, odd accepts (server
          // resolves the name -> entityId before journaling; replay-exact).
          (void)0;
          b.nextRestAt = t + 60.0 + rng.range(0, 30);  // first break a minute in
        }
        // player-paced rhythm: ~60-90s engaged, then a 6-11s door-stop
        if (t >= b.nextRestAt) {
          b.nextRestAt = t + 60.0 + rng.range(0, 30);
          b.restUntil = t + 6.0 + rng.range(0, 5);
        }
        // hunting-camp waypoint for own level & zone (content coords, in tiles)
        // Route v5 (killer-histogram informed): v4 parked every level >= 3 on
        // (55,19), the CENTRE of the ghouls_east rect x[52,58] y[17,22] at
        // maxAlive 8 -- lastDeath clustered on (55,18)/(55,20) and killerByLvl
        // was L3-dominated, so the wall was pack DENSITY, not mob level.  Stand
        // on the east road 3 tiles north of the rect instead: ghoul aggro is 6
        // (7 at night) so only the north row pulls, and the leash (14 from
        // anchor) lets a retreat home break the fight cleanly.
        b.campIsPortal = false;
        if (b.mapId == 3) {
          b.campX = 0; b.campY = 17; b.campIsPortal = true;   // crypt: back up
        } else if (b.mapId == 2) {
          // gate evidence gathered in early legs; fields grind is pack-country,
          // so any later leg resumes here only to walk straight home
          b.campX = 0; b.campY = 14; b.campIsPortal = true;  // west gate home
        } else {
          switch (b.level) {
            case 1:  b.campX = 46; b.campY = 13; break;  // rats_east (passive)
            case 2:  b.campX = 15; b.campY = 6;  break;  // bats_cryptyard
            default: b.campX = 55; b.campY = 14; break;  // ghouls_east NORTH EDGE
          }
          if (b.level >= 7) {
            // T-068 opened the L7->L8 step-up: the L11 gravecaller barricade
            // moved off the bridge approach ((6,18)->(1,43) in
            // tools/mapgen/make_thornwall.py), so the south road / bridge
            // crossing is clean and gnolls_pits is reachable.  Camp the
            // gnolls NORTH edge (rect y[42,45]): gnolls (wander 8, aggro 7)
            // pull straight onto the camp, road ghouls (L3) are fodder via
            // the defend branch, and retreat-home now crosses the river
            // north instead of into worse content.  widow_glade (L9) shares
            // the bank and may pull -- pack cap + retreat handle it; this
            // camp is the L8 probe.
            b.campX = 53; b.campY = 41;  // gnolls_pits NORTH EDGE
          }
        }
        // S13/14 party formation, race-free: even bot invites ONLY once the
        // sibling is welcomed in-world (bot.name is pre-seeded and useless as
        // an online signal — two S14 legs died to that race); retries every
        // 10 s (invites expire at 10 s) with 4 attempts. Odd bot accepts on a
        // heartbeat while roster-empty; same budget. Accept can fire before
        // any invite exists — harmless chat blip at worst.
        // retry while MY roster is missing the sibling (a 1-member "party" is
        // the leader alone — invites are only done when roster size confirms)
        const bool unformed = b.party.size() < 2;
        if (bots.size() >= 2 && unformed) {
          const size_t sib = botIdx % 2 == 0 ? botIdx + 1 : botIdx - 1;
          if (sib < bots.size() && bots[sib].welcomed && b.partyTries < 4 &&
              t >= b.nextPartyTryAt) {
            b.nextPartyTryAt = t + 10.0;
            ++b.partyTries;
            bh::proto::ChatSend cs;
            cs.channel = 0;
            cs.text = botIdx % 2 == 0 ? ("/invite " + bots[sib].name) : "/accept";
            sendProto(b.peer, bh::proto::pack(cs));
          }
        }
        // oath first (T-053): odd bot swears Cultist 3 s after campaign start
        if (!b.kitSworn && botIdx % 2 == 1 && t >= b.campaignT0 + 3.0) {
          b.kitSworn = true;
          bh::proto::ChatSend cs;
          cs.channel = 0;
          cs.text = "/kit cultist";
          sendProto(b.peer, bh::proto::pack(cs));
        }
        // ---- T-055 choir behavior (Cultist kit bots) --------------------
        // priority: mend any <60% party member (ranged 6), bless the leader,
        // else glue to the leader and let the normal fighter flow swing.
        if (b.kitClass == 3 && !b.party.empty()) {
          auto cheb = [&](int ax, int ay, int bx, int by) {
            const int dx = ax > bx ? ax - bx : bx - ax;
            const int dy = ay > by ? ay - by : by - ay;
            return dx > dy ? dx : dy;
          };
          std::uint32_t hurtId = 0;
          int hurtDist = 99;
          int hurtSeen = 0;
          for (const auto& [mid, row] : b.party) {
            if (row.hp * 5 < row.hpMax * 3) ++hurtSeen;  // sub-60% roster rows
            if (row.hp * 5 >= row.hpMax * 3) continue;
            const auto ei = b.ents.find(mid);
            if (ei == b.ents.end()) continue;
            const int d = cheb(b.tileX, b.tileY, ei->second.x, ei->second.y);
            if (d < hurtDist) { hurtDist = d; hurtId = mid; }
          }
          if (hurtSeen > 0 && hurtId == 0) ++b.mendNoSee;
          b.mendHurtCnt += static_cast<std::uint64_t>(hurtSeen);
          if (hurtSeen > 0 && hurtId == 0) { ++b.mendNoSee; b.mendHurtCnt += 0; }
          if (hurtSeen > 0) b.mendHurtCnt += static_cast<std::uint64_t>(hurtSeen);
          const auto lead = b.party.find(b.partyLeaderId);
          const auto leadE = lead != b.party.end() ? b.ents.find(lead->first)
                                                   : b.ents.end();
          const int leadDist = leadE != b.ents.end()
              ? cheb(b.tileX, b.tileY, leadE->second.x, leadE->second.y) : 99;
          // bless the leader on duty (5 min buff; refresh every ~4 min)
          if (b.level >= 3 && leadE != b.ents.end() && leadDist <= 6 &&
              t >= b.nextBlessAt && b.mp >= 15) {
            bh::proto::SkillUse su;
            su.skill = 3;
            su.targetId = b.partyLeaderId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextBlessAt = t + 240.0;
            ++b.blessCasts;
          }
          // T-054b choir-bot v2: kit-v2 channels. Level gates come from the
          // shared chUnlock table — the server enforces them too, so an early
          // cast is a quiet no-op, never a gamble.
          // ch6 Chorus (Choir L9): the S13 formation above supplies the real
          // party; >=2 voices inside the 6-tile sweep or the verse is a hum.
          // The song lasts 2 min and refreshes (never stacks): recast ~2.5 min.
          if (b.level >= bh::content::kitSkillUnlock(b.kitClass, 6) &&
              b.party.size() >= 2 && b.mp >= 14 && t >= b.nextChorusAt) {
            std::uint32_t voices = 1;  // the caster sings too
            for (const auto& [mid, row] : b.party) {
              const auto ei = b.ents.find(mid);
              if (ei != b.ents.end() &&
                  cheb(b.tileX, b.tileY, ei->second.x, ei->second.y) <= 6)
                ++voices;
            }
            if (voices >= 2) {
              bh::proto::SkillUse su;
              su.skill = 6;
              su.targetId = b.ownId;
              sendProto(b.peer, bh::proto::pack(su));
              b.nextChorusAt = t + 150.0;
              ++b.chorusCasts;
            }
          }
          // ch7 Mass Mend (Choir L12): two+ hurt voices in range make the 18
          // mp beat single-target mends; full-hp members are skipped
          // server-side (the half-strength stitch never spills over).
          bool massMended = false;
          if (b.level >= bh::content::kitSkillUnlock(b.kitClass, 7) &&
              b.mp >= 18 && hurtSeen >= 2 && t >= b.nextMassMendAt) {
            bh::proto::SkillUse su;
            su.skill = 7;
            su.targetId = b.ownId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextMassMendAt = t + 3.5;  // server CD 3 s
            ++b.massCasts;
            massMended = true;
          }
          // ch8 Haste (Gravecaller L10 / Choir L11): self rotation gear while
          // something stands in swing range (60 s of fast steel, ~70 s refresh).
          bool mobAdj = false;
          for (const auto& kv : b.ents) {
            if (kv.second.kind == 0 ||
                bh::content::wireIsFurniture(kv.second.kind))
              continue;
            if (cheb(b.tileX, b.tileY, kv.second.x, kv.second.y) <= 2) {
              mobAdj = true;
              break;
            }
          }
          if (b.level >= bh::content::kitSkillUnlock(b.kitClass, 8) &&
              b.mp >= 10 && mobAdj && t >= b.nextHasteAt) {
            bh::proto::SkillUse su;
            su.skill = 8;
            su.targetId = b.ownId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextHasteAt = t + 70.0;
            ++b.hasteCasts;
          }
          if (hurtId != 0 && !massMended) {
            if (hurtDist <= 6 && t >= b.nextMendAt && b.mp >= 8) {
              bh::proto::SkillUse su;
              su.skill = 2;
              su.targetId = hurtId;
              sendProto(b.peer, bh::proto::pack(su));
              b.nextMendAt = t + 1.5;  // server CD is 1.25 s; small slop
              ++b.swings;              // book as a cast for telemetry
              ++b.mendCasts;
              continue;
            }
            // chase the wounded
            const auto ei = b.ents.find(hurtId);
            if (ei != b.ents.end()) {
              bh::proto::InputPath ip;
              ip.goalX = ei->second.x;
              ip.goalY = ei->second.y;
              sendProto(b.peer, bh::proto::pack(ip));
              b.nextMoveAt = t + 0.6;
              continue;
            }
          } else if (leadE != b.ents.end() && leadDist > 6) {
            bh::proto::InputPath ip;
            ip.goalX = leadE->second.x;
            ip.goalY = leadE->second.y;
            sendProto(b.peer, bh::proto::pack(ip));
            b.nextMoveAt = t + 0.8;
            continue;
          }
        }
        if (b.campaignDone) continue;  // target reached: idle out the clock
      }
      if (profile == "fighter" || pilgrimRites || campaign) {
        // nearest mob within 10 tiles -> chase / attack
        std::uint32_t bestId = 0;
        int bestD = 100;
        int swarmOnUs = 0;  // Route v5: aggressive mobs within 2 tiles of US --
                            // the dive-death gauge that drives the early break
        for (const auto& kv : b.ents) {
          if (kv.second.kind == 0 || bh::content::wireIsFurniture(kv.second.kind)) continue;  // players, furniture
          const int d = std::max(std::abs(kv.second.x - b.tileX),
                                 std::abs(kv.second.y - b.tileY));
          if (campaign && d <= 2 && entAggressive(kv.second.kind)) ++swarmOnUs;
          if (campaign && kv.second.level > b.level + 2 && d > 1) continue;  // no walls: strike back only at point-blank
          if (campaign && t < b.restUntil && d > 1) continue;  // resting: fight back only
          if (campaign && b.campIsPortal) continue;  // portal leg: hands off the sword —
              // any AttackRequest path-clears; bat harassment at the chapel door
              // otherwise livelocks the crossing (observed smoke v2)
          // Route v5c: "no new pulls" means no new pulls -- a fleeing bot must
          // still swing at whatever is already on it (see the defend block in
          // the retreat branch below).  Skipping point-blank here too left
          // bestId == 0 for the whole disengage and zeroed XP gain.
          if (campaign && b.retreating && d > 1) continue;
          if (campaign && d > 1) {                     // pull singles: skip packed targets
            int pack = 0;
            for (const auto& kv2 : b.ents) {
              if (kv2.first == kv.first || kv2.second.kind == 0 ||
                  bh::content::wireIsFurniture(kv2.second.kind)) continue;
              if (!entAggressive(kv2.second.kind)) continue;  // passive: cannot pile on
              if (std::max(std::abs(kv2.second.x - kv.second.x),
                           std::abs(kv2.second.y - kv.second.y)) <= 3) ++pack;
            }
            // Route v5, L3+ only (the L1/L2 starter camps are passive-rat and
            // bat-swarm content the route is MEANT to dive): the pack cap
            // ignores LEVEL.  v4 exempted own-2 "era fodder" from the singles
            // rule, which waved an 8-ghoul cluster through as free XP at L5
            // (3 >= 5-1 is false) -- that exemption is what put lastDeath on
            // the waypoint.  `pack` counts OTHERS, so >= 2 means 3+ clustered.
            if (b.level >= 3 && pack >= 2) continue;
            if (pack > 0 && kv.second.level >= b.level - 1) continue;
          }
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
        bool hasArmor = false, armorWorn = false;
        std::uint8_t armorSlot = 255;
        for (const auto& kv : b.inv) {
          if (kv.second.itemId == 4001) pelts += kv.second.qty;
          if (kv.second.itemId == 4001 || kv.second.itemId == 4002 ||
              kv.second.itemId == 4003) ++junk;
          if (kv.second.itemId == 2002 || kv.second.itemId == 2001) {
            hasBlade = true; bladeSlot = kv.first;
            bladeArmed = kv.second.equipped; bladeAura = kv.second.aura;
          }
          if (kv.second.itemId == 2101 || kv.second.itemId == 2102) {
            hasArmor = true; armorSlot = kv.first; armorWorn = kv.second.equipped;
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
        const bool nearTown = b.mapId == 1 && b.homeX >= 0 &&  // town = map 1 only
                              std::abs(b.tileX - b.homeX) < 4 &&
                              std::abs(b.tileY - b.homeY) < 4;
        if (campaign && nearTown && hasBlade && bladeArmed && t >= b.nextShopAt) {
          int vials = 0;
          for (const auto& kv : b.inv)
            if (kv.second.itemId == 3001) vials += kv.second.qty;
          if (vials < 4 && b.gold >= 90) {  // keep a 3-deep flask belt for packs
            b.nextShopAt = t + 2.0;
            bh::proto::BuyRequest buy;
            buy.itemId = 3001;
            buy.qty = 2;
            sendProto(b.peer, bh::proto::pack(buy));
            ++b.shops;
          }
        }
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
        } else if (campaign && bladeArmed && hasArmor && !armorWorn && t >= b.nextShopAt) {
          b.nextShopAt = t + 2.0;
          bh::proto::ToggleEquip te;
          te.slot = armorSlot;
          sendProto(b.peer, bh::proto::pack(te));  // equip anywhere: the re-gear
          // trip buys armor but the bot leaves town before the nearTown-gated
          // branch fires — blade-only bots get shredded by ghoul/hound packs
        } else if (campaign && nearTown && bladeArmed && !hasArmor && b.gold >= 120 && t >= b.nextShopAt) {
          b.nextShopAt = t + 2.0;
          bh::proto::BuyRequest buy;
          buy.itemId = 2101;  // Hide Armor: the 120g life insurance
          buy.qty = 1;
          sendProto(b.peer, bh::proto::pack(buy));
          ++b.shops;
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
        // choir-bot v2 potion priority: baseline band <50%, but with a mob in
        // swing range or a live retreat the band raises — flask before fangs.
        const bool threatAdj = bestId != 0 && bestD <= 2;
        const int sipPct = (campaign && (b.retreating || threatAdj)) ? 65 : 50;
        if (!reckless && b.hpMax > 0 && b.hp * 100 < b.hpMax * sipPct) {
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
        if (campaign) {
          const bool hurt = b.hpMax > 0 && b.hp * 100 < b.hpMax * 50;
          const bool critical = b.hpMax > 0 && b.hp * 100 < b.hpMax * 35;
          if (b.level >= 3 && swarmOnUs >= 5 && !b.retreating) {
            b.retreating = true;  // Route v5: 5+ aggressive on us is a lost
                                  // trade at ANY hp -- break before the red.
                                  // Threshold is high on purpose: at 3 the bot
                                  // disengaged from every normal ghoul trade at
                                  // the north edge and never banked XP.
                                  // T-077 round 2 tried 4: 92 deaths vs 32
                                  // (stuck L3, never out-levels the pack) —
                                  // reverted to v5c 5.
          } else if (hurt && !b.retreating && bestId != 0 && bestD <= 2) {
            b.retreating = true;
          } else if (critical && !b.retreating && bestId != 0 && bestD <= 6) {
            b.retreating = true;  // v2 band: sub-35% with a threat near — break
          } else if (b.retreating &&
                     b.hp * 100 >= b.hpMax * (nearTown ? 75 : 85) &&
                     (nearTown || bestId == 0 || bestD > 8)) {
            // Route v5: sticky until healed AND unpursued.  Clearing on hp
            // alone let the bot pivot mid-flight and re-aggro the same pack.
            // nearTown drops the bar to 75% -- the flask belt and the choir
            // mend finish the job on the walk back out.
            b.retreating = false;
          }
          if (b.retreating) {
            // Route v5c: defend at point-blank while disengaging.  This block
            // `continue`s before the attack block below, so a fleeing bot never
            // swung -- it walked home as a free punching bag for the pursuers.
            if (bestId != 0 && bestD <= 1 && t >= b.nextAttackAt) {
              bh::proto::AttackRequest ar;
              ar.targetId = bestId;
              sendProto(b.peer, bh::proto::pack(ar));
              b.attackTarget = bestId;
              b.nextAttackAt = t + 0.9;
            }
            if (b.mapId == 1 && b.homeX >= 0 && !nearTown) {
              // Route v5: walk HOME, not away along the threat axis.  The v4
              // +-10 flee west-walked bots off (55,19) down the south road
              // x[14,15] y[17,33], which passes 3 tiles from the L11
              // gravecaller_barricade (6,18) and on into hounds_marsh (10,40)
              // -- hence the L11/L5 kills at lastDeath (18,30)/(24,46), far
              // from camp.  Home (~32,16) is outside every aggro radius, and
              // the ghoul leash (14 from anchor) drops the chase en route.
              bh::proto::InputPath ip;
              ip.goalX = b.homeX;
              ip.goalY = b.homeY;
              sendProto(b.peer, bh::proto::pack(ip));
            } else if (!nearTown) {
              // maps 2/3 (or no home anchor yet): keep the old threat-axis flee
              const SeenEnt* threat = bestId != 0 ? &b.ents[bestId] : nullptr;
              int fx = b.tileX + (threat ? (b.tileX - threat->x >= 0 ? 10 : -10) : 8);
              int fy = b.tileY + (threat ? (b.tileY - threat->y >= 0 ? 10 : -10) : 8);
              if (map->inBounds(fx, fy) && !map->isBlocked(fx, fy)) {
                bh::proto::InputPath ip;
                ip.goalX = fx;
                ip.goalY = fy;
                sendProto(b.peer, bh::proto::pack(ip));
              }
            }
            // nearTown: stand fast; the economy + choir blocks sip and mend
            b.nextMoveAt = t + 0.8;
            continue;
          }
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
        // nothing near: campaign walks the level route instead of milling
        if (campaign) {
          // T-034d re-gear trip: walk home when the next gear tier is
          // affordable, instead of hitching shopping to death-respawn. The
          // economy block above does the actual buy/equip once nearTown.
          const bool wantGear = (!hasBlade && b.gold >= 260) ||
                                (bladeArmed && !hasArmor && b.gold >= 120);
          if (wantGear && b.mapId == 1 && b.homeX >= 0) {
            if (!b.regearHome) {
              b.regearHome = true;
              ++b.regearTrips;
            }
            if (!nearTown) {
              bh::proto::InputPath ip;
              ip.goalX = b.homeX;
              ip.goalY = b.homeY;
              sendProto(b.peer, bh::proto::pack(ip));
            }
            b.nextMoveAt = t + 1.2;
            continue;
          }
          b.regearHome = false;
          if (b.campX >= 0 && b.campIsPortal) {
            // portals: the fire check needs the walker SETTLED on the rect;
            // keep re-pathing to the exact tile (the level filter above keeps
            // over-level mobs near the hatch from detouring us into a fight)
            if (b.restUntil <= t) {
              bh::proto::InputPath ip;
              ip.goalX = b.campX;
              ip.goalY = b.campY;
              sendProto(b.peer, bh::proto::pack(ip));
              b.nextMoveAt = t + 1.2;
            }
            continue;
          }
          if (b.campX >= 0 && b.restUntil <= t) {
            const int ddx = std::abs(b.tileX - b.campX);
            const int ddy = std::abs(b.tileY - b.campY);
            if (ddx > 2 || ddy > 2) {
              bh::proto::InputPath ip;
              ip.goalX = b.campX;
              ip.goalY = b.campY;
              sendProto(b.peer, bh::proto::pack(ip));
            } else {
              // at camp: short mill so respawned mobs drift into reach
              bh::proto::InputPath ip;
              ip.goalX = b.campX + static_cast<int>(rng.unit() * 6) - 3;
              ip.goalY = b.campY + static_cast<int>(rng.unit() * 6) - 3;
              sendProto(b.peer, bh::proto::pack(ip));
            }
            b.nextMoveAt = t + 1.2;
          }
          continue;
        }
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
    if (profile == "campaign" && !bots.empty() && !bots[0].name.empty()) {
      bool all = true;
      for (const Bot& cc : bots) all = all && cc.welcomed && cc.campaignDone;
      if (all) { campaignAllDone = true; break; }
    }
  }

  if (campaignAllDone)
    std::printf("[campaign] all bots reached target level\n");
  int welcomed = 0;
  int moved = 0;
  std::uint64_t minDeltas = UINT64_MAX;
  std::uint64_t kills = 0;
  std::uint64_t pots = 0;
  std::uint64_t swings = 0;
  std::uint64_t deaths = 0;
  std::uint64_t shops = 0;
  std::uint64_t levelDrops = 0;
  std::uint64_t regear = 0;
  std::uint64_t anvilTries = 0;
  std::uint64_t dbgNoBlade = 0, dbgNoGold = 0, dbgNoPelts = 0, dbgNoAnvil = 0,
                dbgReady = 0;
  std::uint64_t blessCasts = 0, mendCasts = 0, mendNoSee = 0, mendHurtCnt = 0;
  std::uint64_t chorusCasts = 0, massCasts = 0, hasteCasts = 0;
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
    regear += b.regearTrips;
    anvilTries += b.anvilTries;
    blessCasts += b.blessCasts; mendCasts += b.mendCasts;
    mendNoSee += b.mendNoSee; mendHurtCnt += b.mendHurtCnt;
    chorusCasts += b.chorusCasts; massCasts += b.massCasts;
    hasteCasts += b.hasteCasts;
    dbgNoBlade += b.dbgNoBlade; dbgNoGold += b.dbgNoGold;
    dbgNoPelts += b.dbgNoPelts; dbgNoAnvil += b.dbgNoAnvil; dbgReady += b.dbgReady;
    if (b.dbgMaxGold > mxGold) mxGold = b.dbgMaxGold;
    if (b.dbgMaxPelts > mxPelts) mxPelts = b.dbgMaxPelts;
    maxLevel = std::max(maxLevel, b.level);
  }
  std::printf("[bots] SUMMARY welcomed=%d/%d moved=%d/%d minDeltas=%" PRIu64
              " kills=%" PRIu64 " pots=%" PRIu64 " swings=%" PRIu64" deaths=%" PRIu64 " shops=%" PRIu64 " anvilTries=%" PRIu64 " levelDrops=%" PRIu64
              " regear=%" PRIu64 " maxLevel=%d pkts=%" PRIu64 " bytes=%" PRIu64
              " gates b/g/p/a/r=%" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 " mGold=%u mPelts=%u bless=%" PRIu64 " mend=%" PRIu64 " noSee=%" PRIu64 " hurt=%" PRIu64
              " choir c6=%" PRIu64 " c7=%" PRIu64 " c8=%" PRIu64 "\n",
              welcomed, count, moved, count, minDeltas == UINT64_MAX ? 0 : minDeltas,
              kills, pots, swings, deaths, shops, anvilTries, levelDrops, regear,
              maxLevel,
              packetsRx, bytesRx, dbgNoBlade, dbgNoGold, dbgNoPelts, dbgNoAnvil,
              dbgReady, mxGold, mxPelts, blessCasts, mendCasts, mendNoSee,
              mendHurtCnt, chorusCasts, massCasts, hasteCasts);

  for (Bot& b : bots) {  // T-055 probe: per-bot kit/roster truth
    std::printf("[bots] %-12s kit=%d lvl=%d hp=%d/%d party=%zu deaths=%llu rxR=%llu rxM=%llu lastPid=%u casts c6=%llu c7=%llu c8=%llu lastDeath=(%d,%d) killerByLvl=",
                b.name.c_str(), b.kitClass, b.level, static_cast<int>(b.hp),
                static_cast<int>(b.hpMax), b.party.size(),
                static_cast<unsigned long long>(b.deaths),
                static_cast<unsigned long long>(b.rcvReset),
                static_cast<unsigned long long>(b.rcvMember), b.lastResetPid,
                static_cast<unsigned long long>(b.chorusCasts),
                static_cast<unsigned long long>(b.massCasts),
                static_cast<unsigned long long>(b.hasteCasts),
                b.lastDeathX, b.lastDeathY);
    {
      bool first = true;
      for (const auto& kv : b.deathByKillerLevel) {
        std::printf("%sL%d:%llu", first ? "" : " ", kv.first,
                    static_cast<unsigned long long>(kv.second));
        first = false;
      }
    }
    std::printf("\n");
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
