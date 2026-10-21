# ============================================================================
#  famulus - test plumbing
#
#  GoogleTest arrives through FamulusDependencies.cmake, which must be included
#  first: this file only checks whether the targets appeared. A checkout that
#  fails to populate one dependency should still configure the product, so a
#  missing GTest skips the suites with a status line instead of a hard error.
# ============================================================================

set(FAMULUS_GTEST_AVAILABLE OFF CACHE INTERNAL "" FORCE)

if(FAMULUS_BUILD_TESTS)
    if(TARGET GTest::gtest_main)
        set(FAMULUS_GTEST_AVAILABLE ON CACHE INTERNAL "" FORCE)
        include(GoogleTest)
    else()
        message(STATUS "famulus: GTest unavailable, test targets will be skipped")
    endif()
endif()
