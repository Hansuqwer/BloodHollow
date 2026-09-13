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
  std::uint32_t hp = 0;   // T-118 r7: last seen hp (focus-fire targeting)
};

// Route v5: whether a mob can pile onto us at all.  wireKind is the 1-based
// kMobs index (server/src/world.cpp:189), so the passive starters -- Marsh Rat
// has aggroRadius 0 -- read false here and stay diveable at L1; everything
// that actually aggroes counts toward the pack math.
bool entAggressive(std::uint8_t kind) {
  if (kind == 0 || kind > bh::content::kMobKindCount) return false;
  return bh::content::kMobs[kind - 1].aggroRadius > 0;
}

// T-118 r8: the front runner of the gauntlet stack. The r7 leg's 24 deaths
// were five independent brains trading the crypt's packs one at a time —
// each bot marched its own route node, so the barrow ring (Cantor Vex +
// pulpit + sexton seat) was met two deep and lost. Deterministic election,
// identical on every bot: among party members alive and in MY view (a
// member on another zone is not in ents — interest resets on zone change),
// the forward-most, i.e. greatest x: the map-3 route (1,22)->(22,21)->
// (32,18)->(39,12)->(44,6) and the map-5 causeway (3,30)->(22,3) both
// march strictly east. Ties break on entity id. The front runner keeps the
// route machine; everyone else glues to within 3 tiles and fights the
// packs through with it.
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
  // T-118 crypt gate: boss tracking
  std::uint64_t bossSeen = 0;
  std::uint64_t bossKills = 0;
  std::uint64_t curseSeen = 0;
  std::uint64_t slamSeen = 0;
  std::uint32_t bossId = 0;
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
  // T-118 M3 gate: raider profile state (march/telegraph-dodge/boss telemetry)
  std::uint8_t statPoints = 0;   // from OwnStats; VIT-assign while banked
  std::uint32_t xp = 0;          // from OwnStats (summary line)
  double tMap5Entry = -1.0;      // seconds into the leg, first map-5 Welcome
  // bossId declared above (T-118 crypt gate block) — shared with the raider
  double tBossSight = -1.0;      // first EntitySpawn of her
  double tBossKilled = -1.0;     // CombatEvent kill on her (party TTK source)
  double slamDodgeUntil = 0.0;   // kind-15 telegraph on me: move until this t
  double nextDodgePathAt = 0.0, nextStatAt = 0.0;
  double nextFireboltAt = 0.0, nextIronAt = 0.0;
  std::uint64_t eliteKills = 0, trashKills = 0;  // wire kinds 10 / 7,8
  std::uint64_t dodges = 0;                      // slam-dodge windows entered
  // T-118 r3: staged-push route (the crypt is a single-corridor gauntlet —
  // legs 1-2 died to straight-through pulls; nodes + rests chew it down)
  int routeIdx = 0;
  double routeRest = 0.0;         // current node's rest seconds
  double routeRestUntil = 0.0;    // resting at the node until this t
  int safeX = -1, safeY = -1;     // retreat target for the raider (town/ossuary)
  double retreatT0 = 0.0;         // retreat time-cap (soft-lock guard)
  double lastTraceAt = 0.0;       // 5 s state-trace pace
  int bestDDebug = 100;           // last targeting pass (trace only)
  int swarmDebug = 0;
  // T-118 r8b: gauntlet column bookkeeping
  bool wasRunner = false;         // ran the route machine last pass
  double portalWaitT0 = 0.0;      // depths-stairs ring-wait start (0 = off)
  // T-118 r8d: quorumWaitT0 = the runner holding a gauntlet waypoint for
  // the column to close up (0 = not waiting; the r8c leg's string split).
  double quorumWaitT0 = 0.0;
};

// T-118 r8: the front runner of the gauntlet stack. The r7 leg's 24 deaths
// were five independent brains trading the crypt's packs one at a time —
// each bot marched its own route node, so the barrow ring (Cantor Vex +
// pulpit + sexton seat) was met two deep and lost. Deterministic election,
// identical on every bot: among party members alive and in MY view (a
// member on another zone is not in ents — interest resets on zone change),
// the forward-most, i.e. greatest x: the map-3 route (1,22)->(22,21)->
// (32,18)->(39,12)->(44,6) and the map-5 causeway (3,30)->(22,3) both
// march strictly east. Ties break on entity id. The front runner keeps the
// route machine; everyone else glues to within 3 tiles and fights the
// packs through with it.
struct FrontRunner {
  std::uint32_t id = 0;
  int x = 0, y = 0;
};
FrontRunner frontRunnerOf(const Bot& b) {
  // T-118 r8e: the r8d leg deadlocked here. Two bots on the same tile,
  // one tile of stale EntityDelta apart in each other's views, each saw
  // the OTHER's x as 1 greater — strict `x >` made each bot defer to the
  // other, so NO bot ran the route machine and the tight column milled
  // in place for 600 s (routeIdx frozen at 1, no advance, no deaths).
  // Rule now: the front runner is the smallest-id member within 1 tile
  // of the max-x. Two-pass, order-independent, identical on every view
  // whose position data agree within the 1-tile dead zone — the
  // staleness a live delta stream produces.
  //
  // T-118 r8e (the r8e2/r8e3 re-deadlock at (22,21)): SELF must be read
  // from b.tileX/Y, never b.ents. Own EntityDeltas update b.tileX/Y only
  // (the delta handler special-cases d.id == ownId and does NOT touch
  // ents), so ents[self] holds the LOGIN SPAWN position for the whole
  // leg — the true front runner (self.x=27) carried a ghost x=1 and was
  // outvoted by every live neighbour. Same trap hit the quorum count,
  // which undercounted the column by one (self at ghost (1,22)).
  struct Cand {
    std::uint32_t id;
    int x, y;
  };
  Cand cands[16];
  int nc = 0;
  for (const auto& [mid, row] : b.party) {
    if (row.hp == 0) continue;  // down: the stack re-forms without it
    if (mid == b.ownId) {  // self: own (fresh) tile, never ents
      cands[nc++] = {mid, b.tileX, b.tileY};
      continue;
    }
    const auto ei = b.ents.find(mid);
    if (ei == b.ents.end()) continue;  // other zone / not yet seen
    cands[nc++] = {mid, ei->second.x, ei->second.y};
  }
  if (nc == 0) return FrontRunner{};
  // T-118 r16: direction-aware extremum. Maps 3/5 march east — the front
  // is max-x. Map 1 (the town -> hatch return march) marches WEST: max-x
  // elected the REARMOST bot, so the column anchored on its own tail and
  // ping-ponged (every r13/r14/r15 leg burned its budget in sight of
  // town). Forward-most along the route = min-x on map 1.
  const bool west = (b.mapId == 1);
  int refX = west ? 100000 : -1;
  for (int i = 0; i < nc; ++i) {
    if (west ? (cands[i].x < refX) : (cands[i].x > refX)) refX = cands[i].x;
  }
  FrontRunner fr;
  for (int i = 0; i < nc; ++i) {
    // T-118 r14b: dead-band 2. The 1-tile staleness in peer positions
    // jitters the extremum by ±2; a 2-tile band keeps the brain put
    // while the column is bunched, a real breakaway (>2 ahead) takes it.
    const bool inBand =
        west ? (cands[i].x <= refX + 2) : (cands[i].x >= refX - 2);
    if (inBand && (fr.id == 0 || cands[i].id < fr.id)) {
      fr.id = cands[i].id;
      fr.x = cands[i].x;
      fr.y = cands[i].y;
    }
  }
  return fr;
}

// T-118 r8e: living party members (self INCLUDED, via b.tileX/Y — see the
// frontRunnerOf note on why ents[self] is a login-spawn ghost) within
// Chebyshev r of (cx,cy).
int quorumNear(const Bot& b, int cx, int cy, int r) {
  int q = 0;
  if (std::abs(b.tileX - cx) <= r && std::abs(b.tileY - cy) <= r) ++q;
  for (const auto& [mid, row] : b.party) {
    if (row.hp == 0 || mid == b.ownId) continue;
    const auto ei = b.ents.find(mid);
    if (ei == b.ents.end()) continue;
    if (std::max(std::abs(ei->second.x - cx), std::abs(ei->second.y - cy)) <=
        r)
      ++q;
  }
  return q;
}

