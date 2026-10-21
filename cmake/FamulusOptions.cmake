# ============================================================================
#  famulus - build options
#
#  Every option here is meant to survive the project. A one-off switch belongs
#  in a local preset as a cache variable, not as a new option.
# ============================================================================

option(FAMULUS_BUILD_TESTS "Build unit and integration test targets" ON)
option(FAMULUS_BUILD_SHELL "Build the shells (famulus_shell_cli, famulus_shell_win_desktop)" ON)
option(FAMULUS_WERROR "Treat compiler warnings as errors in first-party targets" ON)
option(FAMULUS_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang only)" OFF)

# Dependency population is a deliberate step, not a side effect of configure:
# a checkout must be able to configure and build with no network at all. Flip
# this on in a preset once a source actually needs json, asio, spdlog, sqlite3
# or GTest.
option(FAMULUS_FETCH_DEPS "Populate third-party dependencies with FetchContent" OFF)

# The platform adapter linked into famulusd. Left empty, FamulusPlatform picks the
# host's adapter. This is a real option and not a hard-coded name on purpose:
# the project is cross-platform by design and this names the seam a second
# platform plugs into, even though only the Windows adapter exists today.
# "null" proves src/core builds with no platform code at all - that is the
# portability gate CI runs.
set(FAMULUS_PLATFORM "" CACHE STRING "platform adapter to link: windows | null (only windows is implemented)")
set_property(CACHE FAMULUS_PLATFORM PROPERTY STRINGS windows null)
