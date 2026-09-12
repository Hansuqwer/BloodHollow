# bh_tests: server/sim law suite. Most TUs are raylib-free; three client-law
# TUs (test_zoom, test_animstate, test_clientlaw) include raylib-bearing
# headers (camera_rig.h etc.). raylib is only fetched when BH_BUILD_CLIENT=ON,
# so those TUs and the raylib link are gated together (T-108) — the headless
# preset must configure and build without X11/GL, per CMakePresets.json.
set(BH_TEST_SOURCES
  tests_main.cpp
  test_grid.cpp
  test_rng.cpp
  test_astar.cpp
  test_bhmap.cpp
  test_walker.cpp
  test_clock.cpp
  test_journal.cpp
  test_bytestream.cpp
  test_protogen.cpp
  test_spatial.cpp
  test_combat.cpp
  test_cmdparity.cpp
  test_party.cpp
  test_pledges.cpp
  test_kits.cpp
  test_alignment.cpp
  test_fence.cpp
  test_curse.cpp
  test_light.cpp
  test_guards.cpp
  test_repent.cpp
  test_tradelog.cpp
  test_purify.cpp
  test_gear_churn.cpp
  test_night.cpp
  test_s18_dropzone.cpp
  test_burst_stamp.cpp
  test_kit_v2.cpp
  test_inv_blob.cpp
  test_refine_glow.cpp
  test_decals.cpp
  test_npcs.cpp
  test_slam.cpp
  test_elites.cpp
  test_refine_parse.cpp
  test_trade_equipped.cpp
  test_deque_dangling.cpp
  test_loginlimit.cpp
  test_worldhash.cpp
  test_t104_review_fixes.cpp
  test_t111_review.cpp
  test_t112_spawn_epoch.cpp
  test_weapon_skill_persist.cpp
)
if(BH_BUILD_CLIENT)
  # T-ART-02: snapZoom lives in raylib-including camera_rig.h — the test TUs
  # never open a display; it is pure float/anim law under test. Headless CI
  # legs simply skip these three TUs (their law is re-covered by the client
  # build's own compile + the full graphical CI leg).
  list(APPEND BH_TEST_SOURCES
    test_zoom.cpp
    test_animstate.cpp
    test_clientlaw.cpp
  )
endif()

add_executable(bh_tests ${BH_TEST_SOURCES})
target_link_libraries(bh_tests PRIVATE bh_shared bh_protocol bh_server_world bh_doctest bh_warnings bh_sqlite)
if(BH_BUILD_CLIENT)
  target_link_libraries(bh_tests PRIVATE raylib)
endif()
# T-071: light-mask law pins live in raylib-free engine/render/lightmask.h.
target_include_directories(bh_tests PRIVATE ${CMAKE_SOURCE_DIR}/engine)
add_test(NAME bh_tests COMMAND bh_tests)
# asset paths in tests are repo-root-relative (e.g. assets/maps/*.bhmap)
set_tests_properties(bh_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
# T-034: balancer harness determinism gate — CI-visible replay-class guard
add_test(NAME bh_duel_selftest COMMAND $<TARGET_FILE:bh_duel> --selftest)
set_tests_properties(bh_duel_selftest PROPERTIES DEPENDS bh_duel)
