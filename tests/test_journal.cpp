#include <cstdio>
#include <filesystem>

#include <doctest/doctest.h>

#include "sim/journal.h"

using namespace bh;

namespace {
std::string tmpPath(const char* name) {
  return (std::filesystem::temp_directory_path() / name).string();
}
}  // namespace

TEST_CASE("journal: writer/reader round-trip (events + hashes)") {
  const std::string path = tmpPath("bh_journal_rt.bhj");
  {
    sim::JournalWriter w;
    REQUIRE(w.open(path));
    w.event(10, sim::kEventPath, 55, 23);
    w.event(11, sim::kEventStep, -1, 1);
    w.hash(10, 0xDEADBEEF12345678ULL);
    w.hash(11, 42);
    w.close();
  }
  std::string err;
  const auto j = sim::readJournal(path, &err);
  REQUIRE(j.has_value());
  REQUIRE(j->events.size() == 2);
  CHECK(j->events[0].tick == 10);
  CHECK(j->events[0].type == sim::kEventPath);
  CHECK(j->events[0].a == 55);
  CHECK(j->events[0].b == 23);
  CHECK(j->events[1].type == sim::kEventStep);
  CHECK(j->events[1].a == -1);
  REQUIRE(j->hashes.size() == 2);
  CHECK(j->hashes[0].second == 0xDEADBEEF12345678ULL);
  std::filesystem::remove(path);
}

TEST_CASE("journal: reader rejects garbage and missing files") {
  std::string err;
  const auto missing = sim::readJournal(tmpPath("bh_journal_nope.bhj"), &err);
  CHECK_FALSE(missing.has_value());
  CHECK_FALSE(err.empty());

  const std::string path = tmpPath("bh_journal_garbage.bhj");
  {
    FILE* f = std::fopen(path.c_str(), "w");
    REQUIRE(f != nullptr);
    std::fputs("NOT_A_JOURNAL\n1 2 3\n", f);
    std::fclose(f);
  }
  err.clear();
  const auto bad = sim::readJournal(path, &err);
  CHECK_FALSE(bad.has_value());
  CHECK_FALSE(err.empty());
  std::filesystem::remove(path);
}
