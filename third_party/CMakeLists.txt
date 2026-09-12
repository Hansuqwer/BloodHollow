# Vendored/approved dependencies (see AGENTS.md). Single-header libs are
# committed; ENet + SQLite are downloaded in-tree and built DIRECTLY (their
# upstream build systems predate cmake 3.5 and are rejected by cmake 4.x).
add_library(bh_doctest INTERFACE)
target_include_directories(bh_doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/doctest)

add_library(bh_json INTERFACE)
target_include_directories(bh_json SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann)

if(BH_BUILD_SERVER)
  set(_deps ${CMAKE_BINARY_DIR}/_deps)

  # ---- ENet 1.3.18 (ADR-0001; connection-layer of the netcode) ----
  set(_enet_tgz ${_deps}/enet-1.3.18.tar.gz)
  set(_enet_dir ${_deps}/enet-1.3.18)
  if(NOT EXISTS ${_enet_dir}/callbacks.c)
    # Prefer a mirror tarball shipped in-tree if the network is unavailable
    # (sandbox CI / offline build); otherwise fetch from upstream.
    set(_enet_local ${CMAKE_CURRENT_SOURCE_DIR}/enet-1.3.18.tar.gz)
    if(EXISTS ${_enet_local})
      file(COPY ${_enet_local} DESTINATION ${_deps})
    else()
      file(DOWNLOAD https://github.com/lsalzman/enet/archive/refs/tags/v1.3.18.tar.gz
           ${_enet_tgz}
           EXPECTED_HASH SHA256=28603c895f9ed24a846478180ee72c7376b39b4bb1287b73877e5eae7d96b0dd
           SHOW_PROGRESS)
    endif()
    file(ARCHIVE_EXTRACT INPUT ${_enet_tgz} DESTINATION ${_deps})
  endif()
  add_library(bh_enet STATIC
    ${_enet_dir}/callbacks.c
    ${_enet_dir}/compress.c
    ${_enet_dir}/host.c
    ${_enet_dir}/list.c
    ${_enet_dir}/packet.c
    ${_enet_dir}/peer.c
    ${_enet_dir}/protocol.c
    ${_enet_dir}/unix.c
  )
  target_include_directories(bh_enet SYSTEM PUBLIC ${_enet_dir}/include)
  target_compile_definitions(bh_enet PRIVATE
    HAS_FCNTL=1 HAS_POLL=1 HAS_MSGHDR_FLAGS=1 HAS_SOCKLEN_T=1
    HAS_INET_PTON=1 HAS_INET_NTOP=1)
  set_property(TARGET bh_enet PROPERTY C_STANDARD 99)

  # ---- SQLite amalgamation (ADR-0004) ----
  # A locally-vendored amalgamation lives at third_party/sqlite-amalgamation/
  # (committed only when the download-from-sqlite.org path is unavailable in
  # CI/sandbox environments). When that directory exists we build directly
  # from it; otherwise we download the pinned 3.43.2 zip from sqlite.org.
  # Operators who wish to pin a specific SQLite can drop sqlite3.c/.h into
  # the vendored directory.
  set(_sq_dir ${_deps}/sqlite-amalgamation-3430200)
  set(_sq_local_dir ${CMAKE_CURRENT_SOURCE_DIR}/sqlite-amalgamation)
  if(EXISTS ${_sq_local_dir}/sqlite3.c)
    set(_sq_dir ${_sq_local_dir})
  elseif(NOT EXISTS ${_sq_dir}/sqlite3.c)
    set(_sq_zip ${_deps}/sqlite-amalgamation-3430200.zip)
    file(DOWNLOAD https://sqlite.org/2023/sqlite-amalgamation-3430200.zip
         ${_sq_zip}
         EXPECTED_HASH SHA256=a17ac8792f57266847d57651c5259001d1e4e4b46be96ec0d985c953925b2a1c
         SHOW_PROGRESS)
    file(ARCHIVE_EXTRACT INPUT ${_sq_zip} DESTINATION ${_deps})
  endif()
  add_library(bh_sqlite STATIC ${_sq_dir}/sqlite3.c)
  target_include_directories(bh_sqlite SYSTEM PUBLIC ${_sq_dir})
  target_link_libraries(bh_sqlite PUBLIC ${CMAKE_DL_LIBS} m Threads::Threads)
  set_property(TARGET bh_sqlite PROPERTY C_STANDARD 99)
  target_compile_definitions(bh_sqlite PRIVATE
    SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_ENABLE_FTS5=1)
endif()
