# ============================================================================
#  famulus - OS adapter selection
#
#  The project is cross-platform by design: every OS-specific capability sits
#  behind one C contract (src/os/api/famulus_os.h) with one adapter per platform
#  behind it, and nothing in src/core knows which one was linked.
#
#  Only the Windows adapter is implemented today. FAMULUS_OS therefore stays a
#  real option rather than a hard-coded name: it names the seam that a second
#  platform plugs into, so adding macOS or Linux means adding src/os/<name> and
#  one entry in FAMULUS_OS_SUPPORTED - not redesigning the build.
#
#  An adapter name is also the path component it lives in: FAMULUS_OS=windows
#  means src/os/windows and famulus_os_windows.
# ============================================================================

set(FAMULUS_OS_SUPPORTED windows null)

if(NOT FAMULUS_OS)
    if(WIN32)
        set(FAMULUS_OS windows)
    else()
        # No real adapter for this host yet. The null adapter is the honest
        # fallback: it builds anywhere and reports every capability as missing.
        set(FAMULUS_OS null)
        message(STATUS "famulus: no adapter for this host yet, using FAMULUS_OS=null")
    endif()
endif()

if(NOT FAMULUS_OS IN_LIST FAMULUS_OS_SUPPORTED)
    message(FATAL_ERROR
        "FAMULUS_OS='${FAMULUS_OS}' is not available. The project is cross-platform by design, "
        "but only the Windows adapter is implemented so far; available today: ${FAMULUS_OS_SUPPORTED}")
endif()

set(FAMULUS_OS "${FAMULUS_OS}" CACHE STRING
    "OS adapter to link: windows | null (only windows is implemented)" FORCE)

# An adapter written for one OS cannot be built on another. The null adapter
# builds anywhere, which is exactly what makes it useful as the portability gate.
if(FAMULUS_OS STREQUAL "windows" AND NOT WIN32)
    message(FATAL_ERROR "FAMULUS_OS=windows can only be built on Windows")
endif()

# The test double is not an adapter: it is built and linked by tests/ only, so
# it is deliberately absent from FAMULUS_OS_SUPPORTED. This file describes what
# the product can be, not what can be tested.

# Single name for the adapter target; consumers use this instead of guessing.
set(FAMULUS_LEAF_TARGET "famulus_os_${FAMULUS_OS}")
