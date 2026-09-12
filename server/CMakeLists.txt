# bh_server: headless authoritative world server (ADR-002/003/004).
# world/combat live in a static lib so bh_tests can unit-test the sim without
# the ENet/CLI shell around it.
add_library(bh_server_world STATIC
  src/world.cpp
  src/persist.cpp
)
target_include_directories(bh_server_world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bh_server_world PUBLIC bh_shared bh_protocol bh_sqlite PRIVATE bh_warnings)

add_executable(bh_server
  src/main.cpp
)
target_include_directories(bh_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bh_server PRIVATE
  bh_server_world bh_shared bh_protocol bh_enet bh_sqlite bh_warnings)
