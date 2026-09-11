#include "content/kits.h"
#include "content/mobs.h"

#include "world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "content/auras.h"
#include "content/wirekind.h"
#include "sim/clock.h"
#include "sim/combat.h"

namespace bh::server {

namespace {
constexpr sim::Tick kPlayerAtkCdTicks = 16;   // 800 ms deliberate era swing
constexpr sim::Tick kPlayerRespawnTicks = 60; // 3 s walk of shame (debt lands in T-021)
constexpr sim::Tick kAggroThinkPeriod = 5;    // staggered think (id % period)
constexpr sim::Tick kOocRegenDelay = 400;     // 20 s out of combat before regen
constexpr sim::Tick kOocRegenPeriod = 40;     // +1 hp per 2 s
constexpr std::uint32_t kFistsBaseDmg = 8;    // proto-Ravager bare hands
constexpr sim::Tick kSipCdTicks = 10;         // GDD: 500 ms global sip
constexpr sim::Tick kPowerSwingCdTicks = 40;  // GDD: 40t CD
constexpr std::uint32_t kPowerSwingMultPct = 140;

// ---- kit skills (T-054, GDD Cultist/Gravecaller v1 numbers) --------------
constexpr sim::Tick kBlessTicks = 6000;      // buffs last 5 min
constexpr sim::Tick kCurseTicks = 600;       // T-070 Blood Curse: 30 s of thin blood
// T-091 Gravemother telegraphed slam: 60t wind-up the client stages 1-2-3
// (decal law), then radius-2 ground rot around the recorded tile. Numbers
// derive from the bolt path she replaces (same dmg stat, same curse lane).
constexpr sim::Tick kSlamWindupTicks = 60;
constexpr int kSlamRadius = 2;
constexpr sim::Tick kMendCdTicks = 25;
constexpr sim::Tick kBlessCdTicks = 40;
constexpr sim::Tick kIronskinCdTicks = 40;
constexpr sim::Tick kFireboltCdTicks = 30;
constexpr int kMendRange = 6;                // tiles, party-only target

// ---- kit skills v2 (T-054b) ----------------------------------------------
constexpr sim::Tick kChorusTicks = 2400;    // song lasts 2 min
constexpr sim::Tick kChorusCdTicks = 240;   // 12s between verses
constexpr sim::Tick kMassMendCdTicks = 60;
constexpr sim::Tick kHasteTicks = 1200;  // 60s of fast steel
constexpr sim::Tick kHasteCdTicks = 400;
constexpr std::uint32_t kChorusMpCost = 14;
constexpr std::uint32_t kMassMendMpCost = 18;
constexpr std::uint32_t kHasteMpCost = 10;
constexpr int kFireboltRange = 8;
constexpr std::uint32_t kMendMpCost = 8;
constexpr std::uint32_t kBlessMpCost = 15;
constexpr std::uint32_t kIronskinMpCost = 15;
constexpr std::uint32_t kFireboltMpCost = 6;
constexpr std::uint32_t kSkillLandsPerPoint = 25;  // Soma: skill up by use

int chebyshev(const sim::TilePos a, const sim::TilePos b) {
  return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

// T-080 trade transaction log path (default; tests override per-file).
static std::string gTradeLogPath = "logs/trades.log";
const std::string& tradeLogPath() { return gTradeLogPath; }
}  // namespace

void World::setTradeLogPath(const std::string& p) { gTradeLogPath = p; }

bool World::load(const std::string& mapPath, std::string* err) {
  auto m = sim::loadBhmap(mapPath, err);
  if (!m) return false;
  return loadFrom(std::move(*m));
}

bool World::loadZone(std::uint16_t mapId, const std::string& mapPath,
                     std::string* err) {
  auto m = sim::loadBhmap(mapPath, err);
  if (!m) return false;
  return loadZoneFrom(mapId, std::move(*m));
}

const sim::Map* World::zoneMap(std::uint16_t mapId) const {
  auto it = zones_.find(mapId);
  return it == zones_.end() ? nullptr : &it->second.map;
}

bool World::loadFrom(sim::Map map) {  // zone 1 (tests + primary)
  return loadZoneFrom(1, std::move(map));
}

bool World::loadZoneFrom(std::uint16_t mapId, sim::Map map) {
  Zone z;
  z.map = std::move(map);
  z.grid = z.map.costGrid();

  const int cx = z.map.w / 2;
  const int cy = z.map.h / 3;
  for (int r = 0; r < 64; ++r) {
    for (int dy = -r; dy <= r; ++dy) {
      for (int dx = -r; dx <= r; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
        const int x = cx + dx;
        const int y = cy + dy;
        if (z.map.inBounds(x, y) && !z.map.isBlocked(x, y)) {
          z.spawnPoint = sim::TilePos{x, y};
          goto done;
        }
      }
    }
  }
done:
  for (const sim::SpawnDef& d : z.map.spawners) {
    SpawnerLive sl;
    sl.def = d;
    sl.respawnReadyAt = 0;
    z.spawners.push_back(sl);
  }
  zones_.emplace(mapId, std::move(z));
  Zone& zone = zones_.at(mapId);
  if (mapId == 1) spawnVendor(zone);
  if (mapId == 1) spawnConfessor(zone);  // T-070: the chapel cure
  initialMobSpawns(zone, mapId);
  if (mapId == 1 || mapId == 3) spawnAnvils();   // plaza + bone barrow
  if (mapId == 1) spawnNpcs();  // T-094: twins + post guards (after anvil)
  return true;
}

Entity& World::insertEntity(Entity e) {
  const std::uint16_t zone = e.zoneId;
  entities_.push_back(std::move(e));
  Entity& ref = entities_.back();
  const sim::TilePos p = ref.walker.tile();
  zones_.at(zone).spatial.insert(ref.id, p.x, p.y);
  return ref;
}

Entity& World::spawn(const std::string& name, std::int64_t charRowId,
                     std::optional<sim::TilePos> at, std::uint16_t zoneId,
                     std::uint8_t classId) {
  if (zones_.count(zoneId) == 0) zoneId = 1;  // zoneless test maps fall back
  Zone& z = zones_.at(zoneId);
  Entity e;
  e.id = nextId_++;
  e.kind = EntityKind::kPlayer;
  e.name = name;
  e.charRowId = charRowId;
  e.zoneId = zoneId;
  e.classId = classId;
  if (const content::KitDef* kit = content::findKit(classId)) {  // T-053 seed
    e.str = kit->str;
    e.vit = kit->vit;
    e.dex = kit->dex;
    e.intg = kit->intg;
    e.mag = kit->mag;
  }
  sim::TilePos pos = z.spawnPoint;
  if (at && z.map.inBounds(at->x, at->y) && !z.map.isBlocked(at->x, at->y)) pos = *at;
  e.walker.place(pos);
  e.hpMax = recomputeHpMax(e);
  e.hp = e.hpMax;
  e.mpMax = 30;  // L1 pool; recomputeMpMax at level-up
  e.mp = e.mpMax;
  // T-073 M4: the newborn walks 5 s unseen by mob lookup (100 ticks).
  e.spawnProtectUntil = tick_ + 100;
  return insertEntity(std::move(e));
}

// T-053: one-time kit swear. Unsworn (0) at any level, or sworn at level 1
// (early re-dedication window, era monastery trope). Emits a choir line so
// the choice is seen — oath-fiction, not a silent stat swap.
bool World::kitChoose(Entity& e, std::uint8_t kitId) {
  if (e.kind != EntityKind::kPlayer || e.dead) return false;
  const content::KitDef* kit = content::findKit(kitId);
  if (kit == nullptr) return false;
  const bool sworn = e.classId != content::kKitUnsworn;
  if (sworn && e.level > 1) return false;  // past the window
  e.classId = kitId;
  e.str = kit->str + (e.str > 8 ? e.str - 8 : 0);    // keep assigned pts
  e.vit = kit->vit + (e.vit > 8 ? e.vit - 8 : 0);
  e.dex = kit->dex + (e.dex > 8 ? e.dex - 8 : 0);
  e.intg = kit->intg;
  e.mag = kit->mag;
  {
    const std::uint32_t newMax = recomputeHpMax(e);
    e.hpMax = newMax;
    if (e.hp > newMax) e.hp = newMax;
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.statsChanged = true;
    ev.chatCh = 2;
    ev.chatText = sworn ? (e.name + " re-swears to the " + kit->name + ".")
                        : (e.name + " swears the oath of the " + kit->name + ".");
    events_.push_back(std::move(ev));
  }
  return true;
}

Entity& World::spawnMob(const content::MobDef& def, sim::TilePos at, size_t spawnerIdx,
                        std::uint16_t zoneId) {
  Entity e;
  e.id = nextId_++;
  e.kind = EntityKind::kMob;
  e.zoneId = zoneId;
  {
    int idx = 1;
    for (const content::MobDef& d : content::kMobs) {
      if (d.mobId == def.mobId) {
        e.wireKind = static_cast<std::uint8_t>(idx);
        break;
      }
      ++idx;
    }
  }
  e.name = def.name;
  e.mobId = def.mobId;
  e.mobLevel = def.level;
  e.hp = def.hp;
  e.hpMax = def.hp;
  e.anchor = at;
  e.aggroRadius = def.aggroRadius;
  e.wanderRadius = def.wanderRadius;
  e.leashRadius = def.leashRadius;
  e.atkCdTicks = def.atkCdTicks;
  e.xpValue = def.xp;
  e.dex = def.dex;
  // mobs store their flat damage as str=0/base via xpValue; raw dmg is def.dmg
  e.walker.place(at);
  Entity& ref = insertEntity(std::move(e));
  ref.spawnerIdx = spawnerIdx;
  return ref;
}

void World::initialMobSpawns(Zone& zone, std::uint16_t zoneId) {
  for (size_t i = 0; i < zone.spawners.size(); ++i) {
    SpawnerLive& sl = zone.spawners[i];
    // T-071: night-bound spawners stay empty by day (boot is 08:00).
    if (sl.def.nightOnly != 0 && !isNight()) continue;
    const content::MobDef* def = content::findMob(sl.def.mobId);
    if (def == nullptr) {
      std::fprintf(stderr, "[world] spawner mobId %u has no def, skipped\n", sl.def.mobId);
      continue;
    }
    for (std::uint32_t n = 0; n < sl.def.maxAlive; ++n) {
      // scatter inside the spawner rect
      const int x = sl.def.x + static_cast<int>(rng_.range(0, sl.def.w - 1));
      const int y = sl.def.y + static_cast<int>(rng_.range(0, sl.def.h - 1));
      if (zone.map.inBounds(x, y) && !zone.map.isBlocked(x, y)) {
        spawnMob(*def, sim::TilePos{x, y}, i, zoneId);
      } else {
        --n;  // blocked tile: re-roll (bounded implicitly by walkable map design)
        if (n > 40) break;
      }
    }
  }
}

void World::despawn(std::uint32_t id) {
  if (Entity* e = find(id)) {
    if (e->duelWith != 0) {  // T-056: logout/forfeit-by-absence frees the mark
      if (Entity* t = find(e->duelWith)) { t->duelWith = 0; t->duelUntil = -1; }
      e->duelWith = 0;
      e->duelUntil = -1;
    }
  }
  for (size_t i = 0; i < entities_.size(); ++i) {
    if (entities_[i].id == id) {
      Entity& e = entities_[i];
      if (e.partyId != 0) {
        // leaving silently: same roster semantics live and under replay
        const std::uint32_t pid = e.partyId;
        e.partyId = 0;
        for (size_t pi = 0; pi < parties_.size(); ++pi) {
          Party& p = parties_[pi];
          if (p.id != pid) continue;
          p.members.erase(std::remove(p.members.begin(), p.members.end(), id),
                          p.members.end());
          if (p.members.empty()) parties_.erase(parties_.begin() + static_cast<long>(pi));
          else if (p.leaderId == id) p.leaderId = p.members.front();
          break;
        }
        emitPartyMsg(pid, id, e.name + " is gone.");  // roster refresh trigger
      }
      zones_.at(entities_[i].zoneId).spatial.remove(id);
      entities_.erase(entities_.begin() + static_cast<long>(i));
      return;
    }
  }
}

Entity* World::find(std::uint32_t id) {
  for (auto& e : entities_) {
    if (e.id == id) return &e;
  }
  return nullptr;
}

const Entity* World::find(std::uint32_t id) const {
  for (const auto& e : entities_) {
    if (e.id == id) return &e;
  }
  return nullptr;
}

void World::queuePath(Entity& e, sim::TilePos goal) {
  if (e.dead) return;
  e.attackTarget = 0;  // moving cancels attacking (era rule)
  const Zone& z = zoneOf(e);
  if (!z.map.inBounds(goal.x, goal.y) || z.map.isBlocked(goal.x, goal.y)) return;
  const sim::TilePos start = e.walker.moving ? e.walker.target : e.walker.tile();
  if (start == goal) {
    e.path.clear();
    return;
  }
  const sim::PathResult res = sim::findPath(z.grid, start, goal);
  if (!res.found) return;
  e.path.assign(res.tiles.begin(), res.tiles.end());
}

void World::setAttack(Entity& self, std::uint32_t targetId) {
  if (self.dead || self.id == targetId) return;
  Entity* target = find(targetId);
  if (target == nullptr || target->dead || content::wireIsFurniture(target->wireKind)) return;
  if (target->zoneId != self.zoneId) return;  // cross-zone targeting impossible
  self.attackTarget = targetId;
  self.path.clear();
}

bool World::assignStat(Entity& e, std::uint8_t stat) {
  if (e.kind != EntityKind::kPlayer || e.statPoints == 0 || stat > 2) return false;
  switch (stat) {
    case 0: ++e.str; break;
    case 1: ++e.vit; break;
    default: ++e.dex; break;
  }
  --e.statPoints;
  const std::uint32_t newMax = recomputeHpMax(e);
  e.hp += newMax - e.hpMax;  // VIT bumps heal by the delta (era QoL)
  e.hpMax = newMax;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.statsChanged = true;
  events_.push_back(std::move(ev));
  return true;
}

std::uint32_t World::recomputeHpMax(Entity& e) const {
  return sim::playerHpMax(e.level, e.vit);
}

std::vector<Entity*> World::playersNear(Zone& zone, int x, int y, int radius) {
  std::vector<Entity*> out;
  const auto ids = zone.spatial.query(x - radius, y - radius, x + radius, y + radius);
  for (const std::uint32_t id : ids) {
    Entity* e = find(id);
    if (e != nullptr && e->kind == EntityKind::kPlayer && !e->dead &&
        chebyshev(e->walker.tile(), sim::TilePos{x, y}) <= radius) {
      out.push_back(e);
    }
  }
  return out;
}

void World::spawnVendor() { spawnVendor(zones_.at(1)); }

void World::spawnVendor(Zone& zone) {
  if (zone.vendorSeeded) return;
  zone.vendorSeeded = true;
  // Marta stands near the town spawn: FIRST walkable tile in a tiny spiral.
  // T-069 fix: the loop lost its break at some point and seeded EVERY
  // walkable ring tile — boot logs carried ~425 duplicate Martas (entity
  // bloat, and no free tile left in town for any new furniture). One Marta,
  // as T-027 documented.
  bool martaseeded = false;
  for (int r = 1; r < 8 && !martaseeded; ++r) {
    for (int dy = -r; dy <= r && !martaseeded; ++dy) {
      for (int dx = -r; dx <= r && !martaseeded; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
        const int x = zone.spawnPoint.x + dx;
        const int y = zone.spawnPoint.y + dy;
        if (!zone.map.inBounds(x, y) || zone.map.isBlocked(x, y)) continue;
        Entity e;
        e.id = nextId_++;
        e.zoneId = 1;
        e.kind = EntityKind::kMob;  // immobile, non-combat; wire kind marks vendor
        e.wireKind = 64;
        e.name = "Marta";
        e.hp = 1;
        e.hpMax = 1;
        e.dead = false;
        e.walker.place(sim::TilePos{x, y});
        insertEntity(std::move(e));
        martaseeded = true;  // T-069: one Marta (see comment above)
      }
    }
  }
  // T-065 Wanted Board: second spiral from the same spawn, skipping any
  // furniture tile. Session-scoped (vanishes at reboot by design).
  for (int r = 1; r < 10; ++r) {
    for (int dy = -r; dy <= r; ++dy) {
      for (int dx = -r; dx <= r; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
        const int x = zone.spawnPoint.x + dx;
        const int y = zone.spawnPoint.y + dy;
        if (!zone.map.inBounds(x, y) || zone.map.isBlocked(x, y)) continue;
        bool occupied = false;
        for (const Entity& ee : entities_)
          if (ee.zoneId == 1 && !ee.dead && ee.walker.tile().x == x &&
              ee.walker.tile().y == y &&
              ee.wireKind >= content::kWireKindFurnitureFloor) { occupied = true; break; }
        if (occupied) continue;
        {
          Entity b;
          b.id = nextId_++;
          b.zoneId = 1;  // spawnVendor is zone 1 only today
          b.kind = EntityKind::kMob;  // furniture, non-combat; wire kind 66
          b.wireKind = content::kWireKindBounty;
          b.name = "Wanted Board";
          b.hp = 1;
          b.hpMax = 1;
          b.dead = false;
          b.walker.place(sim::TilePos{x, y});
          insertEntity(std::move(b));
        }
        spawnFence(zone);
        return;
      }
    }
  }
  // Board found no tile (town fully built over): the fence still seeds —
  // her spiral is independent of the board search above.
  spawnFence(zone);
}

// T-069: Sable the Fence at the gallows pit (the chaotic bindstone).
// Own spiral from gallowsTile(1), skipping occupied furniture tiles.
// Era: Sable works the crowd the temple refuses; Marta stays at the plaza.
void World::spawnFence(Zone& zone) {
  const sim::TilePos gp = gallowsTile(1);
  for (int r = 0; r < 4; ++r) {
    bool placed = false;
    for (int dy = -r; dy <= r && !placed; ++dy) {
      for (int dx = -r; dx <= r && !placed; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
        const int fx = gp.x + dx, fy = gp.y + dy;
        if (!zone.map.inBounds(fx, fy) || zone.map.isBlocked(fx, fy)) continue;
        bool occupied = false;
        for (const Entity& ee : entities_)
          if (ee.zoneId == 1 && !ee.dead && ee.walker.tile().x == fx &&
              ee.walker.tile().y == fy &&
              ee.wireKind >= content::kWireKindFurnitureFloor) {
            occupied = true;
            break;
          }
        if (occupied) continue;
        Entity f;
        f.id = nextId_++;
        f.zoneId = 1;
        f.kind = EntityKind::kMob;  // furniture, non-combat; wire kind 69
        f.wireKind = content::kWireKindFence;
        f.name = "Sable the Fence";
        f.hp = 1;
        f.hpMax = 1;
        f.dead = false;
        f.walker.place(sim::TilePos{fx, fy});
        insertEntity(std::move(f));
        placed = true;
      }
    }
    if (placed) return;
  }
}

// T-094 Thornwall NPC posts (derived positions, flagged for art review):
// the Bonesmith twins flank the Widow Anvil (brief: hammer sister + tongs
// sister, never desynced — two adjacent tiles), the Ashen guard stands the
// east-gate post and the Synod guard the bridge post (the T-073 posts they
// belong to). Spiral scans skipping occupied furniture, deterministic (no
// RNG — same class as the confessor scan). Registrar (72) and steward (73)
// stay unplaced: Marrowgate and the Weeping Castle do not exist yet.
void World::spawnNpcs() {
  auto zit = zones_.find(1);
  if (zit == zones_.end()) return;
  Zone& zone = zit->second;
  auto freeTile = [&](int x, int y) {
    if (!zone.map.inBounds(x, y) || zone.map.isBlocked(x, y)) return false;
    for (const Entity& ee : entities_)
      if (ee.zoneId == 1 && !ee.dead && ee.walker.tile().x == x &&
          ee.walker.tile().y == y &&
          ee.wireKind >= content::kWireKindFurnitureFloor)
        return false;
    return true;
  };
  auto place = [&](std::uint8_t kind, const char* name, int x, int y) {
    Entity f;
    f.id = nextId_++;
    f.zoneId = 1;
    f.kind = EntityKind::kMob;  // furniture, non-combat
    f.wireKind = kind;
    f.name = name;
    f.hp = 1;
    f.hpMax = 1;
    f.dead = false;
    f.walker.place(sim::TilePos{x, y});
    insertEntity(std::move(f));
  };
  // twins: first two free tiles spiralling from the anvil's tile
  sim::TilePos anvilAt{-1, -1};
  for (const Entity& e : entities_)
    if (e.wireKind == content::kWireKindAnvil && e.zoneId == 1 && !e.dead) {
      anvilAt = e.walker.tile();
      break;
    }
  int twins = 0;
  if (anvilAt.x >= 0) {
    for (int r = 1; r < 6 && twins < 2; ++r)
      for (int dy = -r; dy <= r && twins < 2; ++dy)
        for (int dx = -r; dx <= r && twins < 2; ++dx) {
          if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
          if (!freeTile(anvilAt.x + dx, anvilAt.y + dy)) continue;
          place(content::kWireKindBonesmith, "Bonesmith Twin", anvilAt.x + dx,
                anvilAt.y + dy);
          ++twins;
        }
  }
  // post guards: spiral from the T-073 spawner anchors
  const std::pair<std::uint8_t, const char*> posts[] = {
      {content::kWireKindGuardAshen, "Ashen Guard"},
      {content::kWireKindGuardSynod, "Synod Guard"},
  };
  const sim::TilePos anchors[] = {sim::TilePos{60, 13}, sim::TilePos{13, 31}};
  for (size_t i = 0; i < 2; ++i) {
    for (int r = 0; r < 4; ++r) {
      bool placed = false;
      for (int dy = -r; dy <= r && !placed; ++dy)
        for (int dx = -r; dx <= r && !placed; ++dx) {
          if (std::max(std::abs(dx), std::abs(dy)) != r) continue;
          if (!freeTile(anchors[i].x + dx, anchors[i].y + dy)) continue;
          place(posts[i].first, posts[i].second, anchors[i].x + dx,
                anchors[i].y + dy);
          placed = true;
        }
      if (placed) break;
    }
  }
}

// T-049x: the single inv-blob grammar (see world.h). Sequential field split —
// the pre-fix live login used an rfind(':') 3-field heuristic that only worked
// for legacy records: on v5+ blobs it read qty across colons (stoul stops at
// the ':' so qty survived by luck) and then took the LAST field as the
// equipped flag, laundering aura/durability/affix/refine and dropping
// equipped. Replay had its own 4-field sscanf + debugGive lane. Both are
// replaced by this one parser.
void parseInvBlob(const std::string& blob, std::vector<InvSlot>& out) {
  size_t pos = 0;
  while (pos < blob.size()) {
    const size_t end = blob.find(';', pos);
    const std::string rec =
        blob.substr(pos, end == std::string::npos ? end : end - pos);
    pos = end == std::string::npos ? blob.size() : end + 1;
    // split into at most 7 colon-fields
    std::string f[7];
    int nf = 0;
    size_t p = 0;
    while (nf < 7) {
      const size_t c = rec.find(':', p);
      if (c == std::string::npos) {
        f[nf++] = rec.substr(p);
        break;
      }
      f[nf++] = rec.substr(p, c - p);
      p = c + 1;
    }
    if (nf < 3) continue;  // need at least iid:qty:equipped
    auto num = [](const std::string& s, unsigned* out) -> bool {
      if (s.empty()) return false;
      for (const char c : s)
        if (c < '0' || c > '9') return false;
      *out = static_cast<unsigned>(std::stoul(s));
      return true;
    };
    InvSlot sl;
    unsigned v = 0;
    if (!num(f[0], &v)) continue;
    sl.itemId = static_cast<std::uint32_t>(v);
    if (!num(f[1], &v)) continue;
    sl.qty = static_cast<std::uint16_t>(v);
    sl.equipped = f[2] == "1";
    if (nf > 3) { if (!num(f[3], &v)) continue; sl.aura = static_cast<std::uint8_t>(v); }
    if (nf > 4) { if (!num(f[4], &v)) continue; sl.durability = static_cast<std::uint8_t>(v); }
    if (nf > 5) { if (!num(f[5], &v)) continue; sl.affix = static_cast<std::uint8_t>(v); }
    if (nf > 6) { if (!num(f[6], &v)) continue; sl.refine = static_cast<std::uint8_t>(v); }
    out.push_back(sl);
  }
}

std::string canonicalInvBlob(const std::vector<InvSlot>& inv) {
  std::string blob;
  for (const InvSlot& sl : inv) {
    blob += std::to_string(sl.itemId) + ":" + std::to_string(sl.qty) + ":" +
            (sl.equipped ? "1" : "0") + ":" + std::to_string(sl.aura) + ":" +
            std::to_string(sl.durability) + ":" + std::to_string(sl.affix) +
            ":" + std::to_string(sl.refine) + ";";
  }
  return blob;
}

bool World::addItem(Entity& e, std::uint32_t itemId, std::uint16_t qty) {
  const content::ItemDef* d = content::findItem(itemId);
  if (d == nullptr || qty == 0 || e.inv.size() >= 32) return false;
  // stack into an existing un-equipped stack
  if (d->stackMax > 1) {
    for (InvSlot& sl : e.inv) {
      if (sl.itemId == itemId && !sl.equipped && sl.qty + qty <= d->stackMax) {
        sl.qty = static_cast<std::uint16_t>(sl.qty + qty);
        return true;
      }
    }
  }
  InvSlot sl;
  sl.itemId = itemId;
  sl.qty = qty;
  sl.equipped = false;
  e.inv.push_back(sl);
  return true;
}

bool World::isNight() const {
  const float h = sim::hourAt(tick_);
  return h >= 21.0f || h < 5.0f;  // dark hours; legibility floor caps the look
}

std::uint32_t World::equippedWeaponDmg(const Entity& e) const {
  for (const InvSlot& sl : e.inv) {
    if (sl.equipped) {
      const content::ItemDef* d = content::findItem(sl.itemId);
      if (d != nullptr && d->slot == 0) {
        if (sl.durability == 0) return kFistsBaseDmg;  // dormant (T-058)
        std::uint32_t dmg = d->dmg + 2u * sl.refine;  // T-060 refine steps
        if (sl.affix == 1) dmg = dmg + dmg / 10;      // of Whet (T-059): +10%
        if (sl.aura >= 1) {  // Edge Rite (tier I): flat attack bleed
          if (const content::AuraTier* t = content::findAuraTier(sl.aura))
            dmg += t->atkBonusFlat;
        }
        return dmg;
      }
    }
  }
  return kFistsBaseDmg;
}

std::uint8_t World::equippedGlowTier(const Entity& e) const {
  if (e.kind != EntityKind::kPlayer) return 0;  // mobs carry no gear
  for (const InvSlot& sl : e.inv) {
    if (sl.equipped) {
      const content::ItemDef* d = content::findItem(sl.itemId);
      if (d != nullptr && d->slot == 0) {
        if (sl.durability == 0) return 0;  // dormant never glows (T-058)
        if (sl.refine >= 10) return 2;     // mythic silhouette (T-ART-11)
        if (sl.refine >= 5) return 1;      // glow (GDD: from +5)
        return 0;
      }
    }
  }
  return 0;  // fists: no steel, no glow
}

std::uint32_t World::equippedArmorDef(const Entity& e) const {
  for (const InvSlot& sl : e.inv) {
    if (sl.equipped) {
      const content::ItemDef* d = content::findItem(sl.itemId);
      if (d != nullptr && d->slot == 1) {
        if (sl.durability == 0) return 0u;  // dormant (T-058)
        const std::uint32_t def = d->def + sl.refine;  // T-060 refine steps
        return sl.affix == 2 ? def + 2u : def;         // of Warding (T-059)
      }
    }
  }
  return 0;
}

bool World::useItem(Entity& e, std::uint8_t slot) {
  if (e.kind != EntityKind::kPlayer || e.dead || slot >= e.inv.size()) return false;
  if (tick_ - e.lastSipTick < kSipCdTicks) return false;
  InvSlot& sl = e.inv[slot];
  const content::ItemDef* d = content::findItem(sl.itemId);
  if (d == nullptr || d->slot != 2 || sl.qty == 0) return false;
  // T-071 night light: torches and lanterns branch off before the sip lane.
  // Premise note: no new kUse command was needed — kUseItem is already
  // journaled (only chat/ping are excluded) and the client already sends it
  // for any slot-2 click.
  if (sl.itemId == 3003) {  // Torch: burns down, one hand, one sip-gate tick
    e.lastSipTick = tick_;
    --sl.qty;
    if (sl.qty == 0) e.inv.erase(e.inv.begin() + slot);
    e.lightRadius = 6;
    e.lightUntil = tick_ + 6000;  // 300 s of carried light
    e.lanternLit = false;         // the torch wins while it burns
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.invChanged = true;
    ev.statsChanged = true;
    ev.chatCh = 2;
    ev.chatText = e.name + " lights a torch (6 tiles, 5 min).";
    events_.push_back(std::move(ev));
    return true;
  }
  if (sl.itemId == 3004) {  // Blessed Lantern: never consumed, toggles 8
    e.lastSipTick = tick_;
    e.lanternLit = !e.lanternLit;
    e.lightRadius = e.lanternLit ? 8 : 0;
    e.lightUntil = -1;  // held light never expires; dropping ends it (later)
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.statsChanged = true;
    ev.chatCh = 2;
    ev.chatText = e.lanternLit ? e.name + " raises the Blessed Lantern."
                               : e.name + " shutters the lantern.";
    events_.push_back(std::move(ev));
    return true;
  }
  if (e.hp >= e.hpMax) return false;
  e.lastSipTick = tick_;
  --sl.qty;
  std::uint32_t healed = std::min<std::uint32_t>(d->heal, e.hpMax - e.hp);
  if (tick_ < e.curseUntil) healed = healed * 75u / 100u;  // T-070 thin blood
  e.hp += healed;
  if (sl.qty == 0) e.inv.erase(e.inv.begin() + slot);
  WorldEvent ev;
  ev.attacker = e.id;
  ev.target = e.id;
  ev.kind = 4;
  ev.amount = static_cast<std::uint16_t>(healed);
  ev.aboutId = e.id;
  ev.invChanged = true;
  events_.push_back(std::move(ev));
  return true;
}

bool World::toggleEquip(Entity& e, std::uint8_t slot) {
  if (e.kind != EntityKind::kPlayer || slot >= e.inv.size()) return false;
  InvSlot& sl = e.inv[slot];
  const content::ItemDef* d = content::findItem(sl.itemId);
  if (d == nullptr || d->slot > 1) return false;
  if (!sl.equipped) {
    // one equipped item per gear slot
    for (InvSlot& other : e.inv) {
      const content::ItemDef* od = content::findItem(other.itemId);
      if (od != nullptr && od->slot == d->slot) other.equipped = false;
    }
    sl.equipped = true;
  } else {
    sl.equipped = false;
  }
  syncIndxPush(e);
  return true;
}

// T-053: kit-gated dispatch. wrong kit/channel or under-level = silent no-op
// (the rite is not yours) — no event spam on keyspam.
std::uint32_t World::effAcc(const Entity& e) const {
  std::uint32_t acc = 2u * e.dex;
  if (e.blessUntil >= 0 && tick_ < e.blessUntil) acc = acc * 110u / 100u;  
  if (e.chorusUntil >= 0 && tick_ < e.chorusUntil) acc = acc * 105u / 100u;  // T-054b
  return acc;
}

std::uint32_t World::effDmgBase(const Entity& e) const {
  if (e.kind != EntityKind::kPlayer) {
    const content::MobDef* md = content::findMob(e.mobId);
    return md != nullptr ? md->dmg : 4;
  }
  std::uint32_t base = equippedWeaponDmg(e) + e.swordSkill / 20;
  if (e.blessUntil >= 0 && tick_ < e.blessUntil) base = base * 110u / 100u;  
  if (e.chorusUntil >= 0 && tick_ < e.chorusUntil) base = base * 105u / 100u;  // T-054b
  return base;
}

std::uint32_t World::effDef(const Entity& e) const {
  std::uint32_t d = 0;
  if (e.kind == EntityKind::kPlayer) {
    d = equippedArmorDef(e);
    if (e.ironskinUntil >= 0 && tick_ < e.ironskinUntil) d = d * 120u / 100u;
  } else if (const content::MobDef* md = content::findMob(e.mobId)) {
    d = md->def;
  }
  return d;
}

void World::trySkill(Entity& e, std::uint8_t skill, std::uint32_t targetId) {
  if (e.kind != EntityKind::kPlayer || e.dead) return;
  if (skill >= 2 && skill <= 9) {
    const std::uint8_t unlock = content::kitSkillUnlock(e.classId, skill);
    if (unlock == 0 || e.level < unlock) return;
    switch (skill) {
      case 2: tryMend(e, targetId); return;
      case 3: tryBless(e, targetId); return;
      case 4: tryIronskin(e, targetId); return;
      case 5: tryFirebolt(e, targetId); return;
      case 6: tryChorus(e); return;     // T-054b
      case 7: tryMassMend(e); return;   // T-054b
      case 8: tryHaste(e); return;      // T-054b
      case 9: tryPurify(e, targetId); return;  // T-082 field cleanse
      default: return;
    }
  }
  if (skill != 1) return;
  if (content::kitSkillUnlock(e.classId, 1) == 0) return;   // kit has no swing
  if (e.level < content::kitSkillUnlock(e.classId, 1)) return;
  if (tick_ - e.lastPowerTick < kPowerSwingCdTicks) return;
  Entity* target = find(targetId);
  if (target == nullptr || target->dead || content::wireIsFurniture(target->wireKind)) return;
  if (chebyshev(e.walker.tile(), target->walker.tile()) > 1) return;
  e.lastPowerTick = tick_;

  const int acc = static_cast<int>(effAcc(e));
  const int evd = target->kind == EntityKind::kPlayer ? target->dex : target->dex;
  const sim::HitCheck hc = sim::rollHit(acc, evd, e.dex, rng_);
  if (!hc.hit) {
    WorldEvent ev;
    ev.attacker = e.id;
    ev.target = target->id;
    ev.kind = 0;
    events_.push_back(std::move(ev));
    return;
  }
  const std::uint32_t base = effDmgBase(e);
  const std::uint32_t def = effDef(*target);
  std::uint32_t dmg = sim::rollDamage(base, e.str, def, hc.crit);
  dmg = dmg * kPowerSwingMultPct / 100u;
  if (target->kind == EntityKind::kPlayer) dmg = dmg * 65u / 100u;
  dmg = dmg < 1 ? 1 : dmg;
  target->hp = dmg >= target->hp ? 0 : target->hp - dmg;
  target->lastHurtTick = tick_;
  if (target->kind == EntityKind::kMob && target->attackTarget == 0) {
    target->attackTarget = e.id;
  }
  WorldEvent ev;
  ev.attacker = e.id;
  ev.target = target->id;
  ev.kind = 5;  // skill hit (HB red-caps callout client-side)
  ev.amount = static_cast<std::uint16_t>(dmg > 65535 ? 65535 : dmg);
  events_.push_back(ev);
  if (target->hp == 0) {
    const std::string victimName = target->name;
    const std::uint32_t victimId = target->id;
    WorldEvent kill;
    kill.attacker = e.id;
    kill.target = victimId;
    kill.kind = 3;
    kill.amount = ev.amount;
    if (target->kind == EntityKind::kMob) {
      // T-106: snapshot before killMob() — its despawn() erases mid-deque and
      // invalidates ALL element references ([deque.modifiers]); the slot behind
      // `e` can hold a shifted neighbour afterwards (wrong name on the line).
      const std::string killerName = e.name;
      killMob(*target, &e);
      kill.chatCh = 3;
      kill.chatText = killerName + " has slain a " + victimName + ".";
    } else {
      kill.chatCh = 3;
      kill.chatText = victimName + " was slain by " + e.name + ".";
      killPlayer(*target, &e);
    }
    events_.push_back(std::move(kill));
  }
}

// ---- T-054 kit skill handlers --------------------------------------------
namespace {
// shared cultist target rules: self or living party member, same zone, in range
bh::server::Entity* choirTarget(bh::server::World& w, bh::server::Entity& e,
                                std::uint32_t targetId) {
  bh::server::Entity* t = w.find(targetId == 0 ? e.id : targetId);
  if (t == nullptr || t->kind != bh::server::EntityKind::kPlayer || t->dead) return nullptr;
  const bool isSelf = t->id == e.id;
  const bool sameParty = e.partyId != 0 && t->partyId == e.partyId;
  if (!isSelf && !sameParty) return nullptr;  // choir mends the sworn circle only
  if (t->zoneId != e.zoneId) return nullptr;
  return t;
}
}  // namespace

void World::tryMend(Entity& e, std::uint32_t targetId) {
  if (tick_ - e.lastMendTick < kMendCdTicks) return;
  if (e.mp < kMendMpCost) return;  // out of breath (era: quiet fail)
  Entity* t = choirTarget(*this, e, targetId);
  if (t == nullptr) return;
  if (chebyshev(e.walker.tile(), t->walker.tile()) > kMendRange) return;
  e.lastMendTick = tick_;
  e.mp -= kMendMpCost;
  const std::uint32_t amount = 30u + 4u * e.level;
  const std::uint32_t room = t->hpMax - t->hp;
  std::uint32_t healed = amount < room ? amount : room;
  if (tick_ < t->curseUntil) healed = healed * 75u / 100u;  // T-070 thin blood
  t->hp += healed;
  // heal floater reuses combat-event path; kind 8 = life given (client: green)
  WorldEvent ev;
  ev.attacker = e.id;
  ev.target = t->id;
  ev.kind = 8;
  ev.amount = static_cast<std::uint16_t>(healed > 65535 ? 65535 : healed);
  events_.push_back(ev);
  if (healed > 0) {
    WorldEvent txt;
    txt.aboutId = t->id;
    txt.chatCh = 2;
    txt.chatText = e.id == t->id ? e.name + " mends their own wounds (" +
                                       std::to_string(healed) + ")."
                                 : e.name + " mends " + t->name + " (" +
                                       std::to_string(healed) + ").";
    events_.push_back(std::move(txt));
  }
}

// T-082 Purify (chan 9, Cultist): the field cleanse — clears the Blood Curse
// where no chapel stands. Gates mirror Mend exactly (same CD/MP/range/target
// law — stated, not designed): reuses kMendCdTicks/kMendMpCost/kMendRange and
// choirTarget. Cleanses ONLY (no heal, no bless touch); quiet fail when the
// target runs clean blood.
void World::tryPurify(Entity& e, std::uint32_t targetId) {
  if (tick_ - e.lastPurifyTick < kMendCdTicks) return;
  if (e.mp < kMendMpCost) return;  // out of breath (era: quiet fail)
  Entity* t = choirTarget(*this, e, targetId);
  if (t == nullptr) return;
  if (chebyshev(e.walker.tile(), t->walker.tile()) > kMendRange) return;
  if (tick_ >= t->curseUntil) return;  // clean blood: nothing to purge
  e.lastPurifyTick = tick_;
  e.mp -= kMendMpCost;
  t->curseUntil = -1;
  WorldEvent ev;
  ev.aboutId = t->id;
  ev.statsChanged = true;
  ev.chatCh = 2;
  ev.chatText = e.id == t->id ? e.name + " purges the thin blood from their own veins."
                              : e.name + " purges the thin blood from " + t->name + ".";
  events_.push_back(std::move(ev));
}

void World::tryBless(Entity& e, std::uint32_t targetId) {
  if (tick_ - e.lastBlessTick < kBlessCdTicks) return;
  if (e.mp < kBlessMpCost) return;
  Entity* t = choirTarget(*this, e, targetId);
  if (t == nullptr) return;
  if (chebyshev(e.walker.tile(), t->walker.tile()) > kMendRange) return;
  e.lastBlessTick = tick_;
  e.mp -= kBlessMpCost;
  t->blessUntil = tick_ + kBlessTicks;  // re-cast refreshes (stack-free)
  {  // T-066 callout sweep: gold flash over the anointed
    WorldEvent bev;
    bev.attacker = e.id;
    bev.target = t->id;
    bev.kind = 10;   // bless
    bev.amount = 10; // +10%
    events_.push_back(bev);
  }
  WorldEvent txt;
  txt.aboutId = t->id;
  txt.statsChanged = true;
  txt.chatCh = 2;
  txt.chatText = "the " + std::string(content::findKit(content::kKitCultist)->name) +
                 " blesses " + t->name + " (+10% for 5 min).";
  events_.push_back(std::move(txt));
}

// T-054b Chorus (chan 6): the whole choir sounds at once — every living
// party member in range carries the +5% hit&dmg stamp; refresh, never stack.
void World::tryChorus(Entity& e) {
  if (tick_ - e.lastChorusTick < kChorusCdTicks || e.mp < kChorusMpCost) return;
  const Party* p = partyOf(e.id);
  if (p == nullptr) return;  // a choir of one is a hum (era law: needs a party)
  e.lastChorusTick = tick_;
  e.mp -= kChorusMpCost;
  std::uint32_t sung = 0;
  for (const std::uint32_t mid : p->members) {
    Entity* m = find(mid);
    if (m == nullptr || m->dead || m->zoneId != e.zoneId) continue;
    if (chebyshev(m->walker.tile(), e.walker.tile()) > kMendRange) continue;
    m->chorusUntil = tick_ + kChorusTicks;
    ++sung;
    WorldEvent sev;
    sev.attacker = e.id;
    sev.target = mid;
    sev.kind = 13;   // T-066 table: chorus shimmer (gold crown)
    sev.amount = 5;  // +5%
    events_.push_back(std::move(sev));
  }
  WorldEvent txt;
  txt.aboutId = e.id;
  txt.statsChanged = true;
  txt.chatCh = 2;
  txt.chatText = "the choir lifts its verse on " + std::to_string(sung) +
                 " voice(s) (+5% for 2 min).";
  events_.push_back(std::move(txt));
}

// T-054b Mass Mend (chan 7): one hand over the whole party — each member in
// range catches the half-strength stitch (era mass tax).
void World::tryMassMend(Entity& e) {
  if (tick_ - e.lastMassTick < kMassMendCdTicks || e.mp < kMassMendMpCost) return;
  const Party* p = partyOf(e.id);
  if (p == nullptr) return;
  e.lastMassTick = tick_;
  e.mp -= kMassMendMpCost;
  std::uint32_t healedAny = 0;
  for (const std::uint32_t mid : p->members) {
    Entity* m = find(mid);
    if (m == nullptr || m->dead || m->zoneId != e.zoneId) continue;
    if (chebyshev(m->walker.tile(), e.walker.tile()) > kMendRange) continue;
    const std::uint32_t mend = std::min<std::uint32_t>(
        static_cast<std::uint32_t>(m->hpMax - m->hp), (30u + 4u * e.level) / 2u);
    if (mend == 0) continue;
    m->hp += mend;
    ++healedAny;
    WorldEvent sev;
    sev.attacker = e.id;
    sev.target = mid;
    sev.kind = 8;  // mend lane (green)
    sev.amount = static_cast<std::uint16_t>(std::min<std::uint32_t>(mend, 65535));
    events_.push_back(std::move(sev));
  }
  WorldEvent txt;
  txt.aboutId = e.id;
  txt.statsChanged = true;
  txt.chatCh = 2;
  txt.chatText = "mass mend knits " + std::to_string(healedAny) + " body(ies).";
  events_.push_back(std::move(txt));
}

// T-054b Haste (chan 8): self-cast rotation gear — swing cadence -25% for
// the duration; read in trySwing where cd is computed.
void World::tryHaste(Entity& e) {
  if (tick_ - e.lastHasteTick < kHasteCdTicks || e.mp < kHasteMpCost) return;
  e.lastHasteTick = tick_;
  e.mp -= kHasteMpCost;
  e.hasteUntil = tick_ + kHasteTicks;
  {
    WorldEvent sev;
    sev.attacker = e.id;
    sev.target = e.id;
    sev.kind = 14;  // haste: amber streak
    sev.amount = 25;
    events_.push_back(std::move(sev));
  }
  WorldEvent txt;
  txt.aboutId = e.id;
  txt.statsChanged = true;
  txt.chatCh = 2;
  txt.chatText = "haste: the blood remembers its own beat (-25% swing cadence).";
  events_.push_back(std::move(txt));
}

void World::tryIronskin(Entity& e, std::uint32_t targetId) {
  if (tick_ - e.lastIronskinTick < kIronskinCdTicks) return;
  if (e.mp < kIronskinMpCost) return;
  Entity* t = choirTarget(*this, e, targetId);
  if (t == nullptr) return;
  if (chebyshev(e.walker.tile(), t->walker.tile()) > kMendRange) return;
  e.lastIronskinTick = tick_;
  e.mp -= kIronskinMpCost;
  t->ironskinUntil = tick_ + kBlessTicks;
  {  // T-066 callout sweep: steel flash over the armored
    WorldEvent iev;
    iev.attacker = e.id;
    iev.target = t->id;
    iev.kind = 11;  // ironskin
    iev.amount = 0;
    events_.push_back(iev);
  }
  WorldEvent txt;
  txt.aboutId = t->id;
  txt.statsChanged = true;
  txt.chatCh = 2;
  txt.chatText = t->name + "'s skin turns to grave-iron (+20% DR for 5 min).";
  events_.push_back(std::move(txt));
}

void World::tryFirebolt(Entity& e, std::uint32_t targetId) {
  if (tick_ - e.lastFireboltTick < kFireboltCdTicks) return;
  if (e.mp < kFireboltMpCost) return;
  Entity* t = find(targetId);
  if (t == nullptr || t->dead || content::wireIsFurniture(t->wireKind)) return;
  if (t->zoneId != e.zoneId ||
      chebyshev(e.walker.tile(), t->walker.tile()) > kFireboltRange) return;
  e.lastFireboltTick = tick_;
  e.mp -= kFireboltMpCost;
  // plague-fire ignores plate; PvP scalar applies (GDD), int adds via kit seed
  std::uint32_t dmg = 8u + 2u * e.level + 2u * e.intg;
  if (t->kind == EntityKind::kPlayer) dmg = dmg * 65u / 100u;
  dmg = dmg < 1 ? 1 : dmg;
  t->hp = dmg >= t->hp ? 0 : t->hp - dmg;
  t->lastHurtTick = tick_;
  if (t->kind == EntityKind::kMob && t->attackTarget == 0) t->attackTarget = e.id;
  WorldEvent ev;
  ev.attacker = e.id;
  ev.target = t->id;
  ev.kind = 5;  // skill hit callout (shares Power-Swing red-caps lane)
  ev.amount = static_cast<std::uint16_t>(dmg > 65535 ? 65535 : dmg);
  events_.push_back(ev);
  if (t->hp == 0) {
    WorldEvent kill;
    kill.attacker = e.id;
    kill.target = t->id;
    kill.kind = 3;
    kill.amount = ev.amount;
    const std::string victimName = t->name;
    if (t->kind == EntityKind::kMob) {
      killMob(*t, &e);
      kill.chatCh = 3;
      kill.chatText = victimName + " bursts into plague-fire.";
    } else {
      kill.chatCh = 3;
      kill.chatText = victimName + " was burned down by " + e.name + ".";
      killPlayer(*t, &e);
    }
    events_.push_back(std::move(kill));
  }
}

bool World::vendorBuy(Entity& e, std::uint32_t itemId, std::uint16_t qty) {
  if (e.kind != EntityKind::kPlayer || e.dead || qty == 0 || qty > 16) return false;
  if (karmaBandOf(e.karma) == 2) {  // T-056: red names buy nothing from Marta
    WorldEvent sneer; sneer.aboutId = e.id; sneer.chatCh = 2;
    sneer.chatText = "Marta wants no red-stained coin.";
    events_.push_back(std::move(sneer));
    return false;
  }
  // T-073: the wanted are refused like the red (gallows-bound coin).
  if (tick_ < e.wantedUntil) {
    WorldEvent sneer; sneer.aboutId = e.id; sneer.chatCh = 2;
    sneer.chatText = "Marta wants no gallows-bound coin.";
    events_.push_back(std::move(sneer));
    return false;
  }
  bool stocked = false;
  for (const std::uint32_t id : content::kVendorStock) {
    if (id == itemId) stocked = true;
  }
  const content::ItemDef* d = content::findItem(itemId);
  if (!stocked || d == nullptr) return false;
  // must stand near the vendor
  bool nearVendor = false;
  for (const auto& v : entities_) {
    if (v.wireKind == content::kWireKindVendor &&
        chebyshev(v.walker.tile(), e.walker.tile()) <= 3) {
      nearVendor = true;
      break;
    }
  }
  if (!nearVendor) return false;
  const std::uint32_t cost = d->value * qty;
  if (e.gold < cost || (d->stackMax == 1 && qty > 1)) return false;
  // stack check for stackables
  if (d->stackMax == 1) {
    qty = 1;
  }
  const std::uint32_t realCost = d->value * qty;
  if (e.gold < realCost) return false;
  if (!addItem(e, itemId, qty)) return false;
  e.gold -= realCost;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.invChanged = true;
  ev.statsChanged = true;
  ev.chatCh = 255;
  ev.chatText = "bought " + std::to_string(qty) + "x " + d->name + " for " +
                std::to_string(realCost) + "g.";
  events_.push_back(std::move(ev));
  return true;
}

// T-058 repair: vendor-proximity, 1 gold per 2 durability points per item
// (rounded up per item; era shop-rate tax — the Widow undercuts with parts)
bool World::repairAll(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead) return false;
  if (karmaBandOf(e.karma) == 2) return false;  // Marta's refusal stands
  if (tick_ < e.wantedUntil) return false;     // T-073: wanted refused too
  bool nearVendor = false;
  for (const auto& v : entities_) {
    if (v.wireKind == content::kWireKindVendor &&
        chebyshev(v.walker.tile(), e.walker.tile()) <= 3) {
      nearVendor = true;
      break;
    }
  }
  if (!nearVendor) return false;
  std::uint32_t total = 0;
  std::uint32_t fixed = 0;
  for (InvSlot& sl : e.inv) {
    const content::ItemDef* d = content::findItem(sl.itemId);
    if (d == nullptr || d->slot > 1 || sl.durability >= 100) continue;
    const std::uint32_t missing = 100u - sl.durability;
    total += (missing + 1u) / 2u;  // 1g per 2 points
  }
  if (total == 0 || e.gold < total) return false;  // all-or-nothing, era-plain
  e.gold -= total;
  for (InvSlot& sl : e.inv) {
    const content::ItemDef* d = content::findItem(sl.itemId);
    if (d == nullptr || d->slot > 1) continue;
    if (sl.durability < 100) { sl.durability = 100; ++fixed; }
  }
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.statsChanged = true;
  ev.invChanged = true;
  ev.chatCh = 2;
  ev.chatText = "Marta hammers out the dents (-" + std::to_string(total) +
                "g, " + std::to_string(fixed) + " piece(s) good as new).";
  events_.push_back(std::move(ev));
  return true;
}

std::uint32_t World::vendorSellJunk(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead) return 0;
  if (karmaBandOf(e.karma) == 2) {  // T-056 refusal (pawn lane)
    WorldEvent sneer; sneer.aboutId = e.id; sneer.chatCh = 2;
    sneer.chatText = "Marta waves you off. Red hands, red prices: none.";
    events_.push_back(std::move(sneer));
    return 0;
  }
  // T-073: the wanted get the same wave-off.
  if (tick_ < e.wantedUntil) {
    WorldEvent sneer; sneer.aboutId = e.id; sneer.chatCh = 2;
    sneer.chatText = "Marta waves you off. Gallows-bound hands: none.";
    events_.push_back(std::move(sneer));
    return 0;
  }
  bool nearVendor = false;
  for (const auto& v : entities_) {
    if (v.wireKind == content::kWireKindVendor && chebyshev(v.walker.tile(), e.walker.tile()) <= 3) {
      nearVendor = true;
      break;
    }
  }
  if (!nearVendor) return 0;
  std::uint32_t gained = 0;
  for (size_t i = 0; i < e.inv.size();) {
    const content::ItemDef* d = content::findItem(e.inv[i].itemId);
    if (d != nullptr && d->slot == 3) {
      gained += d->value * content::kSellRatioPct / 100 * e.inv[i].qty;
      e.inv.erase(e.inv.begin() + static_cast<long>(i));
    } else {
      ++i;
    }
  }
  if (gained > 0) {
    e.gold += gained;
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.invChanged = true;
    ev.statsChanged = true;
    ev.chatCh = 255;
    ev.chatText = "sold junk for " + std::to_string(gained) + "g.";
    events_.push_back(std::move(ev));
  }
  return gained;
}

