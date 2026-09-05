# bh_protocol: generated protocol serializers + bytestream codec (ADR-006).
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(BH_GEN_DIR ${CMAKE_BINARY_DIR}/generated/protocol)
set(BH_GEN_SRCS ${BH_GEN_DIR}/messages_gen.h ${BH_GEN_DIR}/messages_gen.cpp)

add_custom_command(
  OUTPUT ${BH_GEN_SRCS}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${BH_GEN_DIR}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/protogen/protogen.py
          ${CMAKE_CURRENT_SOURCE_DIR}/messages.md ${BH_GEN_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/messages.md
          ${CMAKE_SOURCE_DIR}/tools/protogen/protogen.py
  COMMENT "protogen: messages.md -> generated/protocol/messages_gen.*"
  VERBATIM)
add_custom_target(bh_protogen DEPENDS ${BH_GEN_SRCS})

add_library(bh_protocol STATIC ${BH_GEN_DIR}/messages_gen.cpp)
add_dependencies(bh_protocol bh_protogen)
target_include_directories(bh_protocol PUBLIC ${CMAKE_BINARY_DIR}/generated)
target_link_libraries(bh_protocol PUBLIC bh_shared PRIVATE bh_warnings)
