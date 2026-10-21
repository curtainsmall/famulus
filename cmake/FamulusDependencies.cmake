# ============================================================================
#  famulus - third-party dependencies
#
#  FetchContent, not a package manager. A dependency is populated into
#  build/deps and then configured as part of this build, so it shares one
#  compiler, one C++ standard, one warning policy and one configuration with
#  our own code. Nothing is installed to a prefix and nothing escapes build/.
#
#  Population is OFF by default (FAMULUS_FETCH_DEPS): a checkout with no
#  network must still configure and build. Turn it on when a source file
#  actually includes one of these headers or links one of these targets.
#
#  Rules for every entry:
#    - pinned to an immutable tag or commit, never a branch
#    - SYSTEM        keeps third-party headers out of our -Werror wall
#    - EXCLUDE_FROM_ALL  keeps their targets and install rules out of ours
#    - bumping a dependency is one line, and the line is the version
# ============================================================================

if(NOT FAMULUS_FETCH_DEPS)
    message(STATUS "famulus: dependencies not populated (FAMULUS_FETCH_DEPS=OFF)")
    return()
endif()

include(FetchContent)

# Deliberately outside the per-preset binary dir so switching presets does not
# refetch, and deliberately inside build/ so the tree stays clean.
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/build/deps" CACHE PATH
    "Shared dependency tree for FetchContent, so presets do not refetch")

set(FETCHCONTENT_QUIET OFF)

# --- header-mostly libraries ----------------------------------------------

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(nlohmann_json asio spdlog)

# --- SQLite ----------------------------------------------------------------
# The amalgamation ships no top-level CMakeLists, so the target is ours: one
# translation unit with the flags this project needs. SQLITE_THREADSAFE=2 is
# multi-thread mode - no connection may be shared by two threads, which matches
# one connection per session strand.

FetchContent_Declare(sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(sqlite3)

add_library(famulus_sqlite3 STATIC "${sqlite3_SOURCE_DIR}/sqlite3.c")

target_include_directories(famulus_sqlite3 SYSTEM PUBLIC
    "${sqlite3_SOURCE_DIR}")

target_compile_definitions(famulus_sqlite3 PUBLIC
    SQLITE_THREADSAFE=2
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_JSON1)

# famulus_warnings is deliberately NOT linked: upstream C is not /W4 clean and
# holding it to our -Werror would only teach us to ignore warnings.

# --- tests -----------------------------------------------------------------

if(FAMULUS_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
        SYSTEM
        EXCLUDE_FROM_ALL)

    FetchContent_MakeAvailable(googletest)
endif()