// ---- T-069 Smugglers' Cove fence (Sable) -----------------------------------
// The no-questions lane Marta refuses: junk pawn at 60% for anyone (better
// than Marta's 40% — that's the draw), secret stock sold to chaotic eyes
// only at a 25% markup. Trading with the fence never moves karma.

bool World::nearFence(const Entity& e) const {
  for (const Entity& other : entities_) {
    if (other.wireKind == content::kWireKindFence &&
        other.zoneId == e.zoneId &&
        chebyshev(other.walker.tile(), e.walker.tile()) <= 3) {
      return true;
    }
  }
  return false;
}

bool World::fenceBuy(Entity& e, std::uint32_t itemId, std::uint16_t qty) {
  if (e.kind != EntityKind::kPlayer || e.dead || qty == 0 || qty > 16) return false;
  bool stocked = false;
  for (const std::uint32_t id : content::kFenceStock) {
    if (id == itemId) stocked = true;
  }
  const content::ItemDef* d = content::findItem(itemId);
  if (!stocked || d == nullptr) return false;  // Sable sells exactly one crate
  if (!nearFence(e)) return false;
  if (karmaBandOf(e.karma) != 2) {  // the crate shows itself to red eyes only
    WorldEvent sneer;
    sneer.aboutId = e.id;
    sneer.chatCh = 2;
    sneer.chatText = "Sable sizes up your clean hands and turns away.";
    events_.push_back(std::move(sneer));
    return false;
  }
  if (d->stackMax == 1) qty = 1;
  const std::uint32_t cost =
      d->value * content::kFenceMarkupPct / 100 * qty;
  if (e.gold < cost) return false;
  if (!addItem(e, itemId, qty)) return false;
  e.gold -= cost;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.invChanged = true;
  ev.statsChanged = true;
  ev.chatCh = 255;
  ev.chatText = "bought " + std::to_string(qty) + "x " + d->name +
                " (no questions) for " + std::to_string(cost) + "g.";
  events_.push_back(std::move(ev));
  return true;
}

