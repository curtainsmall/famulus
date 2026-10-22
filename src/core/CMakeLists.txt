# ============================================================================
#  src/core/ - the daemon
#
#  One rule decides what goes where, and it removes every exclusion:
#
#      src/core/main.cpp      process bootstrap, builds famulusd and nothing else
#      src/core/<subdir>/...  the daemon's logic, builds famulus_core
#
#  A file directly in src/core/ never joins the library, so the library cannot
#  end up carrying a main(). The glob below starts one level down for exactly
#  that reason: `src/core/*/` means "inside a subdirectory".
#
#  Dependency rule: core may use base and platform, and is never linked by a
#  shell. Nothing here includes a Windows header - everything platform-specific
#  goes through famulus_platform.h, and only core/bridge/ includes it at all.
# ============================================================================

set(CMAKE_FOLDER "core")

set(CORE_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.mm")

# --- famulus_core: everything the daemon thinks with ------------------------

if(CORE_SOURCES)
    add_library(famulus_core STATIC ${CORE_SOURCES})

    famulus_apply_warnings(famulus_core)

    # Headers live next to their class, so the daemon can include
    # "server/daemon_server.hpp" without a public/ tree.
    target_include_directories(famulus_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

    target_link_libraries(famulus_core
        PUBLIC famulus_base famulus_proto
        PRIVATE famulus_platform_api)
else()
    message(STATUS "famulus: famulus_core skipped (no sources in a src/core/ subdirectory)")
endif()

# --- famulusd: the resident daemon -----------------------------------------

if(EXISTS "${CORE_MAIN}")
    add_executable(famulusd "${CORE_MAIN}")
    famulus_apply_warnings(famulusd)

    target_link_libraries(famulusd PRIVATE famulus_platform)

    if(TARGET famulus_core)
        target_link_libraries(famulusd PRIVATE famulus_core)
    endif()
else()
    message(STATUS "famulus: famulusd skipped (no src/core/main.cpp yet)")
endif()
