# ============================================================================
#  famulus - top level
#
#  Layout and the boundaries it encodes:
#
#    src/proto/     the contract every process shares (IPC, artifacts, blocks)
#    src/core/      the brain: policy, state, providers, tools, IPC server
#    src/platform/  the platform adapter: the only holder of system authority,
#                   where "platform" means desktop OS, server or cloud backend
#    src/daemon/    the famulusd entry point, a sibling of core/ so the library
#                   can never pick it up
#    src/shell/     the surfaces: cli and win-desktop, both protocol clients
#    tests/         every test, deliberately outside src/ because no test ships
#
#  The boundaries are enforced by the link graph, not by directory layout:
#  src/platform never links src/core, and src/shell links neither.
#
#  The project is cross-platform by design, but only the Windows adapter is
#  implemented so far: every platform capability sits behind one C contract
#  (src/platform/api/famulus_platform.h) with one adapter per platform behind
#  it, so a second platform adds src/platform/<name> instead of changing
#  src/core.
#
#  Language roles: C++20 is the project language, C99 is the boundary language;
#  a future macOS adapter may use Objective-C++ inside its own directory only.
# ============================================================================
cmake_minimum_required(VERSION 3.28)

project(famulus
    VERSION 0.1.0
    DESCRIPTION "Resident assistant: C++ daemon, C platform boundary, protocol-driven shells"
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Everything a configure produces lives under build/ (see CMakePresets.json and
# FamulusDependencies.cmake): the per-preset trees, the dependency sources and
# the install trees. The source tree stays clean, so a full reset is just
# `git clean -xdf`.

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FamulusOptions)
include(FamulusPlatform)
include(FamulusWarnings)
include(FamulusDependencies)
include(FamulusTesting)
include(FamulusTargets)

add_subdirectory(src/proto)
add_subdirectory(src/platform)
add_subdirectory(src/core)
add_subdirectory(src/daemon)
add_subdirectory(src/shell)
add_subdirectory(tests)

message(STATUS "famulus ${PROJECT_VERSION}: platform=${FAMULUS_PLATFORM} tests=${FAMULUS_BUILD_TESTS} shell=${FAMULUS_BUILD_SHELL} werror=${FAMULUS_WERROR}")
