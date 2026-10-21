# ============================================================================
#  famulus - platform adapter selection
#
#  "Platform" means the environment the daemon runs in, not an operating
#  system: a desktop OS, a server, a cloud backend. Everything that belongs to
#  that environment - a global hotkey, a notification channel, a scheduler, a
#  secret store, the local transport - sits behind src/platform/api, with one
#  adapter per platform behind it.
#
#  Only the Windows adapter exists today. FAMULUS_PLATFORM stays a real option
#  rather than a hard-coded name because it names the seam a second platform
#  plugs into: adding macOS, Linux or a cloud adapter means adding
#  src/platform/<name> plus one entry in FAMULUS_PLATFORM_SUPPORTED.
#
#  An adapter name is also the path component it lives in: FAMULUS_PLATFORM=windows
#  means src/platform/windows and famulus_platform_windows.
# ============================================================================

set(FAMULUS_PLATFORM_SUPPORTED windows null)

if(NOT FAMULUS_PLATFORM)
    if(WIN32)
        set(FAMULUS_PLATFORM windows)
    else()
        # No adapter for this host yet. "null" is the honest fallback and also
        # the shape a cloud deployment takes: it builds anywhere and reports
        # every capability as missing.
        set(FAMULUS_PLATFORM null)
        message(STATUS "famulus: no adapter for this host yet, using FAMULUS_PLATFORM=null")
    endif()
endif()

if(NOT FAMULUS_PLATFORM IN_LIST FAMULUS_PLATFORM_SUPPORTED)
    message(FATAL_ERROR
        "FAMULUS_PLATFORM='${FAMULUS_PLATFORM}' is not available. The project is cross-platform "
        "by design, but only the Windows adapter is implemented so far; available today: "
        "${FAMULUS_PLATFORM_SUPPORTED}")
endif()

set(FAMULUS_PLATFORM "${FAMULUS_PLATFORM}" CACHE STRING
    "Platform adapter to link: windows | null (only windows is implemented)" FORCE)

# An adapter written for one platform cannot be built for another. "null" builds
# anywhere, which is exactly what makes it useful as the portability gate.
if(FAMULUS_PLATFORM STREQUAL "windows" AND NOT WIN32)
    message(FATAL_ERROR "FAMULUS_PLATFORM=windows can only be built on Windows")
endif()

# The test double is not an adapter: it is built and linked by tests/ only, so
# it is deliberately absent from FAMULUS_PLATFORM_SUPPORTED. This file describes
# what the product can be, not what can be tested.

# Consumers link this name and get the headers plus whichever adapter was
# selected; the choice happens in src/platform/CMakeLists.txt and nowhere else.
set(FAMULUS_PLATFORM_TARGET famulus_platform)