std::uint32_t World::fenceSellJunk(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead) return 0;
  if (!nearFence(e)) return 0;  // no karma gate: Sable asks nothing
  std::uint32_t gained = 0;
  for (size_t i = 0; i < e.inv.size();) {
    const content::ItemDef* d = content::findItem(e.inv[i].itemId);
    if (d != nullptr && d->slot == 3) {
      gained += d->value * content::kFenceSellRatioPct / 100 * e.inv[i].qty;
      e.inv.erase(e.inv.begin() + static_cast<long>(i));
    } else {
      ++i;
    }
  }
  if (gained > 0) {
    e.gold += gained;
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.invChanged = true;
    ev.statsChanged = true;
    ev.chatCh = 255;
    ev.chatText = "fenced junk for " + std::to_string(gained) +
                  "g (no questions asked).";
    events_.push_back(std::move(ev));
  }
  return gained;
}

// ---- T-070 chapel cure (the confessor) --------------------------------------
// The Blood Curse answers to the chapel, not to coin: standing within 3 of
// the confessor and speaking (/confess) clears curseUntil with a fiction
// line. Karma repentance is out of scope (later card) — this lane touches
// no karma, only the curse.

bool World::nearConfessor(const Entity& e) const {
  for (const Entity& other : entities_) {
    if (other.wireKind == content::kWireKindConfessor &&
        other.zoneId == e.zoneId &&
        chebyshev(other.walker.tile(), e.walker.tile()) <= 3) {
      return true;
    }
  }
  return false;
}

