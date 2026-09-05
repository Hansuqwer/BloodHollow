# bh_duel: offline gear-aware duel harness (balancer v2, T-034).
add_executable(bh_duel main.cpp)
target_link_libraries(bh_duel PRIVATE bh_shared bh_server_world bh_warnings)
