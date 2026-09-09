// S28 law pins: mob sheet-dir table, overhead tint priority, feet anchors.
// All headless-testable (raylib-free headers, or display-free struct use);
// the loader/cache/draw wiring is stated headless-untestable, not faked.
#include <doctest/doctest.h>

#include <string>

#include "assets/atlas.h"
#include "render/overhead.h"

using namespace bh;

TEST_CASE("T-ART-05: sheet dirs resolve for every shipped mob row") {
  char png[160], js[160];
  REQUIRE(mobSheetPaths(1, png, sizeof png, js, sizeof js));
  CHECK(std::string(png) == "assets/aigen/mobs/1001_marsh_rat/sheet.png");
  CHECK(std::string(js) == "assets/aigen/mobs/1001_marsh_rat/sheet.json");
  REQUIRE(mobSheetPaths(3, png, sizeof png, js, sizeof js));
  CHECK(std::string(png) == "assets/aigen/mobs/1003_hollow_hound/sheet.png");
  REQUIRE(mobSheetPaths(9, png, sizeof png, js, sizeof js));
  CHECK(std::string(png) == "assets/aigen/mobs/1009_gravemother/sheet.png");
  REQUIRE(mobSheetPaths(10, png, sizeof png, js, sizeof js));
  CHECK(std::string(png) == "assets/aigen/mobs/1010_sepulcher_elite/sheet.png");
  // 1011 Guard resolves paths too (no sheet shipped yet — loader falls back)
  REQUIRE(mobSheetPaths(11, png, sizeof png, js, sizeof js));
  CHECK(std::string(png) == "assets/aigen/mobs/1011_gate_guard/sheet.png");
  // players, furniture, and out-of-range never reach the loader
  CHECK_FALSE(mobSheetPaths(0, png, sizeof png, js, sizeof js));
  CHECK_FALSE(mobSheetPaths(64, png, sizeof png, js, sizeof js));
  CHECK_FALSE(mobSheetPaths(69, png, sizeof png, js, sizeof js));
  CHECK_FALSE(mobSheetPaths(255, png, sizeof png, js, sizeof js));
}

TEST_CASE("T-ART-07: overhead tint priority (red > party > lawful > gray)") {
  CHECK(resolveNameTint(false, 2, true) == NameTint::kChaotic);  // red wins ties
  CHECK(resolveNameTint(false, 2, false) == NameTint::kChaotic);
  CHECK(resolveNameTint(false, 1, true) == NameTint::kParty);
  CHECK(resolveNameTint(false, 0, true) == NameTint::kParty);  // party beats lawful
  CHECK(resolveNameTint(false, 0, false) == NameTint::kLawful);
  CHECK(resolveNameTint(false, 1, false) == NameTint::kNeutral);
  // no enemy-town enumerator exists (war state unshipped) — by documentation
  CHECK(resolveNameTint(true, 2, true) == NameTint::kNeutral);  // own short-circuits
}

TEST_CASE("T-ART-10: feet anchors ride the anim (42.0 legacy default)") {
  Atlas a;
  CHECK(animAnchorY(a, "walk") == 42.0f);  // absent anim: legacy default
  Anim an;
  an.anchorY = 32.0f;
  a.anims.emplace("walk", an);
  CHECK(animAnchorY(a, "walk") == 32.0f);
  CHECK(animAnchorY(a, "attack") == 42.0f);  // absent anim, present atlas
}