bool World::confess(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead) return false;
  if (tick_ >= e.curseUntil) return false;  // nothing to shrive (quiet fail)
  if (!nearConfessor(e)) return false;
  e.curseUntil = -1;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.statsChanged = true;
  ev.chatCh = 2;
  ev.chatText = e.name + " kneels, and the chapel takes the thin blood away.";
  events_.push_back(std::move(ev));
  return true;
}

// T-075 chapel repentance (deferred from T-070): the chapel's second lane.
// Grace, not cure — karma moves, the curse does not. Amount (+20) is one
// whitening-hour at the pinned rate; cooldown (72000 ticks = 1 logged hour)
// is the same unit. Wanted players are refused with the gallows-bound line:
// the gate law and chapel grace stay separate lanes.
bool World::repent(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead) return false;
  if (!nearConfessor(e)) return false;
  if (tick_ < e.wantedUntil) {
    WorldEvent sneer;
    sneer.aboutId = e.id;
    sneer.chatCh = 2;
    sneer.chatText = "The chapel wants no gallows-bound coin. Serve the post first.";
    events_.push_back(std::move(sneer));
    return false;
  }
  if (tick_ < e.repentUntil) return false;  // grace cools hourly (quiet fail)
  e.repentUntil = tick_ + 72000;
  const std::int32_t before = e.karma;
  bumpKarma(e, 20);  // clamp ±1000 + band-crossing events inside
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.statsChanged = true;
  ev.chatCh = 2;
  ev.chatText = e.name + " repents (" + std::to_string(before) + " -> " +
                std::to_string(e.karma) + " karma).";
  events_.push_back(std::move(ev));
  return true;
}

void World::spawnConfessor(Zone& zone) {
  // The chapel rect (9,9)-(14,13) on thornwall: first walkable tile with no
  // furniture on it. One confessor, session-seeded like the other furniture.
  for (int y = 9; y <= 13; ++y) {
    for (int x = 9; x <= 14; ++x) {
      if (!zone.map.inBounds(x, y) || zone.map.isBlocked(x, y)) continue;
      bool occupied = false;
      for (const Entity& ee : entities_)
        if (ee.zoneId == 1 && !ee.dead && ee.walker.tile().x == x &&
            ee.walker.tile().y == y &&
            ee.wireKind >= content::kWireKindFurnitureFloor) {
          occupied = true;
          break;
        }
      if (occupied) continue;
      Entity f;
      f.id = nextId_++;
      f.zoneId = 1;
      f.kind = EntityKind::kMob;  // furniture, non-combat; wire kind 68
      f.wireKind = content::kWireKindConfessor;
      f.name = "Confessor";
      f.hp = 1;
      f.hpMax = 1;
      f.dead = false;
      f.walker.place(sim::TilePos{x, y});
      insertEntity(std::move(f));
      return;
    }
  }
}

// ---- anvil & aura spine (T-041/T-042, RFC 0001) ----------------------------

void World::spawnAnvils() {
  // zone 1 plaza: near Marta (spiral from spawn); zone 3: bone barrow seat.
  // One anvil per zone (T-069 class fix: the bare `break` below only leaves
  // the dx loop — without the placed flag this seeds every ring row).
  for (const auto& [zoneId, at] :
       {std::pair<std::uint16_t, sim::TilePos>{1, zones_.at(1).spawnPoint},
        {3, sim::TilePos{44, 6}}}) {
    auto it = zones_.find(zoneId);
    if (it == zones_.end()) continue;
    bool placed = false;
    for (int r = 0; r < 8 && !placed; ++r) {
      for (int dy = -r; dy <= r && !placed; ++dy) {
        for (int dx = -r; dx <= r && !placed; ++dx) {
          const int x = at.x + dx, y = at.y + dy;
          if (!it->second.map.inBounds(x, y) || it->second.map.isBlocked(x, y)) continue;
          Entity a;
          a.id = nextId_++;
          a.zoneId = zoneId;
          a.kind = EntityKind::kMob;  // furniture: non-combat
          a.wireKind = 65;            // anvil marker on the wire
          a.name = "Widow Anvil";
          a.hp = 1;
          a.hpMax = 1;
          a.walker.place(sim::TilePos{x, y});
          insertEntity(std::move(a));
          placed = true;
        }
      }
    }
  }
}

bool World::nearAnvil(const Entity& e) const {
  for (const Entity& other : entities_) {
    if (other.wireKind == content::kWireKindAnvil && other.zoneId == e.zoneId &&
        chebyshev(e.walker.tile(), other.walker.tile()) <= 2) {
      return true;
    }
  }
  return false;
}

// T-060 refine: consume 1 monster part + 50g per attempt. Tiers 0->1 and
// 1->2 guaranteed (era mercy), 2->3 is 60% with DESTRUCTION on failure
// (Soma anvil law). Needs full durability — mend it first.
bool World::tryRefine(Entity& e, std::uint8_t invSlot) {
  if (e.kind != EntityKind::kPlayer || e.dead || !nearAnvil(e)) return false;
  if (invSlot >= e.inv.size()) return false;
  InvSlot& sl = e.inv[invSlot];
  const content::ItemDef* d = content::findItem(sl.itemId);
  if (d == nullptr || d->slot > 1) return false;  // gear only
  if (sl.refine >= 7) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "there is no hotter coal for this iron (refine is full).";
    events_.push_back(ev);
    return false;
  }
  if (sl.durability < 100) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "mend it first — cracked metal lies about its temper.";
    events_.push_back(ev);
    return false;
  }
  int junkIdx = -1;
  for (size_t i = 0; i < e.inv.size(); ++i) {
    const content::ItemDef* jd = content::findItem(e.inv[i].itemId);
    if (jd != nullptr && jd->slot == 3) { junkIdx = static_cast<int>(i); break; }
  }
  if (junkIdx < 0 || e.gold < 50) return false;
  if (--e.inv[junkIdx].qty == 0) {
    if (static_cast<size_t>(junkIdx) < e.inv.size() &&
        static_cast<size_t>(junkIdx) != invSlot)
      e.inv.erase(e.inv.begin() + junkIdx);
    // (junk slot is never the gear slot; erase shifts indexes > junkIdx)
    if (static_cast<size_t>(junkIdx) < invSlot) --invSlot;
  }
  InvSlot& tgt = e.inv[invSlot];
  e.gold -= 50;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.statsChanged = true;
  ev.invChanged = true;
  ev.chatCh = 255;
  // T-079: shipped rows frozen (0->1, 1->2 mercy; 2->3 60% + destruction).
  // New rows take GDD §7 rates; failure slips one temper, except the +7
  // bid, which forgets every temper on failure (GDD-literal reset to +0).
  static constexpr std::uint8_t kRefineChance[7] = {100, 100, 60, 65, 50, 35, 25};
  const std::uint8_t chance = kRefineChance[tgt.refine];
  if (rng_.range(1, 100) <= chance) {
    ++tgt.refine;
    std::printf("[anvil-refine] %s %u -> %u item=%u\n", e.name.c_str(),
                static_cast<unsigned>(tgt.refine) - 1,
                static_cast<unsigned>(tgt.refine), tgt.itemId);
    ev.chatText = std::string("(anvil) the ring answers true: ") + d->name +
                  " +" + std::to_string(tgt.refine) + ".";
  } else if (tgt.refine == 2) {
    std::printf("[anvil-refine] %s SHATTERED item=%u\n", e.name.c_str(), tgt.itemId);
    ev.chatText = std::string("(anvil) the ") + d->name +
                  " SINGS WRONG and falls apart. The Widow keeps the iron.";
    e.inv.erase(e.inv.begin() + invSlot);
  } else if (tgt.refine == 6) {
    tgt.refine = 0;
    std::printf("[anvil-refine] %s RESET item=%u\n", e.name.c_str(), tgt.itemId);
    ev.chatText = std::string("(anvil) the ") + d->name +
                  " forgets every temper: +0. The Widow keeps the coal.";
  } else {
    --tgt.refine;
    std::printf("[anvil-refine] %s slipped %u item=%u\n", e.name.c_str(),
                static_cast<unsigned>(tgt.refine) + 1, tgt.itemId);
    ev.chatText = std::string("(anvil) the iron slips a temper: ") + d->name +
                  " +" + std::to_string(tgt.refine) + ".";
  }
  events_.push_back(std::move(ev));
  return true;
}

bool World::tryAnvil(Entity& e, std::uint8_t tier) {
  if (e.kind != EntityKind::kPlayer || e.dead) {
    std::printf("[anvil-refuse] %s tier=%u why=%s\n", e.name.c_str(),
                static_cast<unsigned>(tier),
                e.kind != EntityKind::kPlayer ? "notplayer" : "dead");
    return false;
  }
  const content::AuraTier* t = content::findAuraTier(tier);
  if (t == nullptr || !nearAnvil(e)) {
    std::printf("[anvil-refuse] %s tier=%u why=nearAnvil\n", e.name.c_str(),
                static_cast<unsigned>(tier));
    return false;
  }
  InvSlot* wslot = nullptr;
  for (InvSlot& sl : e.inv) {
    if (sl.equipped) {
      const content::ItemDef* d = content::findItem(sl.itemId);
      if (d != nullptr && d->slot == 0) wslot = &sl;
    }
  }
  if (wslot == nullptr) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    std::printf("[anvil-refuse] %s tier=%u why=unarmed\n", e.name.c_str(),
                static_cast<unsigned>(tier));
    ev.chatText = "the Anvil decides only armed petitions.";
    events_.push_back(ev);
    return false;
  }
  if (wslot->aura >= tier) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "that blessing already sleeps in the steel.";
    std::printf("[anvil-refuse] %s tier=%u why=already aura=%u\n", e.name.c_str(),
                static_cast<unsigned>(tier), static_cast<unsigned>(wslot->aura));
    events_.push_back(ev);
    return false;
  }
  if (wslot->aura + 1 != tier) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "the Anvil honors order: earn tier " + std::to_string(wslot->aura + 1) + " first.";
    std::printf("[anvil-refuse] %s tier=%u why=order aura=%u\n", e.name.c_str(),
                    static_cast<unsigned>(tier), static_cast<unsigned>(wslot->aura));
    events_.push_back(ev);
    return false;
  }
  if (e.swordSkill < t->reqSkill) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "your hand is not yet steady: weapon skill " + std::to_string(t->reqSkill) + " required.";
    std::printf("[anvil-refuse] %s tier=%u why=skill skill=%u req=%u\n", e.name.c_str(),
                    static_cast<unsigned>(tier), static_cast<unsigned>(e.swordSkill),
                    static_cast<unsigned>(t->reqSkill));
    events_.push_back(ev);
    return false;
  }
  if (e.gold < t->gold) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "the toll is " + std::to_string(t->gold) + "g - the Anvil does not haggle.";
    std::printf("[anvil-refuse] %s tier=%u why=gold gold=%u\n", e.name.c_str(),
                    static_cast<unsigned>(tier), static_cast<unsigned>(e.gold));
    events_.push_back(ev);
    return false;
  }
  std::uint32_t parts = 0;
  for (const InvSlot& sl : e.inv) {
    if (sl.itemId == t->partItemId) parts += sl.qty;
  }
  if (parts < t->partQty) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.chatCh = 255;
    ev.chatText = "parts short: " + std::to_string(t->partQty) + "x " +
                  std::string(content::findItem(t->partItemId) != nullptr
                                  ? content::findItem(t->partItemId)->name
                                  : "offering") +
                  " required.";
    std::printf("[anvil-refuse] %s tier=%u why=parts\n", e.name.c_str(),
                    static_cast<unsigned>(tier));
    events_.push_back(ev);
    return false;
  }

  // mercy rule: first attempt at any tier is guaranteed (bit per tier)
  const std::uint32_t mercyBit = 1u << tier;
  const bool mercy = (e.anvilMercyMask & mercyBit) == 0;
  e.anvilMercyMask |= mercyBit;

  // consume parts + gold, atomically (all or nothing)
  std::uint16_t remaining = t->partQty;
  for (size_t i = 0; i < e.inv.size() && remaining > 0;) {
    InvSlot& sl = e.inv[i];
    if (sl.itemId != t->partItemId || sl.equipped) { ++i; continue; }
    const std::uint16_t take = sl.qty < remaining ? sl.qty : remaining;
    sl.qty = static_cast<std::uint16_t>(sl.qty - take);
    remaining = static_cast<std::uint16_t>(remaining - take);
    if (sl.qty == 0) e.inv.erase(e.inv.begin() + static_cast<long>(i));
    else ++i;
  }
  e.gold -= t->gold;

  const bool success = mercy || rng_.chance(static_cast<double>(100 - t->failPct) / 100.0);
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.attacker = e.id;  // on the wire (CombatEvent kind=6) so the plaza sees it
  ev.target = e.id;
  ev.kind = 6;         // aura ceremony pulse (client: red-caps over the anvil)
  ev.amount = tier;
  ev.statsChanged = true;
  if (success) {
    wslot->aura = tier;
    bumpKarma(e, kKarmaAnvilOk);  // the guild remembers its tithes (T-046)
    ev.chatCh = 255;
    ev.chatText = "the Anvil speaks. tier " + std::to_string(tier) + " rests in the steel.";
    std::printf("[aura] %s tier=%u res=ok%s\n", e.name.c_str(), static_cast<unsigned>(tier),
                mercy ? " mercy" : "");
  } else {
    if (t->failLaw == 1) {
      // destroy: clear the weapon slot + the aura with it (RFC 0001)
      wslot->itemId = 0;
      wslot->qty = 0;
      wslot->equipped = false;
      wslot->aura = 0;
      bumpKarma(e, kKarmaAnvilDestroy);  // drowned steel stains the soul
      ev.chatCh = 0;  // broadcast-worthy ceremony failure
      ev.chatText = e.name + "'s steel drowned at the Anvil.";
      std::printf("[aura] %s tier=%u res=destroyed\n", e.name.c_str(), static_cast<unsigned>(tier));
    } else {
      ev.chatCh = 255;
      ev.chatText = "the Anvil drinks the offering and gives nothing back.";
      std::printf("[aura] %s tier=%u res=softfail\n", e.name.c_str(), static_cast<unsigned>(tier));
    }
  }
  std::fflush(stdout);
  events_.push_back(std::move(ev));
  // statsChanged triggers OwnStats push; inventory line too
  WorldEvent invEv;
  invEv.aboutId = e.id;
  invEv.invChanged = true;
  events_.push_back(std::move(invEv));
  return true;
}

