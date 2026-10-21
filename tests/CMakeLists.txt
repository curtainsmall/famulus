# ============================================================================
#  tests/ - every test lives here, and only here
#
#  The source tree carries no tests and no test targets: a component is not
#  responsible for proving itself correct. The layout below therefore does not
#  mirror src/ - suites are grouped by what they need, not by which module they
#  happen to touch.
#
#    support/      doubles and fixtures shared by several suites (the OS mock)
#    unit/         one process, links famulus_core, may reach into internals
#    integration/  real processes, links the protocol only, cannot see internals
#
#  The difference in link targets is the point. Integration tests that could
#  include a core header would stop being evidence that the protocol is enough.
# ============================================================================

# --- support: the OS mock, a test double rather than a test -----------------
# Built here because nothing in the product links it: famulusd uses a real
# adapter and a shell never sees one at all.

famulus_library(famulus_os_mock STATIC support/os_mock/src)

if(TARGET famulus_os_mock)
    target_link_libraries(famulus_os_mock PUBLIC famulus_os_api)

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/support/os_mock/include")
        target_include_directories(famulus_os_mock PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/support/os_mock/include")
    endif()
endif()

# --- suite: unit -----------------------------------------------------------

famulus_test_target(famulus_unit_tests unit)

if(TARGET famulus_unit_tests)
    if(TARGET famulus_core)
        target_link_libraries(famulus_unit_tests PRIVATE famulus_core)
    else()
        message(FATAL_ERROR "unit tests exist but famulus_core does not: add sources under src/core/")
    endif()

    if(TARGET famulus_os_mock)
        target_link_libraries(famulus_unit_tests PRIVATE famulus_os_mock)
    endif()
endif()

# --- suite: integration ----------------------------------------------------

famulus_test_target(famulus_it_tests integration)

if(TARGET famulus_it_tests)
    target_link_libraries(famulus_it_tests PRIVATE famulus_proto)

    if(TARGET famulus_os_mock)
        target_link_libraries(famulus_it_tests PRIVATE famulus_os_mock)
    endif()

    # The suite drives real binaries, so build them before running it.
    foreach(_dep famulusd famulusctl)
        if(TARGET ${_dep})
            add_dependencies(famulus_it_tests ${_dep})
        endif()
    endforeach()
endif()
