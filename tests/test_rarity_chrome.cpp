// T-159f1.3 rarity chrome: the text half of the T-159 tier readout.
// Pins mirror test_refine_glow.cpp (tiers + boundaries, no rendering).
#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>

#include "render/rarity_chrome.h"

TEST_CASE("T-159f1.3: markers per tier, silent common") {
  CHECK(std::strcmp(bh::rarityMarker(0), "") == 0);
  CHECK(std::strcmp(bh::rarityMarker(1), "M ") == 0);
  CHECK(std::strcmp(bh::rarityMarker(2), "R ") == 0);
  CHECK(std::strcmp(bh::rarityMarker(3), "U ") == 0);
}

TEST_CASE("T-159f1.3: chrome presence + wire-distrust") {
  CHECK_FALSE(bh::hasRarityChrome(0));
  CHECK(bh::hasRarityChrome(1));
  CHECK(bh::hasRarityChrome(2));
  CHECK(bh::hasRarityChrome(3));
  CHECK_FALSE(bh::hasRarityChrome(4));  // out-of-range renders common
  CHECK_FALSE(bh::hasRarityChrome(255));
  CHECK(std::strcmp(bh::rarityMarker(4), "") == 0);
  CHECK(std::strcmp(bh::rarityMarker(255), "") == 0);
}
