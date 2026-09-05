cmake_minimum_required(VERSION 3.24)
project(bloodhollow VERSION 0.1.0 LANGUAGES C CXX)

# BLOODHOLLOW root build. Options let headless CI skip the graphical client.
option(BH_BUILD_CLIENT "Build the graphical client (needs X11/GL dev libs on Linux)" ON)
option(BH_BUILD_TOOLS  "Build tools (mapconv)" ON)
option(BH_BUILD_TESTS  "Build tests" ON)
option(BH_BUILD_SERVER "Build the headless world server + soak bots" ON)

if(BH_BUILD_SERVER)
  find_package(Threads REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# Warning discipline applies to OUR targets only, never vendored deps (AGENTS.md).
add_library(bh_warnings INTERFACE)
if(MSVC)
  target_compile_options(bh_warnings INTERFACE /W4 /WX)
else()
  target_compile_options(bh_warnings INTERFACE -Wall -Wextra -Werror)
endif()

add_subdirectory(third_party)
add_subdirectory(shared)

if(BH_BUILD_CLIENT)
  # raylib 5.5, pinned by tag + hash. Zero deps beyond X11/GL dev packages.
  include(FetchContent)
  set(BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES       OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(PLATFORM "Desktop" CACHE STRING "" FORCE)
  FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(raylib)
  # Keep raylib headers outside our -Werror blast radius.
  get_target_property(_raylib_inc raylib INTERFACE_INCLUDE_DIRECTORIES)
  if(_raylib_inc)
    set_target_properties(raylib PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_raylib_inc}")
  endif()
  add_subdirectory(engine)
  add_subdirectory(client)
endif()

if(BH_BUILD_TOOLS)
  add_subdirectory(tools/mapconv)
endif()

if(BH_BUILD_SERVER)
  add_subdirectory(server)
  add_subdirectory(tools/bots)
endif()

if(BH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