// ---- trade (T-029): commit-time validation, single-tick swap ---------------

void World::tradeCancel(Entity& e, const char* why) {
  Entity* other = find(e.tradeWith);
  const std::uint32_t otherId = e.tradeWith;
  e.tradeWith = 0;
  e.tradeOfferItems.clear();
  e.tradeOfferGold = 0;
  e.tradeCommitted = false;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.chatCh = 255;
  ev.chatText = std::string("trade cancelled (") + why + ").";
  events_.push_back(ev);
  if (other != nullptr) {
    other->tradeWith = 0;
    other->tradeOfferItems.clear();
    other->tradeOfferGold = 0;
    other->tradeCommitted = false;
    WorldEvent ev2;
    ev2.aboutId = otherId;
    ev2.chatCh = 255;
    ev2.chatText = std::string("trade with ") + e.name + " cancelled (" + why + ").";
    events_.push_back(ev2);
  }
}

bool World::tradeOpen(Entity& a, std::uint32_t partnerId) {
  Entity* b = find(partnerId);
  if (b == nullptr || b->kind != EntityKind::kPlayer || b == &a || a.dead || b->dead) {
    return false;
  }
  if (a.tradeWith != 0 || b->tradeWith != 0) return false;  // both must be free
  if (chebyshev(a.walker.tile(), b->walker.tile()) > 3) return false;
  a.tradeWith = b->id;
  b->tradeWith = a.id;
  for (Entity* x : {&a, b}) {
    WorldEvent ev;
    ev.aboutId = x->id;
    ev.chatCh = 255;
    ev.chatText = "trading with " + (x == &a ? b->name : a.name) +
                  " - offer items, then commit. Walk away to cancel.";
    events_.push_back(std::move(ev));
  }
  return true;
}

void World::tradeOffer(Entity& e, std::uint32_t itemId, std::uint16_t qty) {
  if (e.tradeWith == 0 || e.dead) return;
  e.tradeCommitted = false;  // changing the offer resets commitment (both sides re-check)
  Entity* other = find(e.tradeWith);
  if (other != nullptr) other->tradeCommitted = false;
  for (auto& [id, q] : e.tradeOfferItems) {
    if (id == itemId) {
      q = qty;
      return;
    }
  }
  e.tradeOfferItems.push_back({itemId, qty});
  // directed notice to the partner (visible offer mirror until S8 window UI)
  if (other != nullptr) {
    const content::ItemDef* d = content::findItem(itemId);
    WorldEvent ev;
    ev.aboutId = other->id;
    ev.chatCh = 255;
    ev.chatText = e.name + " offers " + std::to_string(qty) + "x " +
                  std::string(d != nullptr ? d->name : "item") +
                  ". [P commit / X cancel]";
    events_.push_back(std::move(ev));
  }
}

void World::tradeOfferGold(Entity& e, std::uint32_t gold) {
  if (e.tradeWith == 0 || e.dead) return;
  e.tradeCommitted = false;
  Entity* other = find(e.tradeWith);
  if (other != nullptr) {
    other->tradeCommitted = false;
    WorldEvent ev;
    ev.aboutId = other->id;
    ev.chatCh = 255;
    ev.chatText = e.name + " offers " + std::to_string(gold) + "g. [P commit / X cancel]";
    events_.push_back(std::move(ev));
  }
  e.tradeOfferGold = gold;
}

void World::tradeCommit(Entity& e) {
  Entity* b = find(e.tradeWith);
  if (b == nullptr || e.dead) {
    if (e.tradeWith != 0) tradeCancel(e, "partner gone");
    return;
  }
  e.tradeCommitted = true;
  if (!b->tradeCommitted) return;  // wait for the handshake
  // Validate both offers atomically: ownership, quantities, gold, distance.
  // T-105: count ONLY unequipped stacks — deduce() below skips equipped slots,
  // so the pre-fix validator passed offers that the swap could never honor
  // (offer your one equipped sword, take the partner's gold, sword stays put;
  // the audit log even recorded a completed swap). One grammar on both sides.
  auto satisfiable = [](const Entity& x) {
    if (x.gold < x.tradeOfferGold) return false;
    for (const auto& [itemId, qty] : x.tradeOfferItems) {
      std::uint32_t owned = 0;
      for (const InvSlot& sl : x.inv) {
        if (sl.equipped) continue;  // equipped gear is not on the table
        if (sl.itemId == itemId) owned += sl.qty;
      }
      if (owned < qty) return false;
    }
    return true;
  };
  if (!satisfiable(e) || !satisfiable(*b)) {
    tradeCancel(e, "offer exceeded holdings");
    return;
  }
  if (chebyshev(e.walker.tile(), b->walker.tile()) > 3) {
    tradeCancel(e, "too far");
    return;
  }
  // concrete swap via addItem (stack-aware) after deduct
  auto deduce = [this](Entity& x, Entity& y) {
    for (const auto& [itemId, qty] : x.tradeOfferItems) {
      std::uint16_t remaining = qty;
      for (size_t i = 0; i < x.inv.size() && remaining > 0;) {
        InvSlot& sl = x.inv[i];
        if (sl.itemId != itemId || sl.equipped) {
          ++i;
          continue;
        }
        const std::uint16_t take = sl.qty < remaining ? sl.qty : remaining;
        sl.qty = static_cast<std::uint16_t>(sl.qty - take);
        remaining = static_cast<std::uint16_t>(remaining - take);
        addItem(y, itemId, take);
        if (sl.qty == 0) {
          x.inv.erase(x.inv.begin() + static_cast<long>(i));
        } else {
          ++i;
        }
      }
      if (remaining > 0) {
        // Unreachable while satisfiable() and this loop share one grammar
        // (T-105). If it ever fires, the two drifted again: scream in the log
        // instead of silently short-paying the partner.
        std::fprintf(stderr,
                     "[trade] BUG: %s could not deliver %ux item %u (remainder %u)\n",
                     x.name.c_str(), static_cast<unsigned>(remaining), itemId,
                     static_cast<unsigned>(remaining));
      }
    }
    x.gold -= x.tradeOfferGold;
    y.gold += x.tradeOfferGold;
  };
  deduce(e, *b);
  deduce(*b, e);
  // T-080: one audit line per EXECUTED swap (offers still intact here —
  // the cleanup loop below clears them). Append-only, opened per trade
  // (trades are rare; no hot-loop fd). Replay re-executes identically.
  {
    // Pinned line shape: tick=<t> a=<id:name> b=<id:name>
    // a_gives=(<iid:qty,...>)+<gold>g b_gives=(...)+<gold>g
    auto offerStr = [](const Entity& x) {
      std::string s = "(";
      bool first = true;
      for (const auto& [itemId, qty] : x.tradeOfferItems) {
        if (!first) s += ",";
        first = false;
        s += std::to_string(itemId) + ":" + std::to_string(qty);
      }
      s += ")+" + std::to_string(x.tradeOfferGold) + "g";
      return s;
    };
    // Canonical order: the lower id leads, so the line is identical no
    // matter who commits second (replay- and order-stable for audits).
    const Entity* first = &e;
    const Entity* second = b;
    if (first->id > second->id) std::swap(first, second);
    std::string line = "tick=" + std::to_string(tick_) + " a=" +
                       std::to_string(first->id) + ":" + first->name +
                       " b=" + std::to_string(second->id) + ":" + second->name +
                       " a_gives=" + offerStr(*first) + " b_gives=" + offerStr(*second) +
                       "\n";
    if (FILE* f = std::fopen(tradeLogPath().c_str(), "a")) {
      std::fputs(line.c_str(), f);
      std::fclose(f);
    }
  }
  for (Entity* x : {&e, b}) {
    x->tradeWith = 0;
    x->tradeOfferItems.clear();
    x->tradeOfferGold = 0;
    x->tradeCommitted = false;
    WorldEvent ev;
    ev.aboutId = x->id;
    ev.statsChanged = true;
    ev.invChanged = true;
    ev.chatCh = 255;
    ev.chatText = "trade completed.";
    events_.push_back(std::move(ev));
  }
}

// ---- combat -------------------------------------------------------------

void World::trySwing(Entity& att, Entity& def) {
  const sim::TilePos a = att.walker.tile();
  const sim::TilePos b = def.walker.tile();
  if (chebyshev(a, b) > 1) return;  // not adjacent
  sim::Tick cd = att.kind == EntityKind::kPlayer ? kPlayerAtkCdTicks
                                                 : static_cast<sim::Tick>(att.atkCdTicks);
  if (att.kind == EntityKind::kPlayer &&
      att.hasteUntil >= 0 && tick_ < att.hasteUntil)
    cd = kPlayerAtkCdTicks * 75u / 100u;  // T-054b Haste: -25% cadence
  if (tick_ - att.lastSwingTick < cd) return;
  att.lastSwingTick = tick_;

  // ACC/EVD: GDD ACC=2*DEX(+gear), EVD=DEX(+gear). Gear lands in T-021.
  int acc, evd, adex;
  std::uint32_t base, ddef;
  if (att.kind == EntityKind::kPlayer) {
    acc = static_cast<int>(effAcc(att));      // bless-aware (T-054)
    adex = att.dex;
    base = effDmgBase(att);
  } else {
    const content::MobDef* md = content::findMob(att.mobId);
    acc = 2 * att.dex;
    adex = att.dex;
    base = md != nullptr ? md->dmg : 4;
  }
  if (def.kind == EntityKind::kPlayer) {
    evd = def.dex;
    ddef = effDef(def);                        // ironskin-aware (T-054)
  } else {
    const content::MobDef* md = content::findMob(def.mobId);
    evd = def.dex;
    ddef = md != nullptr ? md->def : 0;
  }

  const sim::HitCheck hc = sim::rollHit(acc, evd, adex, rng_);
  WorldEvent ev;
  ev.attacker = att.id;
  ev.target = def.id;
  // retaliation: passive mobs fight back when struck. Drop any stale
  // wander path so the next think re-paths toward us instead of away.
  // Aggro fires on the strike itself (hit or miss) — the miss case no
  // longer skips retaliation (T-047 fix).
  if (def.kind == EntityKind::kMob && def.attackTarget == 0 && !def.dead) {
    def.attackTarget = att.id;
    def.path.clear();
  }
  if (!hc.hit) {
    ev.kind = 0;
    events_.push_back(std::move(ev));
    return;
  }
  std::uint32_t dmg = sim::rollDamage(base, att.kind == EntityKind::kPlayer ? att.str : 0,
                                      ddef, hc.crit);
  if (att.kind == EntityKind::kMob && isNight()) {
    dmg = dmg * 115u / 100u;  // T-061 nightcreep: +15% mob bite after dark
    dmg = dmg < 1 ? 1 : dmg;
  }
  if (att.kind == EntityKind::kPlayer && def.kind == EntityKind::kPlayer) {
    dmg = dmg * 65u / 100u;  // GDD PvP scalar 0.65
    dmg = dmg < 1 ? 1 : dmg;
  }
  if (def.kind == EntityKind::kMob && def.firstHurtTick < 0) {
    def.firstHurtTick = tick_;
  }
  def.hp = dmg >= def.hp ? 0 : def.hp - dmg;
  def.lastHurtTick = tick_;
  if (att.kind == EntityKind::kPlayer) {
    ++att.swingLands;
    // T-059 of Leech: equipped weapon drinks 5% of damage dealt
    for (const InvSlot& sl : att.inv) {
      const content::ItemDef* dd = content::findItem(sl.itemId);
      if (sl.equipped && dd != nullptr && dd->slot == 0 && sl.affix == 3 &&
          sl.durability > 0) {
        const std::uint32_t sip = std::max<std::uint32_t>(1, dmg / 20);
        att.hp = std::min<std::int32_t>(
            att.hp + static_cast<std::int32_t>(sip), att.hpMax);
        break;
      }
    }
    // T-058 durability burn: weapon wears per landed swing
    for (InvSlot& sl : att.inv)
      if (sl.equipped && content::findItem(sl.itemId) != nullptr &&
          content::findItem(sl.itemId)->slot == 0 && sl.durability > 0)
        --sl.durability;
    if (def.kind == EntityKind::kPlayer)
      for (InvSlot& sl : def.inv)
        if (sl.equipped && content::findItem(sl.itemId) != nullptr &&
            content::findItem(sl.itemId)->slot == 1 && sl.durability > 0)
          --sl.durability;
    const std::uint8_t newSkill =
        static_cast<std::uint8_t>(std::min<std::uint32_t>(100, att.swingLands / kSkillLandsPerPoint));
    if (newSkill != att.swordSkill) {
      const bool pointsOfNote = newSkill % 20 == 0 || newSkill == 100;
      att.swordSkill = newSkill;
      if (pointsOfNote) {
        WorldEvent wsk;
        wsk.aboutId = att.id;
        wsk.statsChanged = true;
        wsk.chatCh = 255;
        wsk.chatText = "weapon skill rises to " + std::to_string(newSkill) + ".";
        events_.push_back(std::move(wsk));
      }
    }
  }
  // aura procs (T-047): tier III cleave / tier IV sunder / tier V lifesteal.
  // sword-family encoding v1; the ceremony table kAuraTiers maps tiers to
  // procId 1/2/3. All rolls draw from rng_ so replays bit-match.
  if (att.kind == EntityKind::kPlayer) {
    std::uint8_t aura = 0;
    for (const auto& sl : att.inv) {
      if (sl.equipped) {
        const content::ItemDef* wd = content::findItem(sl.itemId);
        if (wd != nullptr && wd->slot == 0) aura = sl.aura;
        break;
      }
    }
    if (aura >= 4 && rng_.chance(0.15)) {
      // tier IV SUNDER: +35 flat, ignores nothing else (already hit)
      const std::uint32_t extra = 35;
      def.hp = extra >= def.hp ? 0 : def.hp - extra;
      WorldEvent pe;
      pe.attacker = att.id;
      pe.target = def.id;
      pe.kind = 7;
      pe.amount = static_cast<std::uint16_t>(2000 + extra);  // flavor-coded
      events_.push_back(std::move(pe));
    }
    if (aura >= 3 && aura != 5) {
      // tier III WIDOW'S EDGE: cleave up to 2 extra mobs in reach of the
      // target for the same rolled damage (era rule: 100% hit)
      int extra = 0;
      const sim::TilePos bt = def.walker.tile();
      const std::uint32_t defId = def.id;
      const std::uint16_t zid = def.zoneId;
      std::vector<std::uint32_t> cleaveIds;  // ids only; kills deferred
      for (const auto& other : entities_) {
        if (extra >= 2) break;
        if (other.id == defId || other.id == att.id) continue;
        if (other.kind != EntityKind::kMob || other.dead) continue;
        if (content::wireIsFurniture(other.wireKind)) continue;
        if (other.zoneId != zid) continue;
        if (chebyshev(other.walker.tile(), bt) > 2) continue;
        cleaveIds.push_back(other.id);
        ++extra;
      }
      for (const std::uint32_t cid : cleaveIds) {
        Entity* cv = find(cid);
        if (cv == nullptr || cv->dead) continue;
        cv->hp = dmg >= cv->hp ? 0 : cv->hp - dmg;
        cv->lastHurtTick = tick_;
        WorldEvent ce;
        ce.attacker = att.id;
        ce.target = cid;
        ce.kind = 7;
        ce.amount = static_cast<std::uint16_t>(1000 + (dmg > 900 ? 900 : dmg));
        events_.push_back(std::move(ce));
        if (cv->hp == 0) {
          if (cv->firstHurtTick < 0) cv->firstHurtTick = tick_;
          MobKillStat& ks = killStats[cv->mobId];
          const double ttk = static_cast<double>(tick_ - cv->firstHurtTick);
          ks.emaTtkTicks = ks.kills == 0 ? ttk : 0.85 * ks.emaTtkTicks + 0.15 * ttk;
          ++ks.kills;
          pendingCleaveKills_.push_back(cid);  // killMob at end (deque safety)
          pendingCleaveAttacker_ = att.id;
          pendingCleaveDmg_ = ev.amount;
        } else if (cv->attackTarget == 0) {
          cv->attackTarget = att.id;  // cleaving wakes the pack
        }
      }
    }
    if (aura >= 5 && rng_.chance(0.25)) {
      // tier V GRAFT: steal 20% of dealt damage as health
      const std::uint32_t heal = dmg / 5 + 1;
      const std::uint32_t before = att.hp;
      att.hp = att.hp + heal > att.hpMax ? att.hpMax : att.hp + heal;
      if (att.hp != before) {
        WorldEvent le;
        le.attacker = att.id;
        le.target = att.id;
        le.kind = 7;
        le.amount = static_cast<std::uint16_t>(3000 + (att.hp - before));
        events_.push_back(std::move(le));
      }
    }
  }
  ev.kind = hc.crit ? 2 : 1;
  ev.amount = static_cast<std::uint16_t>(dmg > 65535 ? 65535 : dmg);
  events_.push_back(ev);

  if (def.hp == 0) {
    // balancer (T-031): kills + TTK EMA per mob kind
    if (def.firstHurtTick >= 0) {
      MobKillStat& ks = killStats[def.mobId];
      const double ttk = static_cast<double>(tick_ - def.firstHurtTick);
      ks.emaTtkTicks = ks.kills == 0 ? ttk : 0.85 * ks.emaTtkTicks + 0.15 * ttk;
      ++ks.kills;
    }
    const std::string victimName = def.name;  // killMob() despawns (frees) it
    const std::uint32_t victimId = def.id;
    // T-106: snapshot everything read off `att` across the kill — killMob()'s
    // despawn() erases mid-deque, invalidating ALL element references
    // ([deque.modifiers]); `att`'s old slot can hold a shifted neighbour.
    const bool attIsPlayer = att.kind == EntityKind::kPlayer;
    const std::string killerName =
        attIsPlayer ? att.name : ("a " + att.name);
    WorldEvent kill;
    kill.attacker = att.id;
    kill.target = victimId;
    kill.kind = 3;
    kill.amount = ev.amount;
    if (def.kind == EntityKind::kMob) {
      killMob(def, attIsPlayer ? &att : nullptr);
      if (attIsPlayer) {
        kill.chatCh = 3;
        kill.chatText = killerName + " has slain a " + victimName + ".";
      }
    } else {
      kill.chatCh = 3;
      kill.chatText = victimName + " was slain by " + killerName + ".";
      killPlayer(def, &att);
    }
    events_.push_back(std::move(kill));
  }

  // deferred cleave kills (T-047): entities_ is a deque; references taken here
  // are fresh post-primary-kill lookups.
  for (const std::uint32_t ck : pendingCleaveKills_) {
    Entity* cv = find(ck);
    Entity* killer = find(pendingCleaveAttacker_);
    if (cv == nullptr || cv->hp > 0) continue;
    // T-106: snapshot the killer's identity BEFORE killMob() — its despawn()
    // erases mid-deque, invalidating ALL element references including *killer
    // (the slot can hold a shifted neighbour or a moved-from husk afterwards).
    const bool killerIsPlayer =
        killer != nullptr && killer->kind == EntityKind::kPlayer;
    const std::string killerName =
        killerIsPlayer ? killer->name : std::string{};
    WorldEvent kil;
    kil.attacker = pendingCleaveAttacker_;
    kil.target = ck;
    kil.kind = 3;
    kil.amount = pendingCleaveDmg_;
    if (cv->kind == EntityKind::kMob) {
      const std::string vn = cv->name;
      killMob(*cv, killerIsPlayer ? killer : nullptr);
      if (killerIsPlayer) {
        kil.chatCh = 3;
        kil.chatText = killerName + " has slain a " + vn + ".";
      }
      events_.push_back(std::move(kil));
    }
  }
  pendingCleaveKills_.clear();
  pendingCleaveAttacker_ = 0;
  pendingCleaveDmg_ = 0;
}


