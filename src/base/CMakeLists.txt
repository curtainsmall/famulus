# ============================================================================
#  src/base/ - our own foundation
#
#  Platform-independent code that both the daemon and every shell may use:
#  message framing, encoding and decoding, and small utilities as they appear.
#
#  Dependency rule: base depends on nothing else in this tree. If something here
#  reaches for src/platform/, src/core/ or a shell, it belongs somewhere else.
#  This is the same role VS Code gives src/vs/base and systemd gives src/basic.
# ============================================================================

set(CMAKE_FOLDER "lib")

famulus_library(famulus_base STATIC src)

if(TARGET famulus_base)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_include_directories(famulus_base PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif()
endif()
