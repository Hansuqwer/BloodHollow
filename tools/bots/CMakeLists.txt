# bh_bots: headless scripted clients for soak/load tests (ADR-008: bots exist
# before combat does).
add_executable(bh_bots main.cpp)
target_link_libraries(bh_bots PRIVATE bh_shared bh_protocol bh_enet bh_warnings)
