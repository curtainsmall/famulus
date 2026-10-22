# ============================================================================
#  famulus - third-party dependencies
#
#  FetchContent, not a package manager. A dependency is populated into build/deps
#  and then configured as part of this build, so it shares one compiler, one C++
#  standard, one warning policy and one configuration with our own code. Nothing
#  is installed to a prefix and nothing escapes build/.
#
#  Downloaded by URL, not cloned by git. A URL needs nothing set up on the
#  machine - no git credentials, no SSH keys, no proxy variables, no ~/.gitconfig
#  - so this file says the same thing everywhere and a fresh checkout builds the
#  same way everywhere. The SHA256 under each URL is what pins the content: it
#  plays the role a tag plays for a clone, and an archive that changes fails the
#  build instead of silently replacing the dependency.
#
#  Rules for every entry:
#    - pinned by URL_HASH, never a branch and never an unverified archive
#    - SYSTEM        keeps third-party headers out of our -Werror wall
#    - EXCLUDE_FROM_ALL  keeps their targets and install rules out of ours
#    - bumping a dependency is two lines, and both of them are the version
# ============================================================================

if(NOT FAMULUS_FETCH_DEPS)
    message(STATUS "famulus: dependencies not populated (FAMULUS_FETCH_DEPS=OFF)")
    return()
endif()

include(FetchContent)

# Deliberately outside the per-preset binary dir so switching presets does not
# refetch, and deliberately inside build/ so the tree stays clean. FORCE because
# FetchContent defines this cache entry itself the moment the module loads, and a
# plain CACHE set would be a silent no-op.
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/build/deps" CACHE PATH
    "Shared dependency tree for FetchContent, so presets do not refetch" FORCE)

set(FETCHCONTENT_QUIET OFF)

# --- header-mostly libraries ----------------------------------------------

FetchContent_Declare(nlohmann_json
    URL https://codeload.github.com/nlohmann/json/tar.gz/refs/tags/v3.11.3
    URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_Declare(asio
    URL https://codeload.github.com/chriskohlhoff/asio/tar.gz/refs/tags/asio-1-30-2
    URL_HASH SHA256=755bd7f85a4b269c67ae0ea254907c078d408cce8e1a352ad2ed664d233780e8
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_Declare(spdlog
    URL https://codeload.github.com/gabime/spdlog/tar.gz/refs/tags/v1.14.1
    URL_HASH SHA256=1586508029a7d0670dfcb2d97575dcdc242d3868a259742b69f100801ab4e16b
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
    URL_HASH SHA256=5592243caf28b2cdef41e6ab58d25d653dfc53deded8450eb66072c929f030c4
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

# famulus_apply_warnings() is deliberately NOT applied: upstream C is not /W4 clean and
# holding it to our -Werror would only teach us to ignore warnings.

# --- tests -----------------------------------------------------------------

if(FAMULUS_BUILD_TESTS)
    FetchContent_Declare(googletest
        URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.14.0
        URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
        SYSTEM
        EXCLUDE_FROM_ALL)

    FetchContent_MakeAvailable(googletest)
endif()
