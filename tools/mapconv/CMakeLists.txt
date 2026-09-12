add_executable(bh_mapconv main.cpp)
target_link_libraries(bh_mapconv PRIVATE bh_shared bh_json bh_warnings)

# Maps are data-derived artifacts: every build keeps .bhmap in sync with .tmj.
# T-071: all five maps sync (previously only thornwall did — the v1->v2
# spawner change left zones 2-5 stale and unloadable).
add_custom_target(bh_maps ALL
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/thornwall.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/thornwall.bhmap
          --map-id 1 --validate
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/fields_overflow.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/fields_overflow.bhmap
          --map-id 2 --validate
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/thornwall_crypt.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/thornwall_crypt.bhmap
          --map-id 3 --validate
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/bonehowl_mine.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/bonehowl_mine.bhmap
          --map-id 4 --validate
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/drowned_crypt.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/drowned_crypt.bhmap
          --map-id 5 --validate
  COMMAND bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/weeping_castle.tmj
          ${CMAKE_SOURCE_DIR}/assets/maps/weeping_castle.bhmap
          --map-id 6 --validate
  DEPENDS bh_mapconv
          ${CMAKE_SOURCE_DIR}/data/maps-src/thornwall.tmj
          ${CMAKE_SOURCE_DIR}/data/maps-src/fields_overflow.tmj
          ${CMAKE_SOURCE_DIR}/data/maps-src/thornwall_crypt.tmj
          ${CMAKE_SOURCE_DIR}/data/maps-src/bonehowl_mine.tmj
          ${CMAKE_SOURCE_DIR}/data/maps-src/drowned_crypt.tmj
          ${CMAKE_SOURCE_DIR}/data/maps-src/weeping_castle.tmj
  COMMENT "mapconv: .tmj -> assets/maps/*.bhmap")
