set(_bh_tests_raylib_srcs test_zoom.cpp test_animstate.cpp test_clientlaw.cpp)
if(NOT TARGET raylib)
  set(_bh_tests_raylib_srcs "")
endif()
add_executable(bh_tests
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
  ${_bh_tests_raylib_srcs}
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
  test_affix_v2.cpp
  test_mine.cpp
  test_uniques.cpp
  test_blood_moon.cpp
  test_ek_oath.cpp
  test_siege_sched.cpp
  test_siege_gates.cpp
  test_siege_crown.cpp
  test_siege_taxes.cpp
  test_pledge_bands.cpp
  test_pledge_vault.cpp
  test_gm_authority.cpp
  test_wave2_epoch30.cpp  # wave-2: T-167/T-160/T-161/T-162/T-163/T-166 pins
  test_t153_auth.cpp  # T-153: argon2id PHC, migration, refusals
  test_sanctuary.cpp  # T-161b.1: Cultist ch11 ground hold
  test_weaken.cpp  # T-161b.2: Cultist ch12 debility
  test_rarity_chrome.cpp  # T-159f1.3: tier marker law
  test_raise.cpp  # T-161b.3: Cultist ch13 thrall
  test_corpse.cpp  # T-161b.4: Gravecaller ch14 detonation
  test_control.cpp  # T-161b.5: Gravecaller ch15-18 control set
  test_ravager.cpp  # T-161b.6: Ravager ch19-23 set
  test_droplog.cpp  # T-159f1.5: drop-line instrumentation
  test_multiaffix.cpp  # ADR-0016: multi-affix + 45-row band tables
  test_terrain_skin.cpp  # T-ART-12: textured-ground law (raylib-free)
)
target_link_libraries(bh_tests PRIVATE bh_shared bh_protocol bh_server_world bh_doctest bh_warnings)
# T-ART-02: snapZoom lives in raylib-including camera_rig.h — link raylib for
# the headers (test TU never opens a display; pure float math under test).
if(TARGET raylib)
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