// ---- party (T-050/T-051) ---------------------------------------------------
const World::Party* World::partyOf(std::uint32_t entityId) const {
  for (const Party& p : parties_) {
    if (std::find(p.members.begin(), p.members.end(), entityId) != p.members.end())
      return &p;
  }
  return nullptr;
}

void World::emitPartyMsg(std::uint32_t partyId, std::uint32_t aboutId,
                         const std::string& text) {
  WorldEvent ev;
  ev.partyChanged = true;
  ev.target = partyId;   // 0 => aboutId left/was kicked (clear their frame)
  ev.aboutId = aboutId;
  ev.chatCh = 2;
  ev.chatText = text;
  events_.push_back(std::move(ev));
}

bool World::partyInvite(Entity& inviter, Entity& target) {
  if (inviter.kind != EntityKind::kPlayer || target.kind != EntityKind::kPlayer ||
      inviter.dead || target.dead || inviter.id == target.id) return false;
  if (target.zoneId != inviter.zoneId ||
      chebyshev(inviter.walker.tile(), target.walker.tile()) > 12) return false;
  if (target.partyId != 0) return false;
  // inviter must be unaffiliated (bootstrap) or the party leader
  Party* p = nullptr;
  if (inviter.partyId == 0) {
    Party np;
    np.id = nextPartyId_++;
    np.leaderId = inviter.id;
    np.members.push_back(inviter.id);
    parties_.push_back(np);
    inviter.partyId = np.id;
    p = &parties_.back();
    emitPartyMsg(p->id, inviter.id, inviter.name + " raises a hunting party.");
  } else {
    for (Party& q : parties_) if (q.id == inviter.partyId) { p = &q; break; }
    if (p == nullptr) { inviter.partyId = 0; return false; }   // stale id repair
    if (p->leaderId != inviter.id) return false;               // only the leader invites
  }
  if (static_cast<int>(p->members.size()) >= kPartyMaxMembers) return false;
  // one pending invite per invitee: renew/overwrite
  bool found = false;
  for (auto& inv : invites_) if (inv.first == target.id) {
    inv.second = {tick_ + 200, inviter.id};  // 10 s to answer
    found = true;
  }
  if (!found) invites_.push_back({target.id, {tick_ + 200, inviter.id}});
  emitPartyMsg(p->id, inviter.id, inviter.name + " invites " + target.name + ".");
  emitPartyMsg(p->id, target.id, target.name + ": a hand beckons — /accept.");
  return true;
}

bool World::partyAccept(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead || e.partyId != 0) return false;
  uint32_t inviterId = 0;
  for (size_t i = 0; i < invites_.size(); ++i) {
    if (invites_[i].first == e.id) {
      auto [expiry, who] = invites_[i].second;
      invites_.erase(invites_.begin() + static_cast<long>(i));
      if (tick_ <= expiry) inviterId = who;
      break;
    }
  }
  if (inviterId == 0) return false;
  Entity* inviter = find(inviterId);
  if (inviter == nullptr || inviter->partyId == 0) return false;
  Party* p = nullptr;
  for (Party& q : parties_) if (q.id == inviter->partyId) { p = &q; break; }
  if (p == nullptr || static_cast<int>(p->members.size()) >= kPartyMaxMembers) return false;
  if (e.duelWith != 0) {  // T-056: swearing into the circle ends hostilities
    if (Entity* t = find(e.duelWith)) { t->duelWith = 0; t->duelUntil = -1; }
    e.duelWith = 0;
    e.duelUntil = -1;
  }
  p->members.push_back(e.id);
  e.partyId = p->id;
  emitPartyMsg(p->id, e.id, e.name + " joins the party.");
  return true;
}

bool World::partyLeave(Entity& e) {
  if (e.partyId == 0) return false;
  const std::uint32_t pid = e.partyId;
  e.partyId = 0;
  for (size_t i = 0; i < parties_.size(); ++i) {
    Party& p = parties_[i];
    if (p.id != pid) continue;
    p.members.erase(std::remove(p.members.begin(), p.members.end(), e.id), p.members.end());
    emitPartyMsg(pid, e.id, e.name + " leaves the party.");
    if (p.members.empty()) {
      parties_.erase(parties_.begin() + static_cast<long>(i));
    } else if (p.leaderId == e.id) {
      p.leaderId = p.members.front();  // eldest in join order inherits the horn
      Entity* nl = find(p.leaderId);
      if (nl != nullptr)
        emitPartyMsg(pid, nl->id, nl->name + " takes the lead.");
    }
    return true;
  }
  return false;
}

bool World::partyKick(Entity& leader, std::uint32_t targetId) {
  if (leader.partyId == 0 || targetId == leader.id) return false;
  const Party* p = partyOf(leader.id);
  if (p == nullptr || p->leaderId != leader.id) return false;
  Entity* victim = find(targetId);
  if (victim == nullptr || victim->partyId != p->id) return false;
  return partyLeave(*victim);  // same path; the msg reads as a leave
}

const content::BountyDef* World::bountyNow() const {
  return content::bountyAt(static_cast<std::uint32_t>(tick_ / content::kBountyCycleTicks));
}

// T-065: assign/amend the quarry when a player stands near the Wanted Board.
// Amends only when the cycle moved (a fresh sheet gets pinned to the wall);
// otherwise confirms. Chat line names the price — era plain.
void World::bountyAssign(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead) return;
  bool nearBoard = false;
  for (const Entity& other : entities_) {
    if (other.wireKind == content::kWireKindBounty && other.zoneId == e.zoneId &&
        chebyshev(other.walker.tile(), e.walker.tile()) <= 3) {
      nearBoard = true;
      break;
    }
  }
  if (!nearBoard) return;
  const content::BountyDef* b = bountyNow();
  const std::uint32_t cyc = static_cast<std::uint32_t>(tick_ / content::kBountyCycleTicks);
  if (e.bountyMobId == b->mobId && e.bountyCycle == cyc) return;  // already marked
  e.bountyMobId = b->mobId;
  e.bountyCycle = cyc;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.chatCh = 255;
  const content::MobDef* md = content::findMob(b->mobId);
  ev.chatText = std::string("the board wants: ") + (md != nullptr ? md->name : "?") +
                " — " + std::to_string(b->payoutGold) + "g.";
  events_.push_back(std::move(ev));
}

void World::killMob(Entity& mob, Entity* killer) {
  const sim::TilePos mobPos = mob.walker.tile();  // before despawn below
  if (killer != nullptr && killer->kind == EntityKind::kPlayer) {
    // T-051: party XP share — alive members in the same zone within
    // kPartyXpRadius of the kill split evenly, +kPartyXpBonusPct per extra
    // sharer (the reason to group); loot/gold stay with the killer (era).
    std::vector<std::uint32_t> sharers;
    if (killer->partyId != 0) {
      if (const Party* p = partyOf(killer->id)) {
        for (const std::uint32_t mid : p->members) {
          Entity* m = find(mid);
          if (m != nullptr && !m->dead && m->zoneId == killer->zoneId &&
              chebyshev(m->walker.tile(), mobPos) <= kPartyXpRadius)
            sharers.push_back(mid);
        }
      }
    }
    const std::uint32_t nightXp = isNight() ? mob.xpValue * 110u / 100u
                                            : mob.xpValue;  // T-062
    if (sharers.size() <= 1) {
      awardXp(*killer, nightXp);
    } else {
      const std::uint32_t bonus = 100u +
          kPartyXpBonusPct * static_cast<std::uint32_t>(sharers.size() - 1);
      const std::uint32_t each = nightXp * bonus / 100u /
                                 static_cast<std::uint32_t>(sharers.size());
      for (const std::uint32_t mid : sharers) {
        Entity* m = find(mid);
        if (m != nullptr) {
          awardXp(*m, each);
          WorldEvent sev;
          sev.aboutId = mid;
          sev.chatCh = 2;
          sev.chatText = "party share: " + std::to_string(each) + " xp.";
          events_.push_back(std::move(sev));
          WorldEvent fx;  // T-066: xp shimmer floater over each sharer
          fx.attacker = killer->id;
          fx.target = mid;
          fx.kind = 12;
          fx.amount = static_cast<std::uint16_t>(
              std::min<std::uint32_t>(each, 65535));
          events_.push_back(std::move(fx));
        }
      }
    }
    const content::MobDef* md = content::findMob(mob.mobId);
    // T-078 guard-murder (M3 exit): the post is town law, not game. A player
    // who drops a guard stains like an unlawful PK against level 15 (GDD §5
    // shape, victim level pinned — no new numbers) and takes the same wanted
    // mark through the shared path (the victim's own anchor is within 0).
    // Mob-on-guard violence is not a crime (players only). No faction flags,
    // no permanent marks, vendor law unchanged beyond wanted refusal.
    if (md != nullptr && md->guard != 0) {
      const std::int32_t deficit = killer->level < 15 ? 15 - killer->level : 0;
      bumpKarma(*killer, -(300 + 20 * deficit));
      markWanted(*killer);
      WorldEvent txt;
      txt.aboutId = killer->id;
      txt.chatCh = 255;
      txt.chatText = killer->name + "'s hands are red with a Gate Guard's blood (-" +
                     std::to_string(300 + 20 * deficit) + " karma).";
      events_.push_back(std::move(txt));
    }
    // T-065 bounty treasurer: the held mark matches & still on-cycle -> pay out
    if (md != nullptr && killer->bountyMobId == mob.mobId &&
        killer->bountyCycle ==
            static_cast<std::uint32_t>(tick_ / content::kBountyCycleTicks)) {
      if (const content::BountyDef* b = bountyNow(); b->mobId == mob.mobId) {
        killer->gold += b->payoutGold;
        killer->bountyMobId = 0;  // one kill drains the sheet
        WorldEvent bev;
        bev.aboutId = killer->id;
        bev.statsChanged = true;
        bev.chatCh = 255;
        bev.chatText = std::string("bounty paid: +") + std::to_string(b->payoutGold) +
                       "g. The board sheet curls away.";
        events_.push_back(std::move(bev));
      }
    }
    if (md != nullptr) {
      // T-059 affixes v1: named-gear side-drop with a rolled one-liner mod
      if (const content::GearDropDef* gd = content::findGearDrop(md->mobId)) {
        const std::int64_t gearPct =  // T-062 nightcreep rides drops too
            isNight() ? static_cast<std::int64_t>(gd->chancePct) * 125 / 100
                      : static_cast<std::int64_t>(gd->chancePct);
        if (rng_.range(1, 100) <= gearPct) {
          if (killer->inv.size() < 32) {  // inventory cap (addItem rule)
            InvSlot sl;
            sl.itemId = gd->itemId;
            sl.qty = 1;
            sl.equipped = false;
            sl.aura = 0;
            sl.affix = static_cast<std::uint8_t>(rng_.range(1, content::kAffixCount));
            killer->inv.push_back(sl);
            WorldEvent gev;
            gev.aboutId = killer->id;
            gev.invChanged = true;
            gev.chatCh = 255;
            const content::ItemDef* id = content::findItem(gd->itemId);
            gev.chatText = std::string("looted ") + (id != nullptr ? id->name : "?") +
                           " " + content::kAffixNames[sl.affix] + ".";
            events_.push_back(std::move(gev));
          }
        }
      }
      // loot roll (junk tier for now; gear tables land with affixes in P3)
      const std::int64_t nightLootPct =  // T-062: +25% relative under dark
          isNight() ? static_cast<std::int64_t>(md->lootChancePct) * 125 / 100
                    : static_cast<std::int64_t>(md->lootChancePct);
      if (md->lootItemId != 0 &&
          rng_.range(1, 100) <= nightLootPct) {
        const content::ItemDef* item = content::findItem(md->lootItemId);
        if (item != nullptr && addItem(*killer, md->lootItemId, 1)) {
          WorldEvent ev;
          ev.aboutId = killer->id;
          ev.invChanged = true;
          ev.chatCh = 255;
          ev.chatText = std::string("looted 1x ") + item->name + ".";
          events_.push_back(std::move(ev));
        }
      }
      if (md->goldHi >= md->goldLo) {
        const std::uint32_t g =
            md->goldLo + static_cast<std::uint32_t>(rng_.range(
                             0, static_cast<std::int64_t>(md->goldHi) -
                                    static_cast<std::int64_t>(md->goldLo)));
        killer->gold += (killer->karma < 0) ? g * 115u / 100u : g;  // bad moral: richer drops
        WorldEvent ev2;
        ev2.aboutId = killer->id;
        ev2.statsChanged = true;
        events_.push_back(std::move(ev2));
      }
    }
    if (md != nullptr && killer != nullptr &&
        killer->kind == EntityKind::kPlayer && md->level <= killer->level) {
      bumpKarma(*killer, 1);  // whitening (T-056): farm at-level, whiten
    }
    // T-101 named-elite first blood: world-announced once per reboot.
    if (md != nullptr && killer != nullptr &&
        killer->kind == EntityKind::kPlayer) {
      const int ei = content::namedEliteIdx(mob.mobId);
      if (ei >= 0 && !namedEliteSlain_[ei]) {
        namedEliteSlain_[ei] = true;
        WorldEvent fev;
        fev.chatCh = 2;  // system broadcast (party-share/curse lane)
        fev.chatText = std::string(md->name) + " has fallen to " +
                       killer->name + " — first blood.";
        events_.push_back(std::move(fev));
      }
    }
  }
  // schedule the spawner refill (respawn gap = def.respawnTicks)
  Zone& mz = zoneOf(mob);
  if (mob.spawnerIdx < mz.spawners.size()) {
    SpawnerLive& sl = mz.spawners[mob.spawnerIdx];
    sl.respawnReadyAt = tick_ + static_cast<sim::Tick>(sl.def.respawnTicks);
  }
  const std::uint32_t mobEntId = mob.id;
  for (auto& e : entities_) {
    if (e.attackTarget == mobEntId) e.attackTarget = 0;  // stop stabbing a corpse
  }
  despawn(mobEntId);
}

