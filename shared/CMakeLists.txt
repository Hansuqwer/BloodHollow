# bh_shared: the contract between client, server and tools.
# Pure logic only — no raylib, no I/O except explicit file codecs.
add_library(bh_shared STATIC
  sim/rng.cpp
  sim/grid.cpp
  sim/astar.cpp
  sim/bhmap.cpp
  sim/walker.cpp
  sim/journal.cpp
)
target_include_directories(bh_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bh_shared PRIVATE bh_warnings)

add_subdirectory(protocol)
