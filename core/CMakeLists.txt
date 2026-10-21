# ============================================================================
#  core/ - the brain
#
#  One source directory. famulus_core is every source under src/ except
#  src/main.cpp; famulusd is src/main.cpp linked against it. The entry point is
#  excluded by name right here rather than by giving it a directory of its own,
#  so the rule is visible in one place:
#
#      src/main.cpp   -> famulusd only, never famulus_core
#
#  The reason it matters: a static library that carries a main() collides with
#  any test main, and only sometimes - the linker may never pull that object.
# ============================================================================

famulus_library(famulus_core STATIC src)

if(TARGET famulus_core)
    get_target_property(_core_sources famulus_core SOURCES)
    list(FILTER _core_sources EXCLUDE REGEX "[\\\\/]main\\.cpp$")

    if(_core_sources)
        set_target_properties(famulus_core PROPERTIES SOURCES "${_core_sources}")
    endif()

    target_link_libraries(famulus_core
        PUBLIC famulus_proto
        PRIVATE famulus_os_api)
endif()

# --- famulusd: the resident daemon -----------------------------------------

set(FAMULUS_DAEMON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

if(EXISTS "${FAMULUS_DAEMON_SOURCE}")
    if(NOT TARGET famulus_core)
        message(FATAL_ERROR "src/main.cpp exists but famulus_core has no other sources")
    endif()

    add_executable(famulusd "${FAMULUS_DAEMON_SOURCE}")
    target_link_libraries(famulusd PRIVATE famulus_warnings famulus_core)

    if(TARGET ${FAMULUS_LEAF_TARGET})
        target_link_libraries(famulusd PRIVATE ${FAMULUS_LEAF_TARGET})
    else()
        message(WARNING "famulusd built without an OS adapter: ${FAMULUS_LEAF_TARGET} has no sources yet")
    endif()
else()
    message(STATUS "famulus: famulusd skipped (no src/main.cpp yet)")
endif()