// ---- alignment: karma core (T-056) -----------------------------------------
std::uint8_t World::karmaBandOf(std::int32_t karma) {
  if (karma < 0) return 2;        // chaotic — red name
  return karma > 500 ? 0 : 1;     // lawful / neutral
}

void World::bumpKarma(Entity& e, std::int32_t delta) {
  if (e.kind != EntityKind::kPlayer || delta == 0) return;
  const std::uint8_t before = karmaBandOf(e.karma);
  std::int32_t next = static_cast<std::int32_t>(e.karma) + delta;
  next = next > 1000 ? 1000 : next < -1000 ? -1000 : next;
  e.karma = next;
  const std::uint8_t after = karmaBandOf(e.karma);
  // band crossing is a public event: panel flag + crowd line (T-057 wire)
  if (after != before) {
    WorldEvent ev;
    ev.aboutId = e.id;
    ev.statsChanged = true;
    ev.bandChanged = true;   // server re-spawns the entity for AoI (red name)
    ev.chatCh = 255;
    ev.chatText = e.name + (after > before ? "'s soul whitens."
                                           : "'s soul blackens.");
    events_.push_back(std::move(ev));
  }
}

bool World::duelChallenge(Entity& e, std::uint32_t targetId) {
  Entity* t = find(targetId);
  if (t == nullptr || t->kind != EntityKind::kPlayer || t->dead || e.dead ||
      t->id == e.id || t->zoneId != e.zoneId ||
      chebyshev(e.walker.tile(), t->walker.tile()) > 12) return false;
  // accept: target already offered me a duel within the window
  if (t->duelOfferTo == e.id && tick_ - t->duelOfferAt <= 400) {
    t->duelOfferTo = 0;
    e.duelWith = t->id;
    t->duelWith = e.id;
    e.duelUntil = tick_ + 12000;  // 10 min cap; usually ends at the kill
    t->duelUntil = e.duelUntil;
    WorldEvent txt;
    txt.chatCh = 3;
    txt.chatText = e.name + " duels " + t->name + ". No law watches.";
    events_.push_back(std::move(txt));
    return true;
  }
  // otherwise: this is the offer
  e.duelOfferTo = targetId;
  e.duelOfferAt = tick_;
  WorldEvent txt;
  txt.aboutId = e.id;
  txt.chatCh = 3;
  txt.chatText = e.name + " calls out " + t->name + " (/duel back to accept).";
  events_.push_back(std::move(txt));
  return true;
}

bool World::duelForfeit(Entity& e) {
  Entity* t = e.duelWith != 0 ? find(e.duelWith) : nullptr;
  if (t == nullptr) {  // no duel: a stale offer can still be withdrawn
    if (e.duelOfferTo == 0) return false;
    e.duelOfferTo = 0;
    return true;
  }
  t->duelWith = 0;
  t->duelUntil = -1;
  e.duelWith = 0;
  e.duelUntil = -1;
  WorldEvent txt;
  txt.chatCh = 3;
  txt.chatText = e.name + " forfeits the duel.";
  events_.push_back(std::move(txt));
  return true;
}

// chaotic bindstone: town-edge tile; deterministic walkable search, bindstone
// fallback when the map gives us nothing (era: the pit is a fiction address,
// not a map feature yet — T-063 authors the real gallows).
sim::TilePos World::gallowsTile(std::uint16_t zoneId) const {
  const Zone& z = zones_.at(zoneId);
  static constexpr int ring[][2] = {{2,0},{0,2},{-2,0},{0,-2},{2,2},{-2,-2},
                                    {2,-2},{-2,2},{3,0},{0,3},{-3,0},{0,-3}};
  for (const auto& o : ring) {
    const sim::TilePos t{z.spawnPoint.x + o[0], z.spawnPoint.y + o[1]};
    if (z.map.inBounds(t.x, t.y) && !z.map.isBlocked(t.x, t.y)) return t;
  }
  return z.spawnPoint;  // degenerate map: era says the square then
}

void World::killPlayer(Entity& victim, Entity* killer) {
  victim.dead = true;
  victim.attackTarget = 0;
  victim.path.clear();
  victim.hp = 0;
  victim.respawnAt = tick_ + kPlayerRespawnTicks;
  for (auto& e : entities_) {
    if (e.attackTarget == victim.id) {
      e.attackTarget = 0;
      e.slamAt = -1;  // T-091: death fizzles the wind-up aimed at the mark
    }
  }

  // T-056 PK law: consensual duels carry no karma/debt/drops; unlawful kills
  // of lawful/neutral stain the killer; chaotic victims are lawful prey.
  const bool duel = killer != nullptr &&
                    killer->kind == EntityKind::kPlayer &&
                    killer->duelWith == victim.id && tick_ < killer->duelUntil &&
                    victim.duelWith == killer->id;
  if (duel) {
    // end the duel with a standing victor
    killer->duelWith = 0;
    killer->duelUntil = -1;
    victim.duelWith = 0;
    victim.duelUntil = -1;
    WorldEvent txt;
    txt.chatCh = 3;
    txt.chatText = "the duel is done — " + killer->name + " stands.";
    events_.push_back(std::move(txt));
    return;  // no XP debt below, no drops
  }
  if (killer != nullptr && killer->kind == EntityKind::kPlayer &&
      victim.duelWith == 0 && karmaBandOf(victim.karma) != 2) {
    const std::int32_t deficit =
        killer->level > victim.level ? killer->level - victim.level : 0;
    bumpKarma(*killer, -(300 + 20 * deficit));  // GDD §5 formula
    WorldEvent txt;
    txt.aboutId = killer->id;
    txt.chatCh = 255;
    txt.chatText = killer->name + "'s hands are red with " + victim.name +
                   "'s blood (-" + std::to_string(300 + 20 * deficit) + " karma).";
    events_.push_back(std::move(txt));
  // T-073 gate law: unlawful PK within 8 tiles of a guard anchor marks the
  // killer wanted for 240 s (guards aggro, vendors refuse, death binds at
  // the gallows). Deterministic: anchor scan order, first post that sees.
  for (const Entity& g : entities_) {
    if (!isGuardMob(g) || g.zoneId != killer->zoneId) continue;
    if (chebyshev(killer->walker.tile(), g.anchor) <= 8) {
      markWanted(*killer);
      break;
    }
  }
}
  // duel partner walks away if the mob kills happen mid-duel (cleanup)
  if (victim.duelWith != 0 && !duel) {
    if (Entity* t = find(victim.duelWith)) { t->duelWith = 0; t->duelUntil = -1; }
    victim.duelWith = 0;
    victim.duelUntil = -1;
  }

  // T-056 chaotic death: the crowd picks the corpse (1-6 items + 15%/slot)
  if (karmaBandOf(victim.karma) == 2) {
    int dropped = 0;
    // equipped slots: independent 15% rolls
    for (size_t i = 0; i < victim.inv.size();) {
      if (victim.inv[i].equipped && rng_.range(1, 100) <= 15) {
        victim.inv.erase(victim.inv.begin() + static_cast<long>(i));
        ++dropped;
      } else {
        ++i;
      }
    }
    // non-equipped: 1..6 picks without replacement
    std::vector<size_t> bag;
    for (size_t i = 0; i < victim.inv.size(); ++i) bag.push_back(i);
    const int want = static_cast<int>(rng_.range(1, 6));
    for (int k = 0; k < want && !bag.empty(); ++k) {
      const size_t pick = static_cast<size_t>(rng_.range(0, static_cast<std::int64_t>(bag.size() - 1)));
      const size_t idx = bag[pick];
      bag.erase(bag.begin() + static_cast<long>(pick));
      victim.inv[idx].itemId = 0;  // tomb-empty slot; compact pass below
      victim.inv[idx].qty = 0;
      ++dropped;
    }
    // compact tomb-empties
    for (size_t i = 0; i < victim.inv.size();) {
      if (victim.inv[i].itemId == 0)
        victim.inv.erase(victim.inv.begin() + static_cast<long>(i));
      else ++i;
    }
    WorldEvent txt;
    txt.aboutId = victim.id;
    txt.chatCh = 255;
    txt.invChanged = true;
    txt.chatText = "the crowd picks the corpse: " + std::to_string(dropped) +
                   " item(s) gone.";
    events_.push_back(std::move(txt));
  }
  // T-081 death wear (GDD §7: -5 durability on death). Drops first (above),
  // wear second: every surviving gear slot loses 5, floor 0 (dormant per
  // T-058, never destroyed). Junk/consumables untouched. All victims —
  // lawful rusts the same as red.
  for (InvSlot& sl : victim.inv) {
    const content::ItemDef* dd = content::findItem(sl.itemId);
    if (dd == nullptr || dd->slot > 1) continue;
    sl.durability = sl.durability <= 5 ? 0 : static_cast<std::uint8_t>(sl.durability - 5);
  }
  // GDD: XP debt 10% of bar at L1 rising to 25% at L25; de-level at 0 XP.
  const std::uint32_t bar = sim::xpNext(victim.level);
  if (bar > 0 || victim.level > 1) {
    const std::uint32_t pct = 10u + (static_cast<std::uint32_t>(victim.level) - 1) * 15u / 24u;
    const std::uint32_t refBar = bar > 0 ? bar : sim::xpNext(24);
    std::int64_t xpv = static_cast<std::int64_t>(victim.xp) -
                       static_cast<std::int64_t>(refBar * pct / 100u);
    bool deleveled = false;
    while (xpv < 0 && victim.level > 1) {
      --victim.level;
      xpv += sim::xpNext(victim.level);
      deleveled = true;
    }
    if (xpv < 0) xpv = 0;
    victim.xp = static_cast<std::uint32_t>(xpv);
    victim.hpMax = recomputeHpMax(victim);
    WorldEvent ev;
    ev.aboutId = victim.id;
    ev.statsChanged = true;
    ev.chatCh = 255;
    ev.chatText = deleveled
                      ? "the debt breaks you: de-level to " + std::to_string(victim.level) + "."
                      : "death debt: -" +
                            std::to_string(refBar * pct / 100u) + " XP.";
    events_.push_back(std::move(ev));
  }
  // S7: corpse marker entity for Cultist Resurrect anchor (P3).
}

void World::awardXp(Entity& player, std::uint32_t amount) {
  if (player.level >= sim::kLevelCap) return;
  // moral split (T-046, threshold corrected to GDD §5 in T-056): only the
  // LAWFUL band (>500) earns the +15% XP carrot; neutral gets nothing extra,
  // chaotic eats gold not XP (gold boost site is the loot roll).
  if (player.karma > 500) amount = amount * 115u / 100u;
  player.xp += amount;
  while (player.level < sim::kLevelCap && player.xp >= sim::xpNext(player.level)) {
    player.xp -= sim::xpNext(player.level);
    ++player.level;
    player.statPoints += sim::kStatPointsPerLevel;
    const std::uint32_t newMax = recomputeHpMax(player);
    player.hpMax = newMax;
    player.hp = newMax;  // level-up full heal (era: ding restores)
    player.mpMax = 30u + 6u * (player.level - 1u);
    player.mp = player.mpMax;
    WorldEvent ding;
    ding.aboutId = player.id;
    ding.statsChanged = true;
    ding.chatCh = 2;
    ding.chatText = player.name + " reaches level " + std::to_string(player.level) + ".";
    events_.push_back(std::move(ding));
  }
  WorldEvent ev;
  ev.aboutId = player.id;
  ev.statsChanged = true;
  events_.push_back(std::move(ev));
}

// T-071: a night-bound mob (nightOnly spawner) does not acquire targets by
// day. Held targets are kept (only the acquire act is gated); gm/debug
// spawns (spawnerIdx SIZE_MAX) are always active.
bool World::mobNightDormant(const Entity& mob) const {
  if (mob.spawnerIdx == SIZE_MAX) return false;
  const auto zit = zones_.find(mob.zoneId);
  if (zit == zones_.end()) return false;
  if (mob.spawnerIdx >= zit->second.spawners.size()) return false;
  return zit->second.spawners[mob.spawnerIdx].def.nightOnly != 0 && !isNight();
}

// T-073: gate-guard lookup — a guard-flagged mob def on a real mob body
// (vendors/boards/fences share kind kMob but carry mobId 0: never guards).
bool World::isGuardMob(const Entity& mob) {
  if (mob.kind != EntityKind::kMob) return false;
  if (content::wireIsFurniture(mob.wireKind)) return false;
  const content::MobDef* def = content::findMob(mob.mobId);
  return def != nullptr && def->guard != 0;
}

// T-073/T-078: the one wanted-mark path. Stamp + fiction; callers own the
// condition (anchor scan for PK, victim identity for guard-murder).
void World::markWanted(Entity& killer) {
  killer.wantedUntil = tick_ + 4800;
  WorldEvent wtxt;
  wtxt.aboutId = killer.id;
  wtxt.chatCh = 2;
  wtxt.chatText = killer.name + " is WANTED at the gates (240 s).";
  events_.push_back(std::move(wtxt));
}

void World::slamStrike(Entity& mob) {
  // T-091: the fuse burned down — rot blooms on the recorded tile. Whoever
  // stands there eats it (bolt-mirror numbers: dmg stat, half-plate, night
  // bite, thin-blood curse on survivors); whoever moved dodges clean.
  mob.slamAt = -1;
  const content::MobDef* bd = content::findMob(mob.mobId);
  if (bd == nullptr) return;
  std::vector<Entity*> caught =
      playersNear(zoneOf(mob), mob.slamX, mob.slamY, kSlamRadius);
  for (Entity* v : caught) {
    if (v->kind != EntityKind::kPlayer || v->dead) continue;
    std::uint32_t sdmg =
        sim::rollDamage(bd->dmg, 0, effDef(*v) / 2u, false);
    if (isNight()) sdmg = sdmg * 125u / 100u;  // same dark bite as the bolt
    sdmg = sdmg < 1 ? 1 : sdmg;
    v->hp = sdmg >= static_cast<std::uint32_t>(v->hp)
                ? 0
                : v->hp - static_cast<std::int32_t>(sdmg);
    if (v->hp > 0) {
      v->curseUntil = tick_ + kCurseTicks;  // rot gets in the blood (T-070 lane)
      WorldEvent cev;
      cev.aboutId = v->id;
      cev.chatCh = 2;
      cev.chatText =
          v->name + " feels the blood curse take hold (-25% healing).";
      events_.push_back(std::move(cev));
    }
    WorldEvent ev;
    ev.attacker = mob.id;
    ev.target = v->id;
    ev.kind = 16;  // T-091: slam strike (client red-caps it)
    ev.amount = sdmg;
    ev.statsChanged = true;
    events_.push_back(std::move(ev));
    if (v->hp == 0) killPlayer(*v, &mob);
  }
}

