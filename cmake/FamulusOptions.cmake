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

# Dependencies are populated by default: a checkout should build with one command
# on any machine, and FetchContent is what makes that true. Turning this off
# leaves the project configured for the day a dependency arrives from somewhere
# else (a system package, a mirrored build/deps); it is not an offline mode,
# because src/base needs spdlog.
option(FAMULUS_FETCH_DEPS "Populate third-party dependencies with FetchContent" ON)

# The platform adapter linked into famulusd. Left empty, FamulusPlatform picks the
# host's adapter. This is a real option and not a hard-coded name on purpose:
# the project is cross-platform by design and this names the seam a second
# platform plugs into, even though only the Windows adapter exists today.
# "null" proves src/core builds with no platform code at all - that is the
# portability gate CI runs.
set(FAMULUS_PLATFORM "" CACHE STRING "platform adapter to link: windows | null (only windows is implemented)")
set_property(CACHE FAMULUS_PLATFORM PROPERTY STRINGS windows null)
