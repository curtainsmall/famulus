# ============================================================================
#  famulus - top level
#
#  Layout, and the dependency rule each directory obeys. The rule is the same
#  shape systemd and VS Code use: a directory may only depend on those above it
#  in this list, never on those below.
#
#    src/proto/      the contract: schemas, message types, constants. No code.
#    src/base/       our own foundation: framing, encoding, small utilities.
#                    Depends on nothing else in the tree.
#    src/platform/   the abstracted environment: capabilities (hotkey,
#                    notification, scheduler, secrets, paths) and the local
#                    transport. Depends on base only.
#    src/core/       the daemon: sessions, artifacts, providers, scheduling,
#                    policy. Depends on base and platform. Never on shells.
#    src/shell/      the surfaces: cli, win-desktop. Depend on base and
#                    platform, never on core.
#    tests/          all tests, deliberately outside src/ because no test ships.
#
#  "Platform" here means the environment the daemon runs in, not an operating
#  system: a desktop OS, a server, a cloud backend. Only the Windows adapter
#  exists today; adding another means adding src/platform/<name>.
#
#  Everything generated lands under build/, so `git clean -xdf` is the only
#  reset needed.
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

# Targets are grouped by directory in the IDE's solution tree. Without this
# the FOLDER property is ignored and every target lands at the root.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FamulusOptions)
include(FamulusPlatform)
include(FamulusWarnings)
include(FamulusDependencies)
include(FamulusTesting)
include(FamulusTargets)

add_subdirectory(src/proto)
add_subdirectory(src/base)
add_subdirectory(src/platform)
add_subdirectory(src/core)
add_subdirectory(src/shell)
add_subdirectory(tests)

message(STATUS "famulus ${PROJECT_VERSION}: platform=${FAMULUS_PLATFORM} tests=${FAMULUS_BUILD_TESTS} shell=${FAMULUS_BUILD_SHELL} werror=${FAMULUS_WERROR}")
