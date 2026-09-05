# BloodEngine: the client-facing engine layer (never linked by the server).
add_library(bh_engine STATIC
  render/iso.cpp
  render/daynight.cpp
  assets/atlas.cpp
  assets/placeholder.cpp
)
target_include_directories(bh_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bh_engine
  PUBLIC bh_shared raylib bh_json
  PRIVATE bh_warnings)
