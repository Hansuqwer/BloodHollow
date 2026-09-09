// T-ART-09 decal law: blood persists + fades, telegraphs stage, circles
// hold, the surface caps FIFO. Raylib-free header, display-free pins.
#include <doctest/doctest.h>

#include <deque>

#include "render/decals.h"

using namespace bh;

TEST_CASE("T-ART-09: blood persists 10 min then dies") {
  Decal d;
  d.kind = DecalKind::kBlood;
  d.bornTick = 100;
  CHECK(decalAlive(d, 100));
  CHECK(decalAlive(d, 100 + kBloodDecalTtl - 1));
  CHECK_FALSE(decalAlive(d, 100 + kBloodDecalTtl));
  CHECK(decalAlpha(d, 100) == 110);
  CHECK(decalAlpha(d, 100 + kBloodDecalTtl) == 0);
  const int mid = decalAlpha(d, 100 + kBloodDecalTtl / 2);
  CHECK(mid > 0);
  CHECK(mid < 110);
}

TEST_CASE("T-ART-09: telegraphs run 20/20/40, circles hold") {
  CHECK(decalTtl(DecalKind::kTelegraph1) == 20);
  CHECK(decalTtl(DecalKind::kTelegraph2) == 20);
  CHECK(decalTtl(DecalKind::kTelegraph3) == 40);
  Decal c;
  c.kind = DecalKind::kCircle;
  c.bornTick = 0;
  CHECK(decalAlive(c, 1000000));
  CHECK(decalAlpha(c, 1000000) == 110);
}

TEST_CASE("T-ART-09: kill-20 leaves 20 countable decals; cap evicts FIFO") {
  std::deque<Decal> surface;
  for (int i = 0; i < 20; ++i) {
    Decal d;
    d.x = static_cast<float>(i);
    d.bornTick = 0;
    surface.push_back(d);
    while (surface.size() > kDecalCap) surface.pop_front();
  }
  CHECK(surface.size() == 20);
  int alive = 0;
  for (const Decal& d : surface)
    if (decalAlive(d, 6000)) ++alive;  // mid-blood-life: all hold
  CHECK(alive == 20);
  surface.clear();
  for (int i = 0; i < static_cast<int>(kDecalCap) + 5; ++i) {
    Decal d;
    d.bornTick = i;
    surface.push_back(d);
    while (surface.size() > kDecalCap) surface.pop_front();
  }
  CHECK(surface.size() == kDecalCap);
  CHECK(surface.front().bornTick == 5);  // first 5 evicted FIFO
}
