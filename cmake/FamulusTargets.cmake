# ============================================================================
#  famulus - target helpers
#
#  Every first-party target is created through these helpers, which collect
#  sources from the given subdirectories of the current source directory and
#  skip the target while those directories are still empty. That keeps
#  `cmake --preset ...` green on a skeleton checkout without committing
#  placeholder .cpp files: a target appears the moment a real source lands.
#
#  Only compilable extensions are collected. Headers are intentionally not
#  globbed, so a directory holding nothing but headers cannot produce a target
#  whose link language CMake cannot infer.
# ============================================================================

# famulus_collect_sources(<out_var> <subdir>...)
function(famulus_collect_sources out)
    set(_sources "")

    foreach(_subdir IN LISTS ARGN)
        set(_base "${CMAKE_CURRENT_SOURCE_DIR}/${_subdir}")

        if(NOT IS_DIRECTORY "${_base}")
            continue()
        endif()

        file(GLOB_RECURSE _found CONFIGURE_DEPENDS
            "${_base}/*.c"
            "${_base}/*.cc"
            "${_base}/*.cpp"
            "${_base}/*.cxx"
            "${_base}/*.m"
            "${_base}/*.mm")

        list(APPEND _sources ${_found})
    endforeach()

    set(${out} "${_sources}" PARENT_SCOPE)
endfunction()

# famulus_library(<name> <STATIC|SHARED|OBJECT> <subdir>...)
function(famulus_library name kind)
    famulus_collect_sources(_sources ${ARGN})

    if(NOT _sources)
        message(STATUS "famulus: ${name} skipped (no sources yet)")
        return()
    endif()

    add_library(${name} ${kind} ${_sources})
    target_link_libraries(${name} PRIVATE famulus_warnings)
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

# famulus_executable(<name> <subdir>...)
function(famulus_executable name)
    famulus_collect_sources(_sources ${ARGN})

    if(NOT _sources)
        message(STATUS "famulus: ${name} skipped (no sources yet)")
        return()
    endif()

    add_executable(${name} ${_sources})
    target_link_libraries(${name} PRIVATE famulus_warnings)
endfunction()

# famulus_test_target(<name> <subdir>...)
# The caller is responsible for linking the target's subject and whatever double
# the suite needs, after checking `if(TARGET <name>)`.
function(famulus_test_target name)
    if(NOT FAMULUS_BUILD_TESTS OR NOT FAMULUS_GTEST_AVAILABLE)
        return()
    endif()

    famulus_collect_sources(_sources ${ARGN})

    if(NOT _sources)
        message(STATUS "famulus: ${name} skipped (no tests yet)")
        return()
    endif()

    add_executable(${name} ${_sources})
    target_link_libraries(${name} PRIVATE famulus_warnings GTest::gtest_main)
    gtest_discover_tests(${name})
endfunction()
