# ============================================================================
#  tests/ - every test lives here, and only here
#
#  The source tree carries no tests and no test targets: a component is not
#  responsible for proving itself correct. The layout below therefore does not
#  mirror src/ - suites are grouped by what they need, not by which module they
#  happen to touch.
#
#    support/platform_mock/  the platform test double
#    core/                   one process, links famulus_core, sees internals
#    protocol/               the protocol only, cannot see internals
#
#  The difference in link targets is the point. A protocol test that could
#  include a core header would stop being evidence that the protocol is enough.
# ============================================================================

# --- support: the platform double, a test double rather than a test ---------
# Built here because nothing in the product links it: famulusd links a real
# adapter, and a shell never links an adapter implementation at all.

set(CMAKE_FOLDER "tests")

famulus_library(famulus_platform_mock STATIC support/platform_mock/src)

if(TARGET famulus_platform_mock)
    target_link_libraries(famulus_platform_mock PUBLIC famulus_platform_api)

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/support/platform_mock/include")
        target_include_directories(famulus_platform_mock PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/support/platform_mock/include")
    endif()
endif()

# --- suite: core -----------------------------------------------------------
# White box: this is where behaviour that is not yet expressible over the
# protocol gets tested, so that the protocol suite never needs an internal type.

famulus_test_target(famulus_core_tests core)

if(TARGET famulus_core_tests)
    if(NOT TARGET famulus_core)
        message(FATAL_ERROR "core tests exist but famulus_core does not: add sources under src/core/")
    endif()

    target_link_libraries(famulus_core_tests PRIVATE famulus_core famulus_platform_api)

    if(TARGET famulus_platform_mock)
        target_link_libraries(famulus_core_tests PRIVATE famulus_platform_mock)
    endif()
endif()

# --- suite: protocol -------------------------------------------------------
# Black box: a real daemon, a real transport, no core header in sight. If a
# behaviour can only be checked here by including core, that is a finding about
# the protocol, not a reason to widen this target.

famulus_test_target(famulus_protocol_tests protocol)

if(TARGET famulus_protocol_tests)
    target_link_libraries(famulus_protocol_tests PRIVATE
        famulus_proto
        famulus_base
        famulus_platform_api)

    if(TARGET famulus_platform_mock)
        target_link_libraries(famulus_protocol_tests PRIVATE famulus_platform_mock)
    endif()

    # These tests drive real binaries, so build them first.
    foreach(_dep famulusd famulus_shell_cli)
        if(TARGET ${_dep})
            add_dependencies(famulus_protocol_tests ${_dep})
        endif()
    endforeach()
endif()
