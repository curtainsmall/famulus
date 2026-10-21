# ============================================================================
#  src/core/ - the brain
#
#  famulus_core is every source under this directory. The process entry point
#  lives next door in src/daemon/, so the library is structurally incapable of
#  carrying a main() - no exclusion logic, no filename convention to remember.
# ============================================================================

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm")

if(CORE_SOURCES)
    add_library(famulus_core STATIC ${CORE_SOURCES})

    target_link_libraries(famulus_core
        PRIVATE famulus_warnings
        PUBLIC famulus_proto
        PRIVATE famulus_os_api)
else()
    message(STATUS "famulus: famulus_core skipped (no sources yet)")
endif()
