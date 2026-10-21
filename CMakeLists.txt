# ============================================================================
#  famulus - top level
#
#  Layout and the boundaries it encodes:
#
#    src/proto/     the contract every process shares (IPC, artifacts, blocks)
#    src/core/      the brain: policy, state, providers, tools, IPC server
#    src/os/        the OS adapter: the only holder of system authority
#    src/daemon/    the famulusd entry point, a sibling of core/ so the library
#                   can never pick it up
#    src/shell/     the surfaces: cli and win-desktop, both protocol clients
#    tests/         every test, deliberately outside src/ because no test ships
#
#  The boundaries are enforced by the link graph, not by directory layout:
#  src/os never links src/core, and src/shell links neither.
#
#  Language roles: C++20 is the project language, C99 is the boundary language
#  (src/os/api/famulus_os.h), Objective-C++ is allowed inside src/os/macos only.
# ============================================================================
cmake_minimum_required(VERSION 3.28)

project(famulus
    VERSION 0.1.0
    DESCRIPTION "Resident assistant: C++ daemon, C OS boundary, protocol-driven shells"
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
add_subdirectory(src/os)
add_subdirectory(src/core)
add_subdirectory(src/daemon)
add_subdirectory(src/shell)
add_subdirectory(tests)

message(STATUS "famulus ${PROJECT_VERSION}: os=${FAMULUS_OS} tests=${FAMULUS_BUILD_TESTS} shell=${FAMULUS_BUILD_SHELL} werror=${FAMULUS_WERROR}")
