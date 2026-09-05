// bh_mapconv: Tiled JSON (.tmj) -> BLOODHOLLOW binary map (.bhmap v1).
// Also the map validator gate in CI (--validate).
// Convention (see docs/03-architecture.md section 7):
//   tile layer  "ground"   gid-1 = terrain type (0 = grass)
//   tile layer  "blockers" nonzero = blocked (if absent: derived from terrain {2,3})
//   tile layer  "zones"    gid-1 = zone id (optional)
//   object layer "spawns"  props: mobId (required), maxAlive=3, respawnTicks=1200
//   object layer "portals" props: targetMapId (required), targetX, targetY
// Object rectangles use pixel coordinates on the tile grid (tile = x / tilewidth).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "sim/bhmap.h"

namespace {

void usage() {
  std::cerr
      << "usage: bh_mapconv <input.tmj> <output.bhmap> [--map-id N] [--validate]\n";
}

std::uint32_t maskGid(double gidRaw) {
  return static_cast<std::uint32_t>(gidRaw) & 0x1FFFFFFFu;
}

const nlohmann::json* findTileLayer(const nlohmann::json& root, const char* name) {
  for (const auto& l : root.at("layers")) {
    if (l.value("type", "") == "tilelayer" && l.value("name", "") == name) return &l;
  }
  return nullptr;
}

double propNum(const nlohmann::json& obj, const char* key, double dflt) {
  const auto it = obj.find("properties");
  if (it != obj.end() && it->is_array()) {
    for (const auto& p : *it) {
      if (p.value("name", "") == key && p.contains("value") && p["value"].is_number()) {
        return p["value"].get<double>();
      }
    }
  }
  return dflt;
}

int tileOf(double px, int tileSize) {
  return static_cast<int>(std::floor(px / static_cast<double>(tileSize)));
}

int spanOf(double px, int tileSize) {
  const int s = static_cast<int>(std::ceil(px / static_cast<double>(tileSize)));
  return s > 0 ? s : 1;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    usage();
    return 2;
  }
  const std::string inPath = argv[1];
  const std::string outPath = argv[2];
  bool validate = false;
  std::uint32_t mapId = 0;
  for (int i = 3; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--validate") {
      validate = true;
    } else if (a == "--map-id" && i + 1 < argc) {
      mapId = static_cast<std::uint32_t>(std::stoul(argv[++i]));
    } else {
      usage();
      return 2;
    }
  }

  nlohmann::json j;
  try {
    std::ifstream f(inPath);
    if (!f) {
      std::cerr << "bh_mapconv: cannot open " << inPath << "\n";
      return 1;
    }
    f >> j;
  } catch (const std::exception& e) {
    std::cerr << "bh_mapconv: JSON parse error in " << inPath << ": " << e.what() << "\n";
    return 1;
  }

  bh::sim::Map m;
  try {
    m.w = j.at("width").get<int>();
    m.h = j.at("height").get<int>();
    m.tileW = j.at("tilewidth").get<int>();
    m.tileH = j.at("tileheight").get<int>();
  } catch (const std::exception& e) {
    std::cerr << "bh_mapconv: missing map fields: " << e.what() << "\n";
    return 1;
  }
  if (m.w <= 0 || m.h <= 0) {
    std::cerr << "bh_mapconv: bad dimensions\n";
    return 1;
  }
  const size_t n = static_cast<size_t>(m.w) * static_cast<size_t>(m.h);

  const nlohmann::json* groundL = findTileLayer(j, "ground");
  if (groundL == nullptr || !groundL->contains("data")) {
    std::cerr << "bh_mapconv: no 'ground' tile layer\n";
    return 1;
  }
  const auto& gdata = groundL->at("data");
  if (gdata.size() != n) {
    std::cerr << "bh_mapconv: ground layer size mismatch (" << gdata.size() << " != " << n
              << ")\n";
    return 1;
  }
  m.ground.resize(n);
  for (size_t i = 0; i < n; ++i) {
    const std::uint32_t gid = maskGid(gdata.at(i).get<double>());
    m.ground[i] = gid > 0 ? static_cast<std::uint16_t>(gid - 1) : 0;
  }

  m.blocked.assign(n, 0);
  if (const nlohmann::json* bl = findTileLayer(j, "blockers")) {
    const auto& bdata = bl->at("data");
    if (bdata.size() != n) {
      std::cerr << "bh_mapconv: blockers layer size mismatch\n";
      return 1;
    }
    for (size_t i = 0; i < n; ++i) {
      m.blocked[i] = maskGid(bdata.at(i).get<double>()) != 0 ? 1 : 0;
    }
  } else {
    size_t derived = 0;
    for (size_t i = 0; i < n; ++i) {
      if (m.ground[i] == 2 || m.ground[i] == 3) {
        m.blocked[i] = 1;
        ++derived;
      }
    }
    std::cout << "bh_mapconv: no blockers layer; derived " << derived << " blocked tiles\n";
  }

  m.zone.assign(n, 0);
  if (const nlohmann::json* zl = findTileLayer(j, "zones")) {
    const auto& zdata = zl->at("data");
    if (zdata.size() != n) {
      std::cerr << "bh_mapconv: zones layer size mismatch\n";
      return 1;
    }
    for (size_t i = 0; i < n; ++i) {
      const std::uint32_t gid = maskGid(zdata.at(i).get<double>());
      m.zone[i] = gid > 0 ? static_cast<std::uint16_t>(gid - 1) : 0;
    }
  }

  for (const auto& l : j.at("layers")) {
    if (l.value("type", "") != "objectgroup" || !l.contains("objects")) continue;
    const std::string lname = l.value("name", "");
    if (lname != "spawns" && lname != "portals") continue;
    for (const auto& o : l.at("objects")) {
      const int tx = tileOf(o.value("x", 0.0), m.tileW);
      const int ty = tileOf(o.value("y", 0.0), m.tileH);
      const int tw = spanOf(o.value("width", 0.0), m.tileW);
      const int th = spanOf(o.value("height", 0.0), m.tileH);
      if (lname == "spawns") {
        const double mob = propNum(o, "mobId", 0.0);
        if (mob <= 0.0) {
          std::cerr << "bh_mapconv: spawner without mobId, skipped\n";
          continue;
        }
        bh::sim::SpawnDef s;
        s.x = tx;
        s.y = ty;
        s.w = tw;
        s.h = th;
        s.mobId = static_cast<std::uint32_t>(mob);
        s.maxAlive =
            static_cast<std::uint32_t>(propNum(o, "maxAlive", 3.0));
        if (s.maxAlive < 1) s.maxAlive = 1;
        s.respawnTicks =
            static_cast<std::uint32_t>(propNum(o, "respawnTicks", 1200.0));
        if (s.respawnTicks < 20) s.respawnTicks = 20;
        m.spawners.push_back(s);
      } else {
        const double target = propNum(o, "targetMapId", 0.0);
        if (target <= 0.0) {
          std::cerr << "bh_mapconv: portal without targetMapId, skipped\n";
          continue;
        }
        bh::sim::PortalDef p;
        p.x = tx;
        p.y = ty;
        p.w = tw;
        p.h = th;
        p.targetMapId = static_cast<std::uint32_t>(target);
        p.targetX = static_cast<std::int32_t>(propNum(o, "targetX", -1.0));
        p.targetY = static_cast<std::int32_t>(propNum(o, "targetY", -1.0));
        m.portals.push_back(p);
      }
    }
  }

  size_t walkable = 0;
  for (const auto b : m.blocked) {
    if (b == 0) ++walkable;
  }
  const double wf = 100.0 * static_cast<double>(walkable) / static_cast<double>(n);
  std::printf(
      "bh_mapconv: map %u | %dx%d tiles (%dpx x %dpx) | walkable %.1f%% | spawners %zu | "
      "portals %zu\n",
      mapId, m.w, m.h, m.tileW, m.tileH, wf, m.spawners.size(), m.portals.size());

  if (validate) {
    auto inRect = [&](std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h) {
      return x >= 0 && y >= 0 && x + w <= m.w && y + h <= m.h;
    };
    for (const auto& s : m.spawners) {
      if (!inRect(s.x, s.y, s.w, s.h)) {
        std::cerr << "bh_mapconv: spawner rect out of bounds\n";
        return 2;
      }
    }
    for (const auto& p : m.portals) {
      if (!inRect(p.x, p.y, p.w, p.h)) {
        std::cerr << "bh_mapconv: portal rect out of bounds\n";
        return 2;
      }
    }
    if (wf < 10.0 || wf > 95.0) {
      std::cerr << "bh_mapconv: walkable fraction out of sane range\n";
      return 2;
    }
  }

  std::string err;
  if (!bh::sim::saveBhmap(outPath, m, &err)) {
    std::cerr << "bh_mapconv: " << err << "\n";
    return 1;
  }
  return 0;
}
