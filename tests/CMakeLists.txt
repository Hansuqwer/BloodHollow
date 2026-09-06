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
  test_kits.cpp
  test_alignment.cpp
  test_gear_churn.cpp
  test_night.cpp
  test_s18_dropzone.cpp
  test_burst_stamp.cpp
  test_kit_v2.cpp
  test_inv_blob.cpp
)
target_link_libraries(bh_tests PRIVATE bh_shared bh_protocol bh_server_world bh_doctest bh_warnings)
add_test(NAME bh_tests COMMAND bh_tests)
# asset paths in tests are repo-root-relative (e.g. assets/maps/*.bhmap)
set_tests_properties(bh_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
# T-034: balancer harness determinism gate — CI-visible replay-class guard
add_test(NAME bh_duel_selftest COMMAND $<TARGET_FILE:bh_duel> --selftest)
set_tests_properties(bh_duel_selftest PROPERTIES DEPENDS bh_duel)
