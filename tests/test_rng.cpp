#include <doctest/doctest.h>

#include "sim/rng.h"

using namespace bh;

TEST_CASE("rng: same seed reproduces the same stream (replay determinism)") {
  sim::Rng a(42);
  sim::Rng b(42);
  for (int i = 0; i < 1000; ++i) {
    CHECK(a.next() == b.next());
  }
}

TEST_CASE("rng: different seeds diverge immediately") {
  sim::Rng a(1);
  sim::Rng b(2);
  CHECK(a.next() != b.next());
}

TEST_CASE("rng: range() stays inside [lo, hi] over 100k samples") {
  sim::Rng r(1234);
  for (int i = 0; i < 100000; ++i) {
    const std::int64_t v = r.range(-7, 13);
    CHECK(v >= -7);
    CHECK(v <= 13);
  }
}

TEST_CASE("rng: chance() bounds and unit() mean") {
  sim::Rng r(777);
  CHECK(r.chance(1.0));
  CHECK_FALSE(r.chance(0.0));
  double sum = 0.0;
  const int n = 20000;
  for (int i = 0; i < n; ++i) sum += r.unit();
  const double mean = sum / static_cast<double>(n);
  CHECK(mean > 0.48);
  CHECK(mean < 0.52);
}
