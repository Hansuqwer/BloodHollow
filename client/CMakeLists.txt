add_executable(bh_client src/main.cpp src/game.cpp src/net_client.cpp)
target_include_directories(bh_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bh_client PRIVATE
  bh_engine bh_shared bh_protocol bh_enet bh_warnings)