// T-118 r4 route table: node x/y, rest seconds on arrival, portal flag.
// rest==0 = waypoint (advance on arrival; the push never stops mid-gauntlet).
// Map 1: spawn -> town (resupply) -> chapel edge (regroup) -> the hatch.
// Map 3 (the crypt is four rooms, verified against mapgen): entry hall
// (0-12,12-24, stairs_up at (0,17)) / ossuary gallery (16-46,14-22, ghoul
// racks 18-25,15-20) / candle crypt (16-30,24-34, widow cocoon 24-29,26-31,
// entry (24,30)) / bone barrow (34-46,2-10, cantor 34-37,6-8 + pulpit
// 42-45,3-6 + sexton 44-45,5-7, depths stairs at (44,6)). The r3 node
// (40,18) died leg 3: it sat on the racks' leash edge (14 from the (25,15)
// corner) AND the cocoon's, so two chased packs met the resting party
// (swarm 10, three deaths). r4: the party pushes the corridor spine —
// through the racks (L3: 7x64hp, a 6 s cull) and the cantor's aggro (8)
// in one unbroken pass; the only regroup is the entry hall (6,17), a
// separate room 12+ tiles from every anchor. The party never rests in the
// ossuary or the barrow — there is no safe floor there.
struct RaiderNode { int x, y; double rest; bool portal; };
static int raiderRoute(int mapId, int idx, int* x, int* y, double* rest,
                       bool* portal) {
  const RaiderNode* r = nullptr;
  int n = 0;
  switch (mapId) {
    case 1: {
      // T-118 r8: the gate leg is a transit, not a grind — the party enters
      // at full (top-up) and the rests only delay the hatch crossing.
      static const RaiderNode r1[] = {{32, 16, 3.0, false},
                                      {14, 14, 3.0, false},
                                      {10, 10, 0.0, true}};
      r = r1; n = 3; break;
    }
    case 3: {
      // Node floors verified against mapgen: hall (0-12,12-24), ossuary
      // (16-46,14-22), candle crypt (16-30,24-34), barrow (34-46,2-10);
      // corridors hall->ossuary (12-16,17-18), ossuary->crypt (22-23,22-24),
      // ossuary->barrow (38-39,10-14). (40,12) is a WALL — the r6 party
      // stalled at (20,20) on an unreachable goal; (39,12) is the corridor.
      // T-118 r8: the regroup moves from (12,17) to (1,22) — the entry
      // hall's far corner. (12,17) sat 6 tiles from the racks' spawn edge
      // (18,15-20): ghoul leash 14 reaches it, so the r7 party was picked
      // off at camp (m3g__04 lastDeath=(6,17)) and the 10 s rest inside
      // aggro range fed the death-hold. (1,22) is 17 tiles from the racks
      // corner (outside leash 14), 4 from the stairs_up portal (0,17), and
      // 19 from the cocoon — the only spot in the hall no pack reaches.
      static const RaiderNode r3[] = {{1, 22, 3.0, false},   // entry hall: stack forms
                                      {22, 21, 0.0, false},   // into the ossuary (racks)
                                      {32, 18, 0.0, false},   // across (cantor pulls)
                                      {39, 12, 0.0, false},   // barrow corridor
                                      {44, 6, 0.0, true}};    // depths stairs: settle
      r = r3; n = 5; break;
    }
    case 5: {
      static const RaiderNode r5[] = {{22, 3, 0.0, false}};
      r = r5; n = 1; break;
    }
    case 2: {
      static const RaiderNode r2[] = {{0, 14, 0.0, true}};
      r = r2; n = 1; break;
    }
    default: return 0;
  }
  if (idx < 0) idx = 0;
  if (idx >= n) idx = n - 1;  // hold at the last node until the map changes
  *x = r[idx].x;
  *y = r[idx].y;
  *rest = r[idx].rest;
  *portal = r[idx].portal;
  return n;
}

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
  int partySize = 0;    // T-118: N-party formation (>=3 replaces S13 pair-up)
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
    else if (a == "--party-size") partySize = std::atoi(next("0"));
    else {
      std::fprintf(stderr,
                   "usage: bh_bots [--host H] [--port P] [--count N] [--secs S] [--map M] "
                  "[--prefix P] [--profile wander|fighter|pilgrim|campaign|crypt|raider] "
                  "[--target-level N] [--party-size N]\n");
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
                if (b.welcomed && w.mapId == 5 && b.tMap5Entry < 0.0 &&
                    b.campaignT0 >= 0.0)  // T-118: gate entry stamp
                  b.tMap5Entry = nowSec() - b.campaignT0;
                b.mapId = w.mapId;
                if (b.welcomed) {  // repeated Welcome = zone transfer (T-036)
                  b.ents.clear();
                  b.attackTarget = 0;
                  b.routeIdx = 0;        // T-118 r3: new map, new route
                  b.routeRestUntil = 0.0;
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
                se.hp = m.hp;
                b.ents[m.id] = se;
                if (m.id == b.ownId) {
                  b.hp = static_cast<std::int32_t>(m.hp);
                  b.hpMax = static_cast<std::int32_t>(m.hpMax);
                }
                // T-118: Gravemother = wire kind 9 (mob 1009) at L14
                if (m.kind == 9 || m.level == 14) {
                  ++b.bossSeen;
                  b.bossId = m.id;
                  if (b.tBossSight < 0.0 && b.campaignT0 >= 0.0)
                    b.tBossSight = nowSec() - b.campaignT0;
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
                    it->second.hp = d.hp;  // T-118 r7: focus-fire state
                  }
                }
              }
            } else if (pv.id == bh::proto::kIdEntityDespawn) {
              bh::proto::EntityDespawn d;
              if (d.deserialize(pv.body)) b.ents.erase(d.id);
              if (b.attackTarget == d.id) b.attackTarget = 0;
              if (d.id == b.bossId) b.bossId = 0;  // T-118: she's gone
            } else if (pv.id == bh::proto::kIdCombatEvent) {
              bh::proto::CombatEvent m;
              if (m.deserialize(pv.body)) {
                if (m.kind == 3) {
                  if (m.attackerId == b.ownId) {
                    ++b.kills;
                    if (b.bossId != 0 && m.targetId == b.bossId) ++b.bossKills;
                    else {
                      auto it = b.ents.find(m.targetId);
                      if (it != b.ents.end() && (it->second.kind == 9 || it->second.level == 14)) ++b.bossKills;
                    }
                  }
                  if (m.targetId == b.ownId) {  // T-040: authoritative source
                    ++b.deaths;
                    int kl = 0;
                    const auto ki = b.ents.find(m.attackerId);
                    if (ki != b.ents.end()) kl = ki->second.level;
                    ++b.deathByKillerLevel[kl];
                    b.lastDeathX = b.tileX;
                    b.lastDeathY = b.tileY;
                  }
                  // T-118 raid telemetry: classify the kill, stamp the boss
                  if (b.campaignT0 >= 0.0) {
                    const auto te = b.ents.find(m.targetId);
                    if (te != b.ents.end()) {
                      if (te->second.kind == 10) ++b.eliteKills;  // Sepulcher
                      else if (te->second.kind == 7 || te->second.kind == 8)
                        ++b.trashKills;  // gravecaller / sexton
                    }
                    if (m.targetId == b.bossId && b.tBossKilled < 0.0)
                      b.tBossKilled = nowSec() - b.campaignT0;
                  }
                } else if (m.kind == 9 || m.kind == 16) {
                  if (m.targetId == b.ownId) {
                    if (m.kind == 9) ++b.curseSeen;
                    else ++b.slamSeen;
                  }
                } else if (m.kind == 15) {
                  ++b.slamSeen;
                  if (m.targetId == b.ownId) {
                    // T-091 telegraph on me: the rot blooms on my tile in 3 s
                    b.slamDodgeUntil = nowSec() + 3.5;
                    ++b.dodges;  // one window per telegraph (only the Mother
                                 // arms kind 15 — Cantor Vex stays instant)
                  }
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
                b.xp = m.xp;             // T-118 summary
                b.statPoints = m.statPoints;
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
      const bool crypt = (profile == "crypt" || profile == "crypt_party");
      // T-118 crypt gate: 5-person mixed-kit party -> Gravemother
      if (crypt) {
        if (b.campaignT0 < 0.0) {
          b.campaignT0 = t;
          b.nextRestAt = t + 60.0 + rng.range(0, 30);
        }
        if (!b.kitSworn && t >= b.campaignT0 + 2.0 + botIdx * 0.5) {
          b.kitSworn = true;
          const char* want = "ravager";
          if (botIdx == 1 || botIdx == 2) want = "cultist";
          else if (botIdx == 3) want = "gravecaller";
          bh::proto::ChatSend cs;
          cs.channel = 0;
          cs.text = std::string("/kit ") + want;
          sendProto(b.peer, bh::proto::pack(cs));
        }
        {
          if (botIdx == 0) {
            if (b.party.size() < 5 && b.partyTries < 100 && t >= b.nextPartyTryAt) {
              size_t target = SIZE_MAX;
              for (size_t si = 1; si < bots.size(); ++si) {
                if (!bots[si].welcomed) continue;
                bool already = false;
                for (const auto& kv : b.party) {
                  if (kv.second.name == bots[si].name) { already = true; break; }
                }
                if (!already) { target = si; break; }
              }
              if (target == SIZE_MAX) {
                // re-invite any welcomed bot not yet confirmed (handles death/leaves)
                for (size_t si = 1; si < bots.size(); ++si) {
                  if (bots[si].welcomed) { target = si; break; }
                }
              }
              if (target != SIZE_MAX) {
                b.nextPartyTryAt = t + 3.0;
                ++b.partyTries;
                bh::proto::ChatSend cs;
                cs.channel = 0;
                cs.text = "/invite " + bots[target].name;
                sendProto(b.peer, bh::proto::pack(cs));
              }
            }
          } else {
            const bool inParty = b.party.size() >= 5;
            if (!inParty && b.partyTries < 100 && t >= b.nextPartyTryAt) {
              b.nextPartyTryAt = t + 3.0;
              ++b.partyTries;
              bh::proto::ChatSend cs;
              cs.channel = 0;
              cs.text = "/accept";
              sendProto(b.peer, bh::proto::pack(cs));
            }
          }
        }
        b.campIsPortal = false;
        // T-118: hold in town until party of 5 forms, then dive
        if (b.party.size() < 5) {
          if (b.homeX >= 0) { b.campX = b.homeX; b.campY = b.homeY; }
          else { b.campX = 32; b.campY = 16; }
          b.campIsPortal = false;
        } else if (b.mapId == 1) {
          b.campX = 10; b.campY = 10; b.campIsPortal = true;
        } else if (b.mapId == 3) {
          b.campX = 44; b.campY = 6; b.campIsPortal = true;
        } else if (b.mapId == 5) {
          b.campX = 21; b.campY = 2; b.campIsPortal = false;
        } else {
          b.campX = 0; b.campY = 14; b.campIsPortal = true;
        }
        if (b.kitClass == 3 && !b.party.empty()) {
          auto cheb = [&](int ax, int ay, int bx, int by) {
            const int dx = ax > bx ? ax - bx : bx - ax;
            const int dy = ay > by ? ay - by : by - ay;
            return dx > dy ? dx : dy;
          };
          std::uint32_t hurtId = 0;
          int hurtDist = 99;
          int hurtSeen = 0;
          for (const auto& kv2 : b.party) {
            if (kv2.second.hp * 5 < kv2.second.hpMax * 3) ++hurtSeen;
            if (kv2.second.hp * 5 >= kv2.second.hpMax * 3) continue;
            const auto ei = b.ents.find(kv2.first);
            if (ei == b.ents.end()) continue;
            const int d = cheb(b.tileX, b.tileY, ei->second.x, ei->second.y);
            if (d < hurtDist) { hurtDist = d; hurtId = kv2.first; }
          }
          if (hurtSeen > 0 && hurtId == 0) ++b.mendNoSee;
          b.mendHurtCnt += static_cast<std::uint64_t>(hurtSeen);
          const auto lead = b.party.find(b.partyLeaderId);
          const auto leadE = lead != b.party.end() ? b.ents.find(lead->first) : b.ents.end();
          const int leadDist = leadE != b.ents.end() ? cheb(b.tileX, b.tileY, leadE->second.x, leadE->second.y) : 99;
          if (b.level >= 3 && leadE != b.ents.end() && leadDist <= 6 && t >= b.nextBlessAt && b.mp >= 15) {
            bh::proto::SkillUse su; su.skill = 3; su.targetId = b.partyLeaderId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextBlessAt = t + 240.0; ++b.blessCasts;
          }
          if (b.level >= bh::content::kitSkillUnlock(b.kitClass, 6) && b.party.size() >= 2 && b.mp >= 14 && t >= b.nextChorusAt) {
            std::uint32_t voices = 1;
            for (const auto& kv2 : b.party) {
              const auto ei = b.ents.find(kv2.first);
              if (ei != b.ents.end() && cheb(b.tileX, b.tileY, ei->second.x, ei->second.y) <= 6) ++voices;
            }
            if (voices >= 2) {
              bh::proto::SkillUse su; su.skill = 6; su.targetId = b.ownId;
              sendProto(b.peer, bh::proto::pack(su));
              b.nextChorusAt = t + 150.0; ++b.chorusCasts;
            }
          }
          bool massMended = false;
          if (b.level >= bh::content::kitSkillUnlock(b.kitClass, 7) && b.mp >= 18 && hurtSeen >= 2 && t >= b.nextMassMendAt) {
            bh::proto::SkillUse su; su.skill = 7; su.targetId = b.ownId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextMassMendAt = t + 3.5; ++b.massCasts; massMended = true;
          }
          bool mobAdj = false;
          for (const auto& kv2 : b.ents) {
            if (kv2.second.kind == 0 || bh::content::wireIsFurniture(kv2.second.kind)) continue;
            if (cheb(b.tileX, b.tileY, kv2.second.x, kv2.second.y) <= 2) { mobAdj = true; break; }
          }
          if (b.level >= bh::content::kitSkillUnlock(b.kitClass, 8) && b.mp >= 10 && mobAdj && t >= b.nextHasteAt) {
            bh::proto::SkillUse su; su.skill = 8; su.targetId = b.ownId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextHasteAt = t + 70.0; ++b.hasteCasts;
          }
          if (hurtId != 0 && !massMended) {
            if (hurtDist <= 6 && t >= b.nextMendAt && b.mp >= 8) {
              bh::proto::SkillUse su; su.skill = 2; su.targetId = hurtId;
              sendProto(b.peer, bh::proto::pack(su));
              b.nextMendAt = t + 1.5; ++b.swings; ++b.mendCasts;
            }
          }
        }
      }
      const bool raider = (profile == "raider");  // T-118 M3 gate
      // T-118 r3: 5 s state trace — the gate legs died silently, and the
      // journal shows WHAT was sent but not WHICH state sent it.
      if (raider && b.welcomed && t - b.lastTraceAt >= 5.0) {
        b.lastTraceAt = t;
        // TEMP r8d debug (remove before merge): fr election + quorum.
        const FrontRunner dfr = frontRunnerOf(b);
        const int dq =
            (b.campX >= 0) ? quorumNear(b, b.campX, b.campY, 6) : 0;
        std::fprintf(stderr,
                     "[trace] %-9s t=%.0f (%d,%d) map=%u ret=%d rest=%.0f "
                     "route=%d camp=(%d,%d) nd=%d hp=%d%% bestD=%d "
                     "swarm=%d nearTown=%d fr=%u@(%d,%d) q=%d qT0=%.0f\n",
                     b.name.c_str(), t, b.tileX, b.tileY, b.mapId,
                     (int)b.retreating, b.routeRest, b.routeIdx, b.campX,
                     b.campY,
                     b.campX >= 0
                         ? std::max(std::abs(b.tileX - b.campX),
                                    std::abs(b.tileY - b.campY))
                         : -1,
                     b.hpMax > 0 ? b.hp * 100 / b.hpMax : 0, b.bestDDebug,
                     b.swarmDebug,
                     (int)(b.mapId == 1 && b.homeX >= 0 &&
                           std::abs(b.tileX - b.homeX) < 4 &&
                           std::abs(b.tileY - b.homeY) < 4),
                     dfr.id, dfr.x, dfr.y, dq, b.quorumWaitT0);
      }
      if (campaign || raider) {
        if (b.campaignT0 < 0.0) {
          b.campaignT0 = t;
          // S13 auto-party: sibling pair-up so party-shared legs gate M2b-final.
          // Even-indexed bot invites its successor, odd accepts (server
          // resolves the name -> entityId before journaling; replay-exact).
          (void)0;
          if (!raider)
            b.nextRestAt = t + 60.0 + rng.range(0, 30);  // first break a minute in
        }
        // player-paced rhythm: ~60-90s engaged, then a 6-11s door-stop.
        // The raider fights flat — the gate measures sustained pressure.
        if (!raider && t >= b.nextRestAt) {
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
        b.routeRest = 0.0;
        if (raider) {
          // T-118 r3 staged push: the camp IS the current route node (see
          // raiderRoute). Portal nodes settle EXACTLY (the fire check needs
          // a settled walker); rest nodes pause; the font is a fight camp.
          int rx = 0, ry = 0;
          double rrest = 0.0;
          bool rportal = false;
          b.campX = -1;
          if (raiderRoute(b.mapId, b.routeIdx, &rx, &ry, &rrest, &rportal)) {
            b.campX = rx;
            b.campY = ry;
            b.campIsPortal = rportal;
            b.routeRest = rrest;
          }
          // T-118 r16: desync re-adoption. Retreat drops the walker at the
          // safe node but routeIdx survives intact: the r15 leg's 03
          // resumed at route=2 from camp (1,22) and set off across the
          // whole gauntlet solo (31 tiles of racks, cocoon and ring). If
          // the current node is far (>8), re-adopt the nearest node —
          // same scan the runner transition uses.
          if (b.campX >= 0 &&
              std::max(std::abs(b.tileX - b.campX),
                       std::abs(b.tileY - b.campY)) > 8) {
            int best = 100000, bi = b.routeIdx;
            for (int i = 0; i < raiderRoute(b.mapId, 0, &rx, &ry, &rrest, &rportal);
                 ++i) {
              raiderRoute(b.mapId, i, &rx, &ry, &rrest, &rportal);
              const int d = std::max(std::abs(rx - b.tileX),
                                     std::abs(ry - b.tileY));
              if (d <= best) { best = d; bi = i; }
            }
            if (bi != b.routeIdx) {
              b.routeIdx = bi;
              if (raiderRoute(b.mapId, b.routeIdx, &rx, &ry, &rrest, &rportal)) {
                b.campX = rx;
                b.campY = ry;
                b.campIsPortal = rportal;
                b.routeRest = rrest;
              }
            }
          }
          // retreat target: town on map 1; on map 3 the entry hall's far
          // corner (1,22) — T-118 r8 moved it from (6,17): the racks'
          // ghoul leash (14 from the 18-25,15-20 spawn rect) REACHES (6,17)
          // and the r7 death-hold ran on it (bots holding the "safe" node
          // at 17-35% hp while the 30 s rack respawns walked in). (1,22)
          // is outside every leash: racks 17, cocoon 19, portal (0,17) 4.
          // Map 5 has no retreat (the measure); map 2 falls back to axis.
          b.safeX = (b.mapId == 1) ? 32 : (b.mapId == 3) ? 1 : -1;
          b.safeY = (b.mapId == 1) ? 16 : (b.mapId == 3) ? 22 : -1;
        } else if (b.mapId == 3) {
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
          if (b.level >= 8) {
            // T-084 opens the L8->L9 step-up: widow_glade (L9, rect
            // x[58,63] y[42,45]) shares the south bank with gnolls_pits
            // (x[50,57]). Camp the widow NORTH edge so widows (wander 6,
            // aggro 6, leash 10) pull straight onto the camp; gnolls
            // (wander 8, aggro 7) still reach from the west and read as
            // fodder via the defend branch. Retreat-home crosses the
            // river north, same as the L7 camp. Pack cap + swarm
            // panic-break stay the levers -- no mob numbers moved.
            b.campX = 60; b.campY = 41;  // widow_glade NORTH EDGE
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
        if (bots.size() >= 2 && unformed && partySize < 3) {  // legacy pairs
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
        // T-118 N-party: with --party-size N (>=3) bot 0 leads and invites
        // bots 1..N-1 in one pass (the server keeps one pending invite per
        // invitee, renewed each round), the rest accept on the same 10 s
        // heartbeat. Invites range 12 tiles; everyone converges on the
        // waypoint first, so formation lands once the column is together.
        // The raider retries without the 4-attempt cap — the gate needs the
        // circle, and re-inviting is free (renew, not spam: 10 s cadence).
        if (partySize >= 3 && botIdx < (size_t)partySize &&
            bots.size() >= (size_t)partySize &&
            b.party.size() < (size_t)partySize &&
            t >= b.nextPartyTryAt && (raider || b.partyTries < 4)) {
          b.nextPartyTryAt = t + 10.0;
          ++b.partyTries;
          if (botIdx == 0) {
            for (int k = 1; k < partySize && k < (int)bots.size(); ++k) {
              if (!bots[static_cast<size_t>(k)].welcomed) continue;
              bh::proto::ChatSend cs;
              cs.channel = 0;
              cs.text = "/invite " + bots[static_cast<size_t>(k)].name;
              sendProto(b.peer, bh::proto::pack(cs));
            }
          } else {
            bh::proto::ChatSend cs;
            cs.channel = 0;
            cs.text = "/accept";
            sendProto(b.peer, bh::proto::pack(cs));
          }
        }
        // oath first (T-053/T-118): one-time, idempotent (the server's oath
        // is one-shot — a re-say is an ordinary chat line, not a re-choose).
        // Legacy pairs: odd swears Cultist. T-118 N-party mixed kits:
        // 0/4 Ravager (default class, no oath), 1/3 Cultist (the menders),
        // 2 Gravecaller (the kiter).
        if (!b.kitSworn && t >= b.campaignT0 + 3.0) {
          std::string oath;
          if (partySize >= 3 && botIdx < (size_t)partySize) {
            if (botIdx == 1 || botIdx == 3) oath = "/kit cultist";
            else if (botIdx == 2) oath = "/kit gravecaller";
          } else if (botIdx % 2 == 1) {
            oath = "/kit cultist";
          }
          if (!oath.empty()) {
            b.kitSworn = true;
            bh::proto::ChatSend cs;
            cs.channel = 0;
            cs.text = oath;
            sendProto(b.peer, bh::proto::pack(cs));
          }
        }
        // T-118 stat bank: the raider spends unassigned points into VIT
        // (one assign per 2 s — the server CD). hpMax = 40 + 6*lvl + 6*VIT:
        // a banked VIT pool is the gate's survivability lever.
        if (raider && b.statPoints > 0 && t >= b.nextStatAt) {
          b.nextStatAt = t + 2.0;
          bh::proto::StatAssign sa;
          sa.stat = 1;  // VIT
          sendProto(b.peer, bh::proto::pack(sa));
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
          // T-118 raid: ch4 Ironskin (Choir L3, +20% DR 5 min) — the
          // cultist's plate against the boss swing. Shipped channel the
          // existing bots never cast; the 190 s period outruns the 300 s
          // duration.
          // kitSkillUnlock returns 0 for a channel the kit CANNOT use —
          // require a positive unlock or the ravager would spam a dead cast.
          const std::uint8_t ironUnlock =
              bh::content::kitSkillUnlock(b.kitClass, 4);
          if (raider && ironUnlock > 0 && b.level >= ironUnlock &&
              b.mp >= 15 && t >= b.nextIronAt) {
            bh::proto::SkillUse su;
            su.skill = 4;
            su.targetId = b.ownId;
            sendProto(b.peer, bh::proto::pack(su));
            b.nextIronAt = t + 190.0;
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
              // T-118 raid: the Gravecaller's verse runs the gauntlet —
              // 90 s period keeps the +5% damage near-continuous (120 s
              // duration, 12 s server CD, 14 mp well inside regen).
              b.nextChorusAt = t + (raider ? 90.0 : 150.0);
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
      if (profile == "fighter" || pilgrimRites || campaign || crypt || raider) {
        // nearest mob within 10 tiles -> chase / attack
        std::uint32_t bestId = 0;
        int bestD = 100;
        std::uint32_t bestHp = 0;  // T-118 r7: focus-fire mark (raider only)
        int swarmOnUs = 0;  // Route v5: aggressive mobs within 2 tiles of US --
                            // the dive-death gauge that drives the early break
        for (const auto& kv : b.ents) {
          if (kv.second.kind == 0 || bh::content::wireIsFurniture(kv.second.kind)) continue;  // players, furniture
          const int d = std::max(std::abs(kv.second.x - b.tileX),
                                 std::abs(kv.second.y - b.tileY));
          // T-118 r2: the raider counts the swarm too — leg 1's 44 deaths
          // were the raiders (campaign=false) blinding past every filter.
          if ((campaign || crypt || raider) && d <= 2 && entAggressive(kv.second.kind))
            ++swarmOnUs;
          if ((campaign || crypt) && kv.second.level > b.level + 2 && d > 1) continue;  // no walls: strike back only at point-blank
          if ((campaign || crypt) && t < b.restUntil && d > 1) continue;  // resting: fight back only
          // Sword-off at portal camps: any AttackRequest path-clears, and
          // bat/ghoul harassment at the hatch livelocked the crossing
          // (observed smoke v2). T-118: the raider inherits it for the
          // map-1/2 crossings — but NOT the map-3 depths stairs, where the
          // sexton seat + cantor choir + gravecaller pulpit ring the portal
          // and the raid FIGHTS its approach to the font. The crypt profile
          // keeps sword-off at ALL portal camps (its map-3 camp IS the
          // depths stairs, crossed hands-off like the hatch).
          if (b.campIsPortal &&
              (campaign || crypt || (raider && b.mapId != 3)))
            continue;
          // Route v5c: "no new pulls" means no new pulls -- a fleeing bot must
          // still swing at whatever is already on it (see the defend block in
          // the retreat branch below).  Skipping point-blank here too left
          // bestId == 0 for the whole disengage and zeroed XP gain.
          if ((campaign || crypt) && b.retreating && d > 1) continue;
          // pull singles: skip packed targets — T-118: the Gravemother
          // (wire kind 9, L14) is the gate objective; she is worth the pack.
          // T-118 r2: the raider obeys it too — the cocoon's 6-widow pack
          // at the depths entry killed leg 1's party faster than it could
          // kill back; singles + the panic-break below chew it down.
          // T-118 r4: in the crypt the pack cap is OFF — the gauntlet IS the
          // packs (the 6-widow cocoon, the 7-ghoul racks, the barrow ring);
          // a cap that skips them left the party walking through aggro
          // taking hits it could not answer (leg 3).
          // T-118 r8: same for map 5 — the apse ring (elites L/R + sexton +
          // the Gravemother, 4 within 3 tiles) is the boss wall, and a cap
          // that skips it leaves the stack standing in the bolt range.
          if ((campaign || crypt ||
               (raider && b.mapId != 3 && b.mapId != 5)) && d > 1 &&
              !(kv.second.kind == 9 && kv.second.level == 14)) {
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
          // T-118 r7: FOCUS FIRE. The r6b leg's 34 deaths were 28 L9
          // widows: five bots each picking their own nearest widow spread
          // the party ~0.8 bots per mark — no widow died before the party
          // did. The raider marks the LOWEST-HP candidate instead (tie:
          // nearest), so the five bots converge on the same dying target
          // and finish it before the next mark matters.
          if (raider) {
            const bool candAgg = entAggressive(kv.second.kind);
            const bool bestAgg =
                bestId != 0 && entAggressive(b.ents[bestId].kind);
            if (bestId == 0 ||
                (candAgg && !bestAgg) ||  // threats beat wounded passives
                (candAgg == bestAgg &&
                 (kv.second.hp < bestHp ||
                  (kv.second.hp == bestHp && d < bestD)))) {
              bestId = kv.first;
              bestD = d;
              bestHp = kv.second.hp;
            }
          } else if (d < bestD) {
            bestD = d;
            bestId = kv.first;
          }
        }
        b.bestDDebug = bestD;
        b.swarmDebug = swarmOnUs;
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
        if ((campaign || crypt) && nearTown && hasBlade && bladeArmed && t >= b.nextShopAt) {
          int vials = 0;
          for (const auto& kv : b.inv)
            if (kv.second.itemId == 3001) vials += kv.second.qty;
          // T-114 (bots v3 flask belt): the 4-deep belt refilled 2-at-a-visit
          // was the L9 pace tax — T-090/T-100 measured ~30 town stops/leg for
          // the pair (T-113 finding 2: the empty belt forces the hp-dip ->
          // retreat-home -> refill cycle; the server never capped depth).
          // Buy to a 16-deep belt in one stop, gated at 240g and never
          // spending below the 90g gear/rite floor.
          if (vials < 16 && b.gold >= 240) {
            const int afford = static_cast<int>((b.gold - 90) / 30);
            const int qty = std::min(16 - vials, afford);
            if (qty > 0) {
              b.nextShopAt = t + 2.0;
              bh::proto::BuyRequest buy;
              buy.itemId = 3001;
              buy.qty = static_cast<std::uint32_t>(qty);
              sendProto(b.peer, bh::proto::pack(buy));
              ++b.shops;
            }
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
        } else if ((campaign || crypt) && bladeArmed && hasArmor && !armorWorn && t >= b.nextShopAt) {
          b.nextShopAt = t + 2.0;
          bh::proto::ToggleEquip te;
          te.slot = armorSlot;
          sendProto(b.peer, bh::proto::pack(te));  // equip anywhere: the re-gear
          // trip buys armor but the bot leaves town before the nearTown-gated
          // branch fires — blade-only bots get shredded by ghoul/hound packs
        } else if ((campaign || crypt) && nearTown && bladeArmed && !hasArmor && b.gold >= 120 && t >= b.nextShopAt) {
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
        const int sipPct =
            ((campaign || crypt || raider) && (b.retreating || threatAdj)) ? 65 : 50;
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
        // T-118 r2: the approach keeps the campaign's full survival kit —
        // swarm panic, hurt/critical break, the sticky heal-and-unpursued
        // release. Map 5 is the measure itself: there the raider fights
        // flat (vials at 65%, choir mend + ironskin); a death at the font
        // is data for the verdict, not a re-run.
        if (campaign || crypt || (raider && b.mapId != 5)) {
          const bool hurt = b.hpMax > 0 && b.hp * 100 < b.hpMax * 50;
          const bool critical = b.hpMax > 0 && b.hp * 100 < b.hpMax * 35;
          // T-118 r4: in the crypt (map 3) the raider fights flat through
          // the gauntlet — a swarm-3 or a 50% hit used to send it pulling
          // out (leg 3: the party fragmented on every pack and never
          // reached the font). There the ONLY retreat is critical: sub-35%
          // with a threat on top = pullout to the entry hall.
          const bool gauntlet = (raider && b.mapId == 3);
          if (b.level >= 3 && !gauntlet && swarmOnUs >= (raider ? 3 : 5) &&
              !b.retreating) {
            // Route v5: 5+ aggressive on us is a lost trade at ANY hp --
            // break before the red. The threshold is high on purpose in the
            // OPEN fields (at 3 the campaign disengaged from every normal
            // ghoul trade and never banked XP; T-077 r2 tried 4: 92 deaths
            // vs 32). The raider uses 3 in the open: 4 mobs within 2 tiles
            // is already 80+ raw dmg/s and the safe node is the way out.
            b.retreating = true;
            b.retreatT0 = t;
          } else if (hurt && !gauntlet && !b.retreating && bestId != 0 &&
                     bestD <= 2) {
            b.retreating = true;
            b.retreatT0 = t;
          } else if (critical && !b.retreating && bestId != 0 && bestD <= 6) {
            b.retreating = true;  // v2 band: sub-35% with a threat near — break
            b.retreatT0 = t;
          } else if ((b.retreating &&
                     ((b.hp * 100 >= b.hpMax * (nearTown ? 75 : 85)) ||
                      // T-118 r3: time cap — an empty belt + no mend in sight
                      // must not soft-lock the bot in retreat forever.
                      (t - b.retreatT0 > 45.0)) &&
                     (nearTown || bestId == 0 || bestD > 8 ||
                      // T-118 r6: the cap ALSO bypasses the unpursued
                      // check — a pinned bot (flee off the map edge) with a
                      // phantom blocker at d 3-8 else retreats forever.
                      (t - b.retreatT0 > 45.0))) ||
                     // T-118 r8g: gauntlet stack-retreat release. The r8f
                     // leg's 00 ran the 13-tile safe-node route through
                     // the racks' leash field at 36% and burned all 16
                     // vials before dying at (19,18) — the sticky 85%/d>8
                     // release can never fire on a 13-tile bleed. The
                     // stack retreat (below) ends at the sip band (65%)
                     // with the point-blank clear; the 45 s cap remains.
                     (raider && b.mapId == 3 &&
                      ((b.hp * 100 >= b.hpMax * 65 && bestD > 2) ||
                       (t - b.retreatT0 > 45.0)))) {
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
            if (raider && b.safeX >= 0) {
              // T-118 r3/r6: the raider's retreat target is the route's
              // safe node (town / entry hall). AT the node the bot STANDS —
              // the old fall-through sent it on a 10-tile threat-axis flee
              // that ran off the map edge (no path, no release: the r5b
              // party pinned at (6,17)/(3,17) for 150 s). Sip + mend + the
              // point-blank defend swing cover the wait; the release above
              // (healed / 45 s cap) ends it.
              // T-118 r8: on the gauntlet maps a critical FOLLOWER pulls to
              // the front runner instead of alone across the racks — it
              // stays in the stack (vials at 65%, the choir's mass mend,
              // the point-blank defend swing) and the fight goes on without
              // the 60 s town round-trip. The runner ITSELF pulls to the
              // safe node, and the followers' stack glue drags the column
              // back with it.
              // T-118 r8g: map 1 keeps the town safe node (unchanged).
              // On the gauntlet the default is HOLD — the r8f leg's
              // critical runner ran the safe-node route (13 tiles,
              // straight through the racks' leash field) and died
              // mid-run with a full belt. There the column IS the safe
              // ground: pull to the nearest member that is not itself
              // in the fire, and if every member is in the fire, stand
              // and defend in place (point-blank swing + 65% sip band +
              // choir mend).
              int rx = b.safeX, ry = b.safeY;
              if (b.mapId == 3) {
                rx = b.tileX;
                ry = b.tileY;
                const auto inFire = [&](int px, int py) {
                  for (const auto& kv : b.ents) {
                    if (kv.second.kind == 0 ||
                        !entAggressive(kv.second.kind))
                      continue;
                    if (std::max(std::abs(kv.second.x - px),
                                 std::abs(kv.second.y - py)) <= 3)
                      return true;
                  }
                  return false;
                };
                int best = 1000, bx = 0, by = 0;
                for (const auto& [mid, row] : b.party) {
                  if (row.hp == 0 || mid == b.ownId) continue;
                  const auto ei = b.ents.find(mid);
                  if (ei == b.ents.end()) continue;
                  if (inFire(ei->second.x, ei->second.y)) continue;
                  const int d = std::max(std::abs(ei->second.x - b.tileX),
                                         std::abs(ei->second.y - b.tileY));
                  if (d < best) {
                    best = d;
                    bx = ei->second.x;
                    by = ei->second.y;
                  }
                }
                if (best <= 12) {
                  rx = bx;
                  ry = by;
                }
              }
              const int sd = std::max(std::abs(b.tileX - rx),
                                      std::abs(b.tileY - ry));
              if (sd > 1) {
                bh::proto::InputPath ip;
                ip.goalX = rx;
                ip.goalY = ry;
                sendProto(b.peer, bh::proto::pack(ip));
              }
            } else if (b.mapId == 1 && b.homeX >= 0 && !nearTown) {
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
            // T-118 kiter bolt: the Cultist's Firebolt (ch5, range 8,
            // no-DEF plague fire). Cast en route whenever the mark is in
            // range; the movement policy below decides where she steps.
            // (Firebolt is the Pale Choir's channel — the Gravecaller kit
            // is Chorus/Haste, so the kiter is kitClass 3.)
            if (raider && b.kitClass == 3 && bestD <= 8 &&
                t >= b.nextFireboltAt) {
              if (b.mp >= 12) {
                bh::proto::SkillUse su;
                su.skill = 5;
                su.targetId = bestId;
                sendProto(b.peer, bh::proto::pack(su));
                b.nextFireboltAt = t + 2.5;
                ++b.swings;  // book the bolt like a swing for telemetry
              } else {
                b.nextFireboltAt = t + 1.5;  // MP regen 2/2s: recheck soon
              }
            }
            // T-118 r5: on the open fields the raider NEVER stops for a
            // fight — the r4 gate sat 150 s on a respawning pack because
            // "something is always within 12" in mob country, so the route
            // machine (below) never ran. March-first: bolt en route above,
            // then FALL THROUGH to the march branch — the route walk IS the
            // movement. (An early version `continue`d here, which stalled
            // the party at town on any passive rat within 12.)
            // T-118 r6: a PASSIVE mark is never worth stopping for either
            // — the r5b hall rat sat inside the kiter's band for 65 s
            // (band-mid = stand and shoot) while the party's march stalled.
            const bool passiveMark =
                b.ents.find(bestId) != b.ents.end() &&
                !entAggressive(b.ents[bestId].kind);
            if (raider && (b.mapId == 1 || passiveMark)) {
              b.attackTarget = 0;
            } else {
            // T-118 kiter band (gauntlet/boss maps): hold the line instead
            // of the route. Bolt casters (kind 14 Cantor, kind 9
            // Gravemother: bolt range 6) get the far band 7-8 — inside
            // our Firebolt's range 8, outside theirs. Melee: 3-6.
            // T-118 r8c: OFF on the march-first maps — the band's
            // stand-and-shoot `continue` would pin a Cultist runner at the
            // first respawned ghoul and stall the column exactly like the
            // combat gate did. The bolt cast above still runs; the route
            // walk supplies the position (the font node (22,3) IS inside
            // the boss band, the portal node (44,6) just outside the
            // cantor's).
            if (raider && b.kitClass == 3 && bestD <= 8 &&
                b.mapId != 1 && b.mapId != 3 && b.mapId != 5) {
              b.attackTarget = 0;
              const SeenEnt& se = b.ents[bestId];
              const bool caster = (se.kind == 14 || se.kind == 9);
              const int farEdge = caster ? 8 : 6;
              const int nearEdge = caster ? 6 : 3;
              if (bestD > farEdge) {
                // close from range
                bh::proto::InputPath ip;
                ip.goalX = se.x;
                ip.goalY = se.y;
                sendProto(b.peer, bh::proto::pack(ip));
              } else if (bestD <= nearEdge) {
                // inside the band's near edge: step 4 out, away from her
                const int ax = b.tileX - se.x, ay = b.tileY - se.y;
                int gx = b.tileX, gy = b.tileY;
                if (std::abs(ax) >= std::abs(ay) && ax != 0)
                  gx += (ax > 0 ? 4 : -4);
                else if (ay != 0)
                  gy += (ay > 0 ? 4 : -4);
                else
                  gx += 4;
                if (map->inBounds(gx, gy) && !map->isBlocked(gx, gy)) {
                  bh::proto::InputPath ip;
                  ip.goalX = gx;
                  ip.goalY = gy;
                  sendProto(b.peer, bh::proto::pack(ip));
                }
              }
              // band middle: stand and shoot; re-evaluate next tick
              b.nextMoveAt = t + 0.5;
              continue;
            }
            b.attackTarget = 0;
            const SeenEnt& se = b.ents[bestId];
            // T-118 r8f: a MILLER (within 3 of the front runner) holds
            // its stack slot — no march path. The r8e4 leg's node (22,21)
            // fight: each miller's march path to its own mark overrode
            // the column glue (the mill branch sends no path, so the
            // combat path won), the 5-stack fragmented into solo trades
            // against the swarm-8 racks+cocoon pile, and three died on
            // the node. Attack when adjacent, march only when loose.
            // (r11 generalised this to ALL raiders on maps 3/5; r14
            // restores the runner's chase — the no-chase sprint dragged
            // the whole double-leash field into one swarm-10 convergence
            // at (32,18) and wiped. r9's posture: the runner fights its
            // way along the route, millers hold their slots.)
            {
              const FrontRunner hfr = frontRunnerOf(b);
              const bool holdStack = raider &&
                  (b.mapId == 3 || b.mapId == 5) && hfr.id != 0 &&
                  hfr.id != b.ownId &&
                  std::max(std::abs(hfr.x - b.tileX),
                           std::abs(hfr.y - b.tileY)) <= 3;
              if (!holdStack) {
                bh::proto::InputPath ip;
                ip.goalX = se.x;
                ip.goalY = se.y;
                sendProto(b.peer, bh::proto::pack(ip));
              }
            }
            b.nextMoveAt = t + 0.5;
            }  // close: not the map-1 march-first path
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
          // T-118 r5: the map-1 march-first path FALLS THROUGH — the route
          // walk is the movement (the point-blank swing above rides the
          // existing path; a swing never cancels it).
          // T-118 r8c: the gauntlet maps march-first TOO. The r8b leg
          // stalled in the ossuary for 8+ minutes: with the racks respawning
          // a ghoul every 30 s three tiles from the column, "something is
          // always within 12" and this block's `continue` starved the route
          // machine so the runner's routeIdx never advanced — the column
          // ground in circles around (22,21) until the leg clock died.
          // March-first: the route walk IS the movement, the swing rides it.
          // (r12 let the point-blank swing fall through on maps 3/5; r14
          // restores bestD > 1 — with the runner's chase back, the r9
          // posture: stop-and-win the point-blank fight, march between.)
          if (!(raider && bestD > 1 &&
                (b.mapId == 1 || b.mapId == 3 || b.mapId == 5)))
            continue;
        }
        // nothing near: campaign/crypt/raider walks the route instead of milling
        if (campaign || crypt || raider) {
          // T-118 resupply: with multi-leg persistence the character resumes
          // AT CAMP, so `home` — and the nearTown economy — can no longer
          // see Marta. When the belt is thin (or gear is missing) and no
          // aggressive mob is within 6, walk to the vendor (wire kind 64)
          // and buy in gear-first order.
          if ((raider || partySize >= 3) && b.mapId == 1) {
            int vials = 0;
            for (const auto& kv : b.inv)
              if (kv.second.itemId == 3001) vials += kv.second.qty;
            std::uint32_t vendorId = 0;
            int vendorD = 99;
            for (const auto& kv : b.ents) {
              if (kv.second.kind != bh::content::kWireKindVendor) continue;
              const int d = std::max(std::abs(kv.second.x - b.tileX),
                                     std::abs(kv.second.y - b.tileY));
              if (d < vendorD) { vendorD = d; vendorId = kv.first; }
            }
            int threat = 0;
            for (const auto& kv : b.ents) {
              if (kv.second.kind == 0 ||
                  bh::content::wireIsFurniture(kv.second.kind))
                continue;
              if (!entAggressive(kv.second.kind)) continue;
              if (std::max(std::abs(kv.second.x - b.tileX),
                           std::abs(kv.second.y - b.tileY)) <= 6)
                ++threat;
            }
            const bool poor = vials < 6 || !hasBlade || !hasArmor;
            if (poor && threat == 0 && vendorId != 0) {
              if (vendorD <= 3 && t >= b.nextShopAt) {
                b.nextShopAt = t + 2.0;
                bh::proto::BuyRequest buy;
                bool bought = false;
                if (!hasBlade && b.gold >= 260) {
                  buy.itemId = 2002;  // Pit Blade
                  buy.qty = 1;
                  bought = true;
                } else if (hasBlade && !hasArmor && b.gold >= 120) {
                  buy.itemId = 2101;  // Hide Armor
                  buy.qty = 1;
                  bought = true;
                } else if (vials < 16 && b.gold >= 240) {
                  buy.itemId = 3001;
                  buy.qty = static_cast<std::uint32_t>(
                      std::min(16 - vials, static_cast<int>((b.gold - 90) / 30)));
                  bought = buy.qty > 0;  // 240g gate makes 0 unreachable; guard
                }
                if (bought) {
                  sendProto(b.peer, bh::proto::pack(buy));
                  ++b.shops;
                  b.nextMoveAt = t + 0.8;
                  continue;
                }
                // poor but short on gold: stop standing at the counter — fall
                // through to the route (fight for the coin, then return).
                b.nextMoveAt = t + 2.0;
              } else if (vendorD > 3) {
                bh::proto::InputPath ip;
                ip.goalX = b.ents[vendorId].x;
                ip.goalY = b.ents[vendorId].y;
                sendProto(b.peer, bh::proto::pack(ip));
                b.nextMoveAt = t + 1.2;
                continue;
              }
            }
          }
          // T-034d re-gear trip: walk home when the next gear tier is
          // affordable, instead of hitching shopping to death-respawn. The
          // economy block above does the actual buy/equip once nearTown.
          const bool wantGear = (!hasBlade && b.gold >= 260) ||
                                (bladeArmed && !hasArmor && b.gold >= 120);
          if (wantGear && (b.mapId == 1 || crypt) && b.homeX >= 0) {
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
          if (raider && b.campX >= 0) {
            // T-118 r8b: the gauntlet column. Exactly ONE brain — the
            // front runner (forward-most alive party member in view;
            // frontRunnerOf is the same deterministic pick on every bot)
            // runs this route machine. Every other gauntlet bot MILLS:
            // within 3 tiles of the runner it stands and fights with the
            // column; beyond 3 it paths back to the runner. Followers
            // NEVER run their own nodes — that was the r7/8a failure:
            // five nodes = five solo trades = 24 deaths, the barrow ring
            // met two deep. The column's pace is the runner's pace; its
            // DPS is the stack's. The combat/economy/choir blocks above
            // already ran this iteration, so a miller still swings at its
            // focus-fire mark, sips, and mends like any fighter.
            // A bot that BECOMES the runner (its predecessor fell) holds a
            // stale routeIdx — it never ran the machine. Adopt the node
            // nearest its own tile: it stands at the column's head, so
            // that IS the column's node (ties take the furthest — the
            // route only marches east).
            // T-118 r13: the column exists on map 1 TOO. r12 forensics:
            // the post-wipe return march ping-ponged at (24-30,14) for
            // minutes — five solo routeIdxes, fr election flipping every
            // overshoot, nobody ever reached the hatch. One brain per
            // zone, everywhere the raider routes.
            if (b.mapId == 1 || b.mapId == 3 || b.mapId == 5) {
              const FrontRunner fr = frontRunnerOf(b);
              if (fr.id != 0 && fr.id != b.ownId) {
                const int fd = std::max(std::abs(fr.x - b.tileX),
                                        std::abs(fr.y - b.tileY));
                if (fd > 3) {
                  bh::proto::InputPath ip;
                  ip.goalX = fr.x;
                  ip.goalY = fr.y;
                  sendProto(b.peer, bh::proto::pack(ip));
                  b.nextMoveAt = t + 1.2;
                } else {
                  b.nextMoveAt = t + 0.8;  // mill in place with the column
                }
                b.wasRunner = false;
                continue;
              }
              if (!b.wasRunner) {
                // NB raiderRoute CLAMPS out-of-range indices (holds the
                // last node) and returns the node count — loop the count,
                // never a sentinel break. (The unbounded variant livelocked
                // the whole bot at the first map-3 pass: no heartbeats,
                // the server dropped all five peers at login, online=0.)
                int ax0, ay0;
                double ar0;
                bool ap0;
                const int nn =
                    raiderRoute(b.mapId, 0, &ax0, &ay0, &ar0, &ap0);
                int best = 100000, bi = 0;
                for (int i = 0; i < nn; ++i) {
                  int ax, ay;
                  double ar;
                  bool ap;
                  raiderRoute(b.mapId, i, &ax, &ay, &ar, &ap);
                  const int d = std::max(std::abs(ax - b.tileX),
                                         std::abs(ay - b.tileY));
                  if (d <= best) {  // tie: furthest (east) node wins
                    best = d;
                    bi = i;
                  }
                }
                b.routeIdx = bi;
              }
              b.wasRunner = true;
            }
            // T-118 r3 staged push: walk the current node; on arrival hold
            // for its rest (group + sip + mend), then advance. Portal nodes
            // re-path to the exact tile until the fire check trips.
            const int nd = std::max(std::abs(b.tileX - b.campX),
                                    std::abs(b.tileY - b.campY));
            if (nd <= 2) {
              if (b.campIsPortal) {
                // T-118 r8b: the depths stairs (map 3) are GUARDED — the
                // barrow ring (Cantor Vex's bolt reaches 6, the sexton
                // seat is on the portal's own tile row) leashes onto the
                // rect. The runner only settles the walker on it once the
                // ring is dead (or 60 s have passed — progress over
                // perfection); the column mills within 3 tiles and
                // focus-fires the ring meanwhile. Map 1/2 portal
                // crossings keep the old instant settle.
                if ((b.mapId == 3) && b.bestDDebug <= 8) {
                  if (b.portalWaitT0 == 0.0) b.portalWaitT0 = t;
                  if (t - b.portalWaitT0 < 60.0) {
                    b.nextMoveAt = t + 0.8;  // mill, fight the ring
                    continue;
                  }
                }
                b.portalWaitT0 = 0.0;
                // T-118 r8e: the map-1 hatch (10,10) EXITS the crypt at
                // (24,30) — inside the widow cocoon. The probe-2 leg's 01
                // doom-looped on it: T-056 respawn in town -> map-1 route
                // (32,16)/(14,14)/(10,10) -> hatch -> cocoon -> death ->
                // town, five times in 90 s. The crossing IS a fight, so
                // the runner only settles the hatch when 3+ of the stack
                // are here; the 45 s cap crosses at q>=2 (a pair at least)
                // and a lone bot waits it out at the chapel — it would
                // just feed the cocoon.
                if (b.mapId == 1 && b.quorumWaitT0 == 0.0) {
                  int q = 0;
                  for (const auto& kv : b.party) {
                    if (kv.second.hp == 0) continue;
                    const auto ei = b.ents.find(kv.first);
                    if (ei == b.ents.end()) continue;
                    const int d = std::max(std::abs(ei->second.x - b.campX),
                                           std::abs(ei->second.y - b.campY));
                    if (d <= 6) ++q;
                  }
                  if (q < 3) b.quorumWaitT0 = t;
                }
                if (b.mapId == 1 && b.quorumWaitT0 != 0.0) {
                  if (t - b.quorumWaitT0 < 45.0) {
                    b.nextMoveAt = t + 0.8;  // mill at the chapel edge
                    continue;
                  }
                  const int q2 = quorumNear(b, b.campX, b.campY, 6);
                  if (q2 >= 3) {
                    b.quorumWaitT0 = 0.0;  // T-118 r13: cross as a STACK.
                    // The r8e pair-cap let duos/solos through; the r12
                    // leg's solo re-crosser died in the cocoon corridor
                    // (22,24), swarm 8. Under 3 after the cap: fall back
                    // to town and re-converge, like the solo rule.
                  } else if (q2 < 3) {
                    // T-118 r8f: solo after the cap — the r8e4 looped bot
                    // sat the chapel 130 s, lost to the party. Walk back
                    // to town (node 0, rest 3 s, safe floor); the ~90 s
                    // round trip is the cooldown, and when the column
                    // converges the quorum assembles before the crossing.
                    b.quorumWaitT0 = 0.0;
                    b.routeIdx = 0;
                    b.nextMoveAt = t + 0.8;
                    continue;
                  }
                }
                // portal node: re-path the exact tile until the fire check
                // settles the walker on the rect
                bh::proto::InputPath ip;
                ip.goalX = b.campX;
                ip.goalY = b.campY;
                sendProto(b.peer, bh::proto::pack(ip));
                b.nextMoveAt = t + 1.2;
              } else if (b.routeRest > 0.0) {
                // rest node: stand — the party converges, sips, mends —
                // then advance
                if (b.routeRestUntil == 0.0) b.routeRestUntil = t + b.routeRest;
                if (t >= b.routeRestUntil) {
                  b.routeRestUntil = 0.0;
                  ++b.routeIdx;
                }
              } else {
                // T-118 r4 waypoint: the gauntlet push never stops mid-room.
                // T-118 r8d: unless the column is not assembled. The r8c
                // leg split the stack — the laggards ground through the
                // racks' aggro chain at ~0.3 tiles/s while the runner
                // sprinted the nodes ahead of them, so the barrow ring
                // (Cantor bolt r6 + leashes) met the runner solo and
                // picked the string off one by one. Gate: at a gauntlet
                // waypoint the runner holds until 3+ living party members
                // are within 6 tiles of the node; capped at 45 s (the
                // laggards DO arrive, just slowly — progress over
                // perfection).
                // T-118 r9: the unbroken pass. The (22,21) node sits inside
                // BOTH leash fields (racks leash 14 + cocoon leash 10):
                // every quorum hold there turned into the double-leash pile
                // that cost 2-3 of 5 in every full r8 leg (r8e4/r8f/r8g).
                // No wait at the pile node — the runner advances on arrival
                // and the stack crosses the overlap rect in ~10-15 s, before
                // the 30 s respawn can reform the swarm. The r4 design
                // intent, restored now that the column actually holds
                // together (election + holdStack + stack retreat).
                // T-118 r14: the r8d quorum gate returns (r9 won its map-5
                // entries WITH these holds) — the r13 unbroken march
                // out-sprinted its own kills and dragged the entire
                // double-leash field into a swarm-10 convergence at
                // (32,18). Hold the waypoints, win the fights, advance.
                // The pile node stays wait-IMMUNE (r12): drop any
                // residual wait and go — waiting on the overlap rect is
                // how every r8 leg lost 2-3 of 5.
                const bool pileNode =
                    b.mapId == 3 && b.campX == 22 && b.campY == 21;
                if (pileNode) {
                  b.quorumWaitT0 = 0.0;
                } else {
                  if ((b.mapId == 3 || b.mapId == 5) &&
                      b.quorumWaitT0 == 0.0) {
                    if (quorumNear(b, b.campX, b.campY, 6) < 3)
                      b.quorumWaitT0 = t;
                  }
                  if (b.quorumWaitT0 != 0.0) {
                    if (t - b.quorumWaitT0 < 45.0) {
                      b.nextMoveAt = t + 0.8;  // mill in place: column forms
                      continue;
                    }
                    b.quorumWaitT0 = 0.0;  // cap: advance anyway
                  }
                }
                ++b.routeIdx;
              }
            } else {
              b.routeRestUntil = 0.0;  // interrupted: re-rest on arrival
              bh::proto::InputPath ip;
              ip.goalX = b.campX;
              ip.goalY = b.campY;
              sendProto(b.peer, bh::proto::pack(ip));
              b.nextMoveAt = t + 1.2;
            }
            continue;
          }
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
      } else if (profile != "wander" && profile != "pilgrim" && profile != "crypt" && profile != "crypt_party") {
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
  std::uint64_t bossSeen = 0, bossKills = 0, curseSeen = 0, slamSeen = 0;
  for (const Bot& b : bots) { bossSeen += b.bossSeen; bossKills += b.bossKills; curseSeen += b.curseSeen; slamSeen += b.slamSeen; }
  std::printf("[bots] CRYPT bossSeen=%llu bossKills=%llu curse=%llu slam=%llu\n", (unsigned long long)bossSeen, (unsigned long long)bossKills, (unsigned long long)curseSeen, (unsigned long long)slamSeen);
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
    std::printf("[bots] %-12s kit=%d lvl=%d hp=%d/%d party=%zu deaths=%llu rxR=%llu rxM=%llu lastPid=%u casts c6=%llu c7=%llu c8=%llu bossSeen=%llu bossKills=%llu curse=%llu slam=%llu lastDeath=(%d,%d) killerByLvl=",
                b.name.c_str(), b.kitClass, b.level, static_cast<int>(b.hp),
                static_cast<int>(b.hpMax), b.party.size(),
                static_cast<unsigned long long>(b.deaths),
                static_cast<unsigned long long>(b.rcvReset),
                static_cast<unsigned long long>(b.rcvMember), b.lastResetPid,
                static_cast<unsigned long long>(b.chorusCasts),
                static_cast<unsigned long long>(b.massCasts),
                static_cast<unsigned long long>(b.hasteCasts),
                static_cast<unsigned long long>(b.bossSeen),
                static_cast<unsigned long long>(b.bossKills),
                static_cast<unsigned long long>(b.curseSeen),
                static_cast<unsigned long long>(b.slamSeen),
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
    if (profile == "raider") {  // T-118 gate line, one per bot (greppable)
      // Four DISTINCT buffers — a shared one would alias the %s args and
      // print the last value for every field.
      char tm5[32], tbs[32], tbk[32], ttk[32];
      auto fmtT = [](char* buf, size_t n, double v) {
        std::snprintf(buf, n, v < 0.0 ? "n/a" : "%.1f", v);
      };
      const double ttkS = (b.tBossSight >= 0.0 && b.tBossKilled >= 0.0)
                              ? b.tBossKilled - b.tBossSight
                              : -1.0;
      fmtT(tm5, sizeof tm5, b.tMap5Entry);
      fmtT(tbs, sizeof tbs, b.tBossSight);
      fmtT(tbk, sizeof tbk, b.tBossKilled);
      fmtT(ttk, sizeof ttk, ttkS);
      std::printf("[raid] %-12s kit=%d party=%zu deaths=%llu dodges=%llu "
                  "elites=%llu trash=%llu lvl=%d xp=%u gold=%u map5=%s "
                  "boss_sight=%s boss_kill=%s TTK=%s\n",
                  b.name.c_str(), b.kitClass, b.party.size(),
                  static_cast<unsigned long long>(b.deaths),
                  static_cast<unsigned long long>(b.dodges),
                  static_cast<unsigned long long>(b.eliteKills),
                  static_cast<unsigned long long>(b.trashKills),
                  b.level, b.xp, b.gold, tm5, tbs, tbk, ttk);
    }
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
