#pragma once

#include <cstdint>

namespace bh::sim {

// xoshiro256** with splitmix64 seeding.
// Deterministic across Linux/macOS and compilers; never uses <random>
// distributions (those are implementation-defined and would break replay).
// All gameplay randomness flows through this class (ADR-005).
class Rng {
 public:
  explicit Rng(std::uint64_t seed);

  std::uint64_t next();
  // Uniform integer in [lo, hi] (inclusive). Unbiased on targets with
  // 128-bit multiply (all ours); modulo fallback otherwise (negligible bias).
  std::int64_t range(std::int64_t lo, std::int64_t hi);
  // Uniform double in [0, 1).
  double unit();
  // True with probability p.
  bool chance(double p);
  // Uniform float in [lo, hi).
  float frange(float lo, float hi);

 private:
  static std::uint64_t rotl(std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
  std::uint64_t s_[4];
};

}  // namespace bh::sim