void World::mobThink(Entity& mob) {
  if (mob.dead || content::wireIsFurniture(mob.wireKind)) return;  // vendors stand eternally  // leash: too far from anchor -> drop target and path home, no re-aggro
  // T-091: a wind-up with no mark is a dud — leash breaks, logouts, deaths,
  // and anything else that clears attackTarget must not leave an armed fuse
  // behind to strike a stale tile.
  if (mob.attackTarget == 0) mob.slamAt = -1;
  const sim::TilePos p = mob.walker.tile();
  const bool leashed = chebyshev(p, mob.anchor) > mob.leashRadius;
  if (leashed) {
    mob.attackTarget = 0;
    if (mob.path.empty() && !mob.walker.moving && p != mob.anchor) {
      const sim::PathResult res = sim::findPath(zoneOf(mob).grid, p, mob.anchor);
      if (res.found) mob.path.assign(res.tiles.begin(), res.tiles.end());
    }
    return;
  }
  if (mob.attackTarget == 0 && p == mob.anchor && mob.hp < mob.hpMax) {
    // resting heals 2%/tick up at the den (Lineage-style soft reset)
    mob.hp += std::max<std::uint32_t>(1, mob.hpMax / 50);
    if (mob.hp > mob.hpMax) mob.hp = mob.hpMax;
  }
  if (mob.attackTarget == 0 && p == mob.anchor) {
    mob.firstHurtTick = -1;  // metric hygiene: TTK window starts on re-aggression
  }

  // player attacked me? handled in trySwing. Acquire aggro:
  // T-073 gate guards: wanted-only acquire inside leash reach (aggro 0 by
  // design — the post ignores the innocent). Spawn protection (M4) skips
  // every mob lookup; retaliation on being struck still fires (no free hits).
  if (mob.attackTarget == 0 && isGuardMob(mob) &&
      mob.id % kAggroThinkPeriod == static_cast<std::uint32_t>(tick_ % kAggroThinkPeriod)) {
    std::vector<Entity*> near = playersNear(zoneOf(mob), p.x, p.y, mob.leashRadius);
    for (Entity* c : near) {
      if (c->kind != EntityKind::kPlayer) continue;
      if (tick_ < c->wantedUntil && tick_ >= c->spawnProtectUntil) {
        mob.attackTarget = c->id;  // deterministic: spatial order
        mob.path.clear();
        break;
      }
    }
  }
  // T-071: night-bound mobs do not acquire by day.
  if (mob.attackTarget == 0 && mob.aggroRadius > 0 && !mobNightDormant(mob) &&
      mob.id % kAggroThinkPeriod == static_cast<std::uint32_t>(tick_ % kAggroThinkPeriod)) {
    // T-061 nightcreep: dark eyes see one tile further
    const std::uint8_t effAggro =
        static_cast<std::uint8_t>(std::min(255, mob.aggroRadius + (isNight() ? 1 : 0)));
    std::vector<Entity*> near = playersNear(zoneOf(mob), p.x, p.y, effAggro);
    // T-073 M4: fresh/respawned players are invisible to the lookup for 5 s.
    near.erase(std::remove_if(near.begin(), near.end(),
                              [&](Entity* c) { return tick_ < c->spawnProtectUntil; }),
               near.end());
    if (!near.empty()) {
      mob.attackTarget = near[0]->id;  // deterministic: spatial order
      mob.path.clear();
    }
  }

  if (mob.attackTarget != 0) {
    Entity* target = find(mob.attackTarget);    if (target == nullptr || target->dead ||
        chebyshev(target->walker.tile(), mob.anchor) > mob.leashRadius) {
      mob.attackTarget = 0;
      mob.slamAt = -1;  // T-091: losing the mark cancels the wind-up
      return;
    }
    // T-073: the post stands down when the name clears — guards hold no
    // grudges past expiry (normal mobs keep their held target).
    if (isGuardMob(mob) && target->kind == EntityKind::kPlayer &&
        tick_ >= target->wantedUntil) {
      mob.attackTarget = 0;
      return;
    }
    const int d = chebyshev(p, target->walker.tile());
    // T-091 Gravemother telegraphed slam: an armed wind-up holds the boss
    // still (readable) and strikes when the 60t fuse burns down.
    if (mob.mobId == 1009 && mob.slamAt >= 0) {
      if (tick_ >= mob.slamAt) slamStrike(mob);
      return;
    }
    // T-064 Blood Bolt: boss casts at range instead of padding into melee
    if (const content::MobDef* bd = content::findMob(mob.mobId);
        bd != nullptr && bd->boss && bd->boltRange > 0 && d > 1 &&
        d <= bd->boltRange &&
        tick_ - mob.lastSwingTick >= static_cast<sim::Tick>(bd->boltCdTicks)) {
      // T-091: the Mother's bolt is a telegraphed slam, not instant rot —
      // arm the wind-up on the shared cadence and let the client stage it.
      if (mob.mobId == 1009 && mob.slamAt < 0) {
        mob.slamAt = tick_ + kSlamWindupTicks;
        mob.slamX = target->walker.tile().x;
        mob.slamY = target->walker.tile().y;
        mob.lastSwingTick = tick_;  // telegraph tax: the fuse eats the cast
        WorldEvent ev;
        ev.attacker = mob.id;
        ev.target = target->id;
        ev.kind = 15;  // T-091: telegraph wind-up (client stages 1-2-3)
        ev.amount = static_cast<std::uint32_t>(kSlamRadius);
        events_.push_back(std::move(ev));
        return;
      }
      mob.lastSwingTick = tick_;  // shared cadence, distinct cast
      std::uint32_t bdmg = sim::rollDamage(bd->dmg, 0,
          target->kind == EntityKind::kPlayer ? effDef(*target) / 2u : 0u, false);
      if (isNight()) bdmg = bdmg * 125u / 100u;  // Blood Bolt bites +25% after dark
      bdmg = bdmg < 1 ? 1 : bdmg;
      target->hp = bdmg >= static_cast<std::uint32_t>(target->hp)
                       ? 0 : target->hp - static_cast<std::int32_t>(bdmg);
      // T-070 Blood Curse: the bolt leaves thin blood — heals land at 75%
      // for 30 s. Gravecaller and Gravemother share this cast path.
      if (target->kind == EntityKind::kPlayer && target->hp > 0) {
        target->curseUntil = tick_ + kCurseTicks;
        WorldEvent cev;
        cev.aboutId = target->id;
        cev.chatCh = 2;
        cev.chatText = target->name +
                       " feels the blood curse take hold (-25% healing).";
        events_.push_back(std::move(cev));
      }
      WorldEvent ev;
      ev.attacker = mob.id;
      ev.target = target->id;
      ev.kind = 9;  // T-064: Blood Bolt ranged callout
      ev.amount = bdmg;
      ev.statsChanged = true;
      events_.push_back(std::move(ev));
      if (target->hp == 0 && target->kind == EntityKind::kPlayer)
        killPlayer(*target, &mob);
      return;
    }
    if (d <= 1) {
      mob.path.clear();
      trySwing(mob, *target);
    } else if (mob.path.empty() &&
               mob.id % kAggroThinkPeriod == static_cast<std::uint32_t>(tick_ % kAggroThinkPeriod)) {
      if (target->zoneId != mob.zoneId) { mob.attackTarget = 0; return; }
      const sim::TilePos start = mob.walker.moving ? mob.walker.target : p;
      const sim::PathResult res = sim::findPath(zoneOf(mob).grid, start, target->walker.tile());
      if (res.found) mob.path.assign(res.tiles.begin(), res.tiles.end());
    }
    return;
  }

  // idle wander (small chance per think slice)
  if (!mob.walker.moving && mob.path.empty() &&
      mob.id % kAggroThinkPeriod == static_cast<std::uint32_t>(tick_ % kAggroThinkPeriod) &&
      rng_.chance(0.25)) {
    const int dx = static_cast<int>(rng_.range(-2, 2));
    const int dy = static_cast<int>(rng_.range(-2, 2));
    const sim::TilePos goal{p.x + dx, p.y + dy};
    const Zone& wz = zoneOf(mob);
    if (wz.map.inBounds(goal.x, goal.y) && !wz.map.isBlocked(goal.x, goal.y) &&
        chebyshev(goal, mob.anchor) <= mob.wanderRadius) {
      const sim::PathResult res = sim::findPath(wz.grid, mob.walker.tile(), goal);
      if (res.found) mob.path.assign(res.tiles.begin(), res.tiles.end());
    }
  }
}

void World::respawnTick() {
  // mob refill (one per spawner per check), zones independent
  for (auto& [zid, zone] : zones_) {
    for (size_t i = 0; i < zone.spawners.size(); ++i) {
      SpawnerLive& sl = zone.spawners[i];
      if (tick_ < sl.respawnReadyAt) continue;
      // T-071: night-bound spawners refill only inside the night window.
      if (sl.def.nightOnly != 0 && !isNight()) continue;
      std::uint32_t alive = 0;
      for (const auto& e : entities_) {
        if (e.kind == EntityKind::kMob && e.zoneId == zid && e.spawnerIdx == i) ++alive;
      }
      if (alive >= sl.def.maxAlive) continue;
      const content::MobDef* def = content::findMob(sl.def.mobId);
      if (def == nullptr) continue;
      sl.respawnReadyAt = tick_ + static_cast<sim::Tick>(sl.def.respawnTicks);
      for (int tries = 0; tries < 20; ++tries) {
        const int x = sl.def.x + static_cast<int>(rng_.range(0, sl.def.w - 1));
        const int y = sl.def.y + static_cast<int>(rng_.range(0, sl.def.h - 1));
        if (zone.map.inBounds(x, y) && !zone.map.isBlocked(x, y)) {
          spawnMob(*def, sim::TilePos{x, y}, i, zid);
          break;
        }
      }
    }
  }
  // player respawns
  for (auto& e : entities_) {
    if (e.kind == EntityKind::kPlayer && e.dead && tick_ >= e.respawnAt) {
      e.dead = false;
      e.hp = e.hpMax;
      // death binds at the town bindstone — unless the name is red: the
      // gallows pit takes the chaotic (T-056, phase-3 bindstone rule).
      // T-073: the wanted bind at the gallows even with clean karma.
      const bool gallowsBound =
          karmaBandOf(e.karma) == 2 || tick_ < e.wantedUntil;
      const sim::TilePos home_ =
          gallowsBound ? gallowsTile(1) : zones_.at(1).spawnPoint;
      if (e.zoneId != 1) {
        Zone& cz = zones_.at(e.zoneId);
        cz.spatial.remove(e.id);
        e.zoneId = 1;
        Zone& home = zones_.at(1);
        e.walker.place(home_);
        home.spatial.insert(e.id, home.spawnPoint.x, home.spawnPoint.y);
        e.lastPortalTick = tick_;
        WorldEvent zev;
        zev.aboutId = e.id;
        zev.zoneChanged = true;
        events_.push_back(std::move(zev));
      } else {
        Zone& rz = zones_.at(1);
        e.walker.place(home_);
        rz.spatial.move(e.id, home_.x, home_.y);
      }
      WorldEvent ev;
      ev.aboutId = e.id;
      ev.statsChanged = true;
      ev.chatCh = 2;
      ev.chatText = e.name + " crawls back from the brink.";
      events_.push_back(std::move(ev));
      // T-073 M4: the respawned walks 5 s unseen (corpse-camp breaker).
      e.spawnProtectUntil = tick_ + 100;
    }
  }
}

void World::tick() {
  ++tick_;
  events_.clear();

  for (auto& e : entities_) {
    const sim::TilePos before = e.walker.tile();
    if (!e.dead) e.walker.step();
    const sim::TilePos after = e.walker.tile();
    if (after != before) {
      zones_.at(e.zoneId).spatial.move(e.id, after.x, after.y);
    }
    // T-071: torches burn down on the tick edge (lanterns never do).
    if (e.lightUntil >= 0 && tick_ >= e.lightUntil) {
      e.lightRadius = 0;
      e.lightUntil = -1;
    }
    // portal fire: on arrival/settled (not every tick: world-transfer is chunky)
    if (e.kind == EntityKind::kPlayer && !e.dead && !e.walker.moving && e.path.empty()) {
      checkPortals(e);
    }
    // T-065: the board reads passively at ~0.5Hz per player while standing around
    if (e.kind == EntityKind::kPlayer && !e.dead &&
        (tick_ + e.id) % 40 == 0) {
      bountyAssign(e);
    }
  }

  for (auto it = entities_.begin(); it != entities_.end(); ++it) {
    Entity& e = *it;
    // trade auto-cancel (T-029): partner gone/far/dead
    if (e.tradeWith != 0) {
      Entity* b = find(e.tradeWith);
      if (e.dead || b == nullptr || b->dead ||
          chebyshev(e.walker.tile(), b->walker.tile()) > 3) {
        tradeCancel(e, "partner out of reach");
      }
    }
  }

  for (auto& e : entities_) {
    if (e.walker.moving || e.path.empty() || e.dead) continue;
    const sim::TilePos cur = e.walker.tile();
    while (!e.path.empty()) {
      const sim::TilePos t = e.path.front();
      if (t == cur) {
        e.path.pop_front();
        continue;
      }
      if (e.walker.beginStep(zoneOf(e).grid, t)) {
        e.path.pop_front();
      } else {
        e.path.clear();
      }
      break;
    }
  }

  // mob brains + mutual swings
  const size_t n = entities_.size();  // spawns during tick are deferred harmlessly
  for (size_t i = 0; i < n && i < entities_.size(); ++i) {
    Entity& e = entities_[i];
    if (e.kind == EntityKind::kMob) mobThink(e);
  }

  // player attack intents
  for (size_t i = 0; i < entities_.size(); ++i) {
    Entity& e = entities_[i];
    if (e.kind != EntityKind::kPlayer || e.attackTarget == 0 || e.dead) continue;
    Entity* target = find(e.attackTarget);
    if (target == nullptr || target->dead) {
      e.attackTarget = 0;
      continue;
    }
    const sim::TilePos p = e.walker.tile();
    const int d = chebyshev(p, target->walker.tile());
    if (d <= 1) {
      e.path.clear();
      trySwing(e, *target);
    } else if (d <= 12 && tick_ - e.lastChaseTick >= 8) {
      // era-sticky pursuit: re-path the moving mark (rate-limited)
      e.lastChaseTick = tick_;
      const sim::TilePos start = e.walker.moving ? e.walker.target : p;
      const sim::PathResult res = sim::findPath(zoneOf(e).grid, start, target->walker.tile());
      if (res.found) {
        // drop any prefix tiles we'd immediately step out of
        e.path.assign(res.tiles.begin(), res.tiles.end());
      }
    } else if (d > 12) {
      e.attackTarget = 0;  // gave up the chase
    }
  }

  // alignment whitening (T-056): +1 karma per 3 600 ticks online
  if (tick_ % 3600 == 0) {
    for (auto& e : entities_)
      if (e.kind == EntityKind::kPlayer && !e.dead) bumpKarma(e, 1);
  }

  // mana regen: 1 MP / tick (100/min), always on — mana is the kit's
  // rhythm resource, not a second hp bar (GDD: MAG economy arrives w/ INT).
  for (auto& e : entities_) {
    if (e.kind == EntityKind::kPlayer && !e.dead && e.mp < e.mpMax &&
        tick_ % 20 == 0) ++e.mp;  // 1 MP / s
  }

  // out-of-combat player regen (Blood Wed aura II accelerates: +N% hpMax / period)
  for (auto& e : entities_) {
    if (e.kind != EntityKind::kPlayer || e.dead || e.hp >= e.hpMax) continue;
    if (tick_ - e.lastHurtTick > kOocRegenDelay && tick_ % kOocRegenPeriod == 0) {
      ++e.hp;
      std::uint8_t aura = 0;
      for (const InvSlot& sl : e.inv) {
        if (sl.equipped && sl.aura >= 2) aura = sl.aura;
      }
      if (aura >= 2) {
        if (const content::AuraTier* t = content::findAuraTier(aura);
            t != nullptr && t->regenPct60t > 0) {
          e.hp += e.hpMax * t->regenPct60t / 100u + 1;
          if (e.hp > e.hpMax) e.hp = e.hpMax;
        }
      }
    }
  }

  respawnTick();
}

// ---- portals & zone transfer (T-036) --------------------------------------

bool World::transferToZone(Entity& e, std::uint16_t mapId, sim::TilePos at) {
  auto it = zones_.find(mapId);
  if (it == zones_.end() || e.kind != EntityKind::kPlayer || e.dead) return false;
  Zone& to = it->second;
  sim::TilePos pos = at;
  if (!to.map.inBounds(pos.x, pos.y) || to.map.isBlocked(pos.x, pos.y)) {
    pos = to.spawnPoint;
  }
  if (e.duelWith != 0) {  // T-056: leaving the field ends the duel
    if (Entity* t = find(e.duelWith)) { t->duelWith = 0; t->duelUntil = -1; }
    e.duelWith = 0;
    e.duelUntil = -1;
  }
  zoneOf(e).spatial.remove(e.id);
  e.zoneId = mapId;
  e.walker.place(pos);
  e.path.clear();
  e.attackTarget = 0;
  to.spatial.insert(e.id, pos.x, pos.y);
  e.lastPortalTick = tick_;
  WorldEvent ev;
  ev.aboutId = e.id;
  ev.zoneChanged = true;
  ev.chatCh = 255;
  ev.chatText = "the air changes - another hall answers your steps.";
  events_.push_back(std::move(ev));
  return true;
}

bool World::checkPortals(Entity& e) {
  if (e.kind != EntityKind::kPlayer || e.dead || e.walker.moving) return false;
  if (tick_ - e.lastPortalTick < 40) return false;  // just arrived elsewhere
  const sim::TilePos p = e.walker.tile();
  const sim::Map& m = mapOf(e);
  for (const sim::PortalDef& pd : m.portals) {
    if (p.x >= pd.x && p.x <  pd.x + std::max(1, pd.w) &&
        p.y >= pd.y && p.y <  pd.y + std::max(1, pd.h)) {
      return transferToZone(e, static_cast<std::uint16_t>(pd.targetMapId),
                            sim::TilePos{pd.targetX, pd.targetY});
    }
  }
  return false;
}

std::vector<std::uint32_t> World::queryAoi(std::uint16_t zoneId, int x, int y,
                                           int radiusTiles) const {
  const auto it = zones_.find(zoneId);
  if (it == zones_.end()) return {};
  const auto raw = it->second.spatial.query(x - radiusTiles, y - radiusTiles,
                                            x + radiusTiles, y + radiusTiles);
  std::vector<std::uint32_t> out;
  out.reserve(raw.size());
  for (const std::uint32_t id : raw) {
    const Entity* e = find(id);
    if (e == nullptr) continue;
    const sim::TilePos p = e->walker.tile();
    const int dx = std::abs(p.x - x);
    const int dy = std::abs(p.y - y);
    if (dx <= radiusTiles && dy <= radiusTiles) out.push_back(id);
  }
  return out;
}

std::uint64_t World::worldHash() const {
  std::uint64_t h = 1469598103934665603ULL;
  auto mix = [&h](std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
      h ^= static_cast<std::uint8_t>(v >> (8 * i));
      h *= 1099511628211ULL;
    }
  };
  for (const auto& e : entities_) {
    mix(e.id);
    mix(e.zoneId);
    mix(static_cast<std::uint32_t>(e.walker.x));
    mix(static_cast<std::uint32_t>(e.walker.y));
    mix(e.hp);
  }
  return h;
}

}  // namespace bh::server
