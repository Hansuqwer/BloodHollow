#pragma once

#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "sim/tick.h"

namespace bh::sim {

// Replay journal v0: commands + per-tick state hashes (T-009 / ADR-005).
// Text format on purpose: grep-able until volume justifies a binary v1.
//   BHJ1                <- magic header
//   e <tick> <type> <a> <b>   <- command event
//   h <tick> <hash>           <- post-tick world hash (optional in files)
enum JournalEventType : std::uint8_t {
  kEventPath = 1,  // a,b = goal tile x,y (click-to-move)
  kEventStep = 2,  // a,b = signed delta (WASD committed step)
};

struct JournalEvent {
  Tick tick = 0;
  std::uint8_t type = 0;
  std::int32_t a = 0;
  std::int32_t b = 0;
};

struct Journal {
  std::vector<JournalEvent> events;  // sorted by tick, stable
  std::vector<std::pair<Tick, std::uint64_t>> hashes;
};

class JournalWriter {
 public:
  bool open(const std::string& path);
  void event(Tick t, std::uint8_t type, std::int32_t a, std::int32_t b);
  void hash(Tick t, std::uint64_t h);
  bool ok() const { return ok_; }
  void close();
  ~JournalWriter() { close(); }

 private:
  std::FILE* f_ = nullptr;
  bool ok_ = false;
};

std::optional<Journal> readJournal(const std::string& path, std::string* err);

}  // namespace bh::sim
