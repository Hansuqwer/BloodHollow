#include "sim/bhmap.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace bh::sim {

std::uint64_t fnv1a64(const std::uint8_t* data, size_t n, std::uint64_t seed) {
  std::uint64_t h = seed;
  for (size_t i = 0; i < n; ++i) {
    h ^= data[i];
    h *= 1099511628211ULL;
  }
  return h;
}

namespace {

void put16(std::vector<std::uint8_t>& b, std::uint16_t v) {
  b.push_back(static_cast<std::uint8_t>(v));
  b.push_back(static_cast<std::uint8_t>(v >> 8));
}
void put32(std::vector<std::uint8_t>& b, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}
void puti32(std::vector<std::uint8_t>& b, std::int32_t v) {
  put32(b, static_cast<std::uint32_t>(v));
}
void put64(std::vector<std::uint8_t>& b, std::uint64_t v) {
  for (int i = 0; i < 8; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}

void writeRle16(std::vector<std::uint8_t>& b, const std::vector<std::uint16_t>& v) {
  size_t i = 0;
  while (i < v.size()) {
    size_t run = 1;
    while (i + run < v.size() && v[i + run] == v[i] && run < 0xFFFFFFFFu) ++run;
    put16(b, v[i]);
    put32(b, static_cast<std::uint32_t>(run));
    i += run;
  }
}

struct Reader {
  const std::uint8_t* p = nullptr;
  size_t n = 0;
  size_t at = 0;
  bool u8(std::uint8_t& v) {
    if (at + 1 > n) return false;
    v = p[at++];
    return true;
  }
  bool u16(std::uint16_t& v) {
    std::uint8_t a, b;
    if (!u8(a) || !u8(b)) return false;
    v = static_cast<std::uint16_t>(a | (static_cast<std::uint16_t>(b) << 8));
    return true;
  }
  bool u32(std::uint32_t& v) {
    std::uint32_t r = 0;
    for (int i = 0; i < 4; ++i) {
      std::uint8_t b;
      if (!u8(b)) return false;
      r |= static_cast<std::uint32_t>(b) << (8 * i);
    }
    v = r;
    return true;
  }
  bool i32(std::int32_t& v) {
    std::uint32_t u;
    if (!u32(u)) return false;
    v = static_cast<std::int32_t>(u);
    return true;
  }
  bool u64(std::uint64_t& v) {
    std::uint64_t r = 0;
    for (int i = 0; i < 8; ++i) {
      std::uint8_t b;
      if (!u8(b)) return false;
      r |= static_cast<std::uint64_t>(b) << (8 * i);
    }
    v = r;
    return true;
  }
  bool skip(size_t k) {
    if (at + k > n) return false;
    at += k;
    return true;
  }
};

bool readRle16(Reader& r, std::vector<std::uint16_t>& out, size_t total) {
  out.clear();
  out.reserve(total);
  while (out.size() < total) {
    std::uint16_t val;
    std::uint32_t run;
    if (!r.u16(val) || !r.u32(run)) return false;
    if (run == 0 || out.size() + run > total) return false;
    out.insert(out.end(), run, val);
  }
  return true;
}

void setErr(std::string* err, const char* msg, const std::string& path) {
  if (err) *err = std::string(msg) + ": " + path;
}

}  // namespace

// GCC 14 -O3 false positive: -Wfree-nonheap-object fires on vector<uint8_t>
// reserve+push_back after inlining put32 into saveBhmap. Suppress locally;
// the pattern is plain vector append and is ASan-clean in test runs.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
bool saveBhmap(const std::string& path, const Map& m, std::string* err) {
  const size_t n = static_cast<size_t>(m.w) * static_cast<size_t>(m.h);
  if (m.w <= 0 || m.h <= 0 || m.w > kBhmapMaxDim || m.h > kBhmapMaxDim ||
      m.ground.size() != n || m.zone.size() != n || m.blocked.size() != n) {
    setErr(err, "map dimension/layer size mismatch", path);
    return false;
  }

  std::vector<std::uint8_t> payload;
  payload.reserve(n * 2);
  writeRle16(payload, m.ground);
  writeRle16(payload, m.zone);

  const size_t bitBytes = (n + 7) / 8;
  std::vector<std::uint8_t> bits(bitBytes, 0);
  for (size_t i = 0; i < n; ++i) {
    if (m.blocked[i] != 0) bits[i >> 3] |= static_cast<std::uint8_t>(1u << (i & 7));
  }
  payload.insert(payload.end(), bits.begin(), bits.end());

  for (const SpawnDef& s : m.spawners) {
    puti32(payload, s.x);
    puti32(payload, s.y);
    puti32(payload, s.w);
    puti32(payload, s.h);
    put32(payload, s.mobId);
    put32(payload, s.maxAlive);
    put32(payload, s.respawnTicks);
  }
  for (const PortalDef& p : m.portals) {
    puti32(payload, p.x);
    puti32(payload, p.y);
    puti32(payload, p.w);
    puti32(payload, p.h);
    puti32(payload, p.targetX);
    puti32(payload, p.targetY);
    put32(payload, p.targetMapId);
  }

  std::vector<std::uint8_t> file;
  file.reserve(40 + payload.size());
  put32(file, kBhmapMagic);
  put32(file, kBhmapVersion);
  puti32(file, m.w);
  puti32(file, m.h);
  puti32(file, m.tileW);
  puti32(file, m.tileH);
  put32(file, static_cast<std::uint32_t>(m.spawners.size()));
  put32(file, static_cast<std::uint32_t>(m.portals.size()));
  put64(file, fnv1a64(payload.data(), payload.size()));
  file.insert(file.end(), payload.begin(), payload.end());

  std::error_code ec;
  const std::filesystem::path parent = std::filesystem::path(path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent, ec);

  FILE* f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) {
    setErr(err, "cannot open for write", path);
    return false;
  }
  const bool ok = std::fwrite(file.data(), 1, file.size(), f) == file.size();
  std::fclose(f);
  if (!ok) setErr(err, "short write", path);
  return ok;
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

std::optional<Map> loadBhmap(const std::string& path, std::string* err) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) {
    setErr(err, "cannot open", path);
    return std::nullopt;
  }
  std::fseek(f, 0, SEEK_END);
  const long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (sz < 40) {
    std::fclose(f);
    setErr(err, "file too small", path);
    return std::nullopt;
  }
  std::vector<std::uint8_t> bytes(static_cast<size_t>(sz));
  const size_t got = std::fread(bytes.data(), 1, bytes.size(), f);
  std::fclose(f);
  if (got != bytes.size()) {
    setErr(err, "short read", path);
    return std::nullopt;
  }

  Reader r{bytes.data(), bytes.size(), 0};
  std::uint32_t magic, version, spawnCount, portalCount;
  std::uint64_t checksum;
  Map m;
  if (!r.u32(magic) || !r.u32(version) || !r.i32(m.w) || !r.i32(m.h) || !r.i32(m.tileW) ||
      !r.i32(m.tileH) || !r.u32(spawnCount) || !r.u32(portalCount) || !r.u64(checksum)) {
    setErr(err, "truncated header", path);
    return std::nullopt;
  }
  if (magic != kBhmapMagic) {
    setErr(err, "bad magic (not a .bhmap)", path);
    return std::nullopt;
  }
  if (version != kBhmapVersion) {
    setErr(err, "unsupported version", path);
    return std::nullopt;
  }
  if (m.w <= 0 || m.h <= 0 || m.w > kBhmapMaxDim || m.h > kBhmapMaxDim) {
    setErr(err, "bad dimensions", path);
    return std::nullopt;
  }
  const std::uint64_t actual = fnv1a64(bytes.data() + r.at, bytes.size() - r.at);
  if (actual != checksum) {
    setErr(err, "checksum mismatch (corrupt or tampered map)", path);
    return std::nullopt;
  }

  const size_t n = static_cast<size_t>(m.w) * static_cast<size_t>(m.h);
  if (!readRle16(r, m.ground, n) || !readRle16(r, m.zone, n)) {
    setErr(err, "corrupt RLE layer", path);
    return std::nullopt;
  }
  const size_t bitBytes = (n + 7) / 8;
  if (r.at + bitBytes > r.n) {
    setErr(err, "truncated bitfield", path);
    return std::nullopt;
  }
  m.blocked.assign(n, 0);
  for (size_t i = 0; i < n; ++i) {
    m.blocked[i] = static_cast<std::uint8_t>((r.p[r.at + (i >> 3)] >> (i & 7)) & 1u);
  }
  if (!r.skip(bitBytes)) {
    setErr(err, "truncated bitfield", path);
    return std::nullopt;
  }

  m.spawners.clear();
  for (std::uint32_t i = 0; i < spawnCount; ++i) {
    SpawnDef s;
    if (!r.i32(s.x) || !r.i32(s.y) || !r.i32(s.w) || !r.i32(s.h) || !r.u32(s.mobId) ||
        !r.u32(s.maxAlive) || !r.u32(s.respawnTicks)) {
      setErr(err, "truncated spawners", path);
      return std::nullopt;
    }
    m.spawners.push_back(s);
  }
  m.portals.clear();
  for (std::uint32_t i = 0; i < portalCount; ++i) {
    PortalDef p;
    if (!r.i32(p.x) || !r.i32(p.y) || !r.i32(p.w) || !r.i32(p.h) || !r.i32(p.targetX) ||
        !r.i32(p.targetY) || !r.u32(p.targetMapId)) {
      setErr(err, "truncated portals", path);
      return std::nullopt;
    }
    m.portals.push_back(p);
  }
  if (r.at != r.n) {
    setErr(err, "trailing bytes", path);
    return std::nullopt;
  }
  return m;
}

}  // namespace bh::sim
