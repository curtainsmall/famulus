# ============================================================================
#  famulus - build options
#
#  Every option here is meant to survive the project. A one-off switch belongs
#  in a local preset as a cache variable, not as a new option.
# ============================================================================

option(FAMULUS_BUILD_TESTS "Build unit and integration test targets" ON)
option(FAMULUS_BUILD_SHELL "Build the shells (famulusctl, famulus_ui)" ON)
option(FAMULUS_WERROR "Treat compiler warnings as errors in first-party targets" ON)
option(FAMULUS_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang only)" OFF)

# Dependency population is a deliberate step, not a side effect of configure:
# a checkout must be able to configure and build with no network at all. Flip
# this on in a preset once a source actually needs json, asio, spdlog, sqlite3
# or GTest.
option(FAMULUS_FETCH_DEPS "Populate third-party dependencies with FetchContent" OFF)

# The OS adapter linked into famulusd. Left empty, FamulusPlatform picks the
# host's adapter. Setting it to "null" proves core/ builds with no platform
# code at all - that is the portability gate CI runs.
set(FAMULUS_OS "" CACHE STRING "OS adapter to link: windows | macos | linux | null | mock")
set_property(CACHE FAMULUS_OS PROPERTY STRINGS windows macos linux null mock)
