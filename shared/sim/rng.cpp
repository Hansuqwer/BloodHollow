#include "sim/rng.h"

namespace bh::sim {

namespace {
std::uint64_t splitmix64(std::uint64_t& x) {
  std::uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}
}  // namespace

Rng::Rng(std::uint64_t seed) {
  std::uint64_t x = seed != 0 ? seed : 0x0123456789abcdefULL;
  for (int i = 0; i < 4; ++i) s_[i] = splitmix64(x);
  for (int i = 0; i < 16; ++i) (void)next();  // discard warmup
}

std::uint64_t Rng::next() {
  const std::uint64_t r = rotl(s_[1] * 5, 7) * 9;
  const std::uint64_t t = s_[1] << 17;
  s_[2] ^= s_[0];
  s_[3] ^= s_[1];
  s_[1] ^= s_[2];
  s_[0] ^= s_[3];
  s_[2] ^= t;
  s_[3] = rotl(s_[3], 45);
  return r;
}

std::int64_t Rng::range(std::int64_t lo, std::int64_t hi) {
  const std::uint64_t span = static_cast<std::uint64_t>(hi - lo) + 1ULL;
#if defined(__SIZEOF_INT128__)
  // Lemire multiply-shift; rejection threshold keeps it unbiased.
  const std::uint64_t threshold = (0ULL - span) % span;
  for (;;) {
    const std::uint64_t x = next();
    const unsigned __int128 m = static_cast<unsigned __int128>(x) * span;
    const std::uint64_t l = static_cast<std::uint64_t>(m);
    if (l >= threshold) return lo + static_cast<std::int64_t>(m >> 64);
  }
#else
  return lo + static_cast<std::int64_t>(next() % span);
#endif
}

double Rng::unit() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }

bool Rng::chance(double p) {
  if (p <= 0.0) return false;
  if (p >= 1.0) return true;
  return unit() < p;
}

float Rng::frange(float lo, float hi) { return lo + (hi - lo) * static_cast<float>(unit()); }

}  // namespace bh::sim
