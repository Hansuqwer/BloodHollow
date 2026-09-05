add_executable(bh_mapconv main.cpp)
target_link_libraries(bh_mapconv PRIVATE bh_shared bh_json bh_warnings)

# Maps are data-derived artifacts: every build keeps .bhmap in sync with .tmj.
add_custom_target(bh_maps ALL
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/thornwall.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/thornwall.bhmap
          --map-id 1 --validate
  DEPENDS bh_mapconv ${CMAKE_SOURCE_DIR}/data/maps-src/thornwall.tmj
  COMMENT "mapconv: thornwall.tmj -> assets/maps/thornwall.bhmap")
