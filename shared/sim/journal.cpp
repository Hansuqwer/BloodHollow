#include "sim/journal.h"

#include <algorithm>
#include <sstream>

namespace bh::sim {

bool JournalWriter::open(const std::string& path) {
  close();
  f_ = std::fopen(path.c_str(), "w");
  ok_ = f_ != nullptr;
  if (ok_) std::fprintf(f_, "BHJ1\n");
  return ok_;
}

void JournalWriter::event(Tick t, std::uint8_t type, std::int32_t a, std::int32_t b) {
  if (!ok_) return;
  std::fprintf(f_, "e %lld %u %d %d\n", static_cast<long long>(t),
               static_cast<unsigned>(type), a, b);
}

void JournalWriter::hash(Tick t, std::uint64_t h) {
  if (!ok_) return;
  std::fprintf(f_, "h %lld %llu\n", static_cast<long long>(t),
               static_cast<unsigned long long>(h));
}

void JournalWriter::close() {
  if (f_ != nullptr) {
    std::fclose(f_);
    f_ = nullptr;
  }
  ok_ = false;
}

std::optional<Journal> readJournal(const std::string& path, std::string* err) {
  std::FILE* f = std::fopen(path.c_str(), "r");
  if (f == nullptr) {
    if (err) *err = "cannot open: " + path;
    return std::nullopt;
  }
  Journal j;
  char line[256];
  bool headerOk = false;
  while (std::fgets(line, sizeof line, f) != nullptr) {
    std::istringstream ls(line);
    std::string tag;
    ls >> tag;
    if (tag == "BHJ1") {
      headerOk = true;
      continue;
    }
    if (tag == "e") {
      JournalEvent e;
      long long t;
      unsigned ty;
      if (!(ls >> t >> ty >> e.a >> e.b)) continue;
      e.tick = static_cast<Tick>(t);
      e.type = static_cast<std::uint8_t>(ty);
      j.events.push_back(e);
      continue;
    }
    if (tag == "h") {
      long long t;
      unsigned long long h;
      if (!(ls >> t >> h)) continue;
      j.hashes.push_back({static_cast<Tick>(t), static_cast<std::uint64_t>(h)});
      continue;
    }
  }
  std::fclose(f);
  if (!headerOk) {
    if (err) *err = "bad journal header (expected BHJ1): " + path;
    return std::nullopt;
  }
  // Events arrive in tick order by construction; enforce stability anyway.
  std::stable_sort(j.events.begin(), j.events.end(),
                   [](const JournalEvent& a, const JournalEvent& b) { return a.tick < b.tick; });
  return j;
}

}  // namespace bh::sim
