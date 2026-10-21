# ============================================================================
#  famulus - OS adapter selection
#
#  Exactly one adapter is compiled into famulusd. The others stay in the tree
#  as reference and are not built, which is why core/ never needs an #ifdef.
#
#  An adapter name is also the path component it lives in: FAMULUS_OS=windows
#  means os/windows, so the value, the directory and the target name
#  (famulus_os_windows) all say the same thing.
# ============================================================================

set(FAMULUS_OS_SUPPORTED windows macos linux null mock)

if(NOT FAMULUS_OS)
    if(WIN32)
        set(FAMULUS_OS windows)
    elseif(APPLE)
        set(FAMULUS_OS macos)
    elseif(UNIX)
        set(FAMULUS_OS linux)
    else()
        set(FAMULUS_OS null)
    endif()
endif()

if(NOT FAMULUS_OS IN_LIST FAMULUS_OS_SUPPORTED)
    message(FATAL_ERROR
        "FAMULUS_OS='${FAMULUS_OS}' is not a known adapter; pick one of: ${FAMULUS_OS_SUPPORTED}")
endif()

set(FAMULUS_OS "${FAMULUS_OS}" CACHE STRING
    "OS adapter to link: windows | macos | linux | null | mock" FORCE)

# An adapter written for one OS cannot be built on another. "null" and "mock"
# build anywhere, which is exactly what makes them useful.
if(FAMULUS_OS STREQUAL "windows" AND NOT WIN32)
    message(FATAL_ERROR "FAMULUS_OS=windows can only be built on Windows")
endif()

if(FAMULUS_OS STREQUAL "macos" AND NOT APPLE)
    message(FATAL_ERROR "FAMULUS_OS=macos can only be built on macOS")
endif()

if(FAMULUS_OS STREQUAL "linux" AND NOT (UNIX AND NOT APPLE))
    message(FATAL_ERROR "FAMULUS_OS=linux can only be built on Linux")
endif()

# Single name for the adapter target; consumers use this instead of guessing.
set(FAMULUS_LEAF_TARGET "famulus_os_${FAMULUS_OS}")
