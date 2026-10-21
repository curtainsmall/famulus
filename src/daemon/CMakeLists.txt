# ============================================================================
#  src/daemon/ - famulusd
#
#  Process bootstrap only: argument parsing, single-instance acquisition, log
#  initialisation, the supervisor handshake and signal handling. It links the
#  library so it can start it, and the OS adapter so the daemon - and only the
#  daemon - holds system authority.
#
#  The directory is a sibling of src/core/ on purpose. A static library that
#  carries a main() collides with any test main, and only sometimes, because
#  the linker may never pull that object; keeping the entry point in its own
#  directory makes that impossible rather than merely unlikely.
# ============================================================================

file(GLOB DAEMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

if(DAEMON_SOURCES)
    add_executable(famulusd ${DAEMON_SOURCES})

    target_link_libraries(famulusd PRIVATE famulus_warnings)

    if(TARGET famulus_core)
        target_link_libraries(famulusd PRIVATE famulus_core)
    endif()

    if(TARGET ${FAMULUS_LEAF_TARGET})
        target_link_libraries(famulusd PRIVATE ${FAMULUS_LEAF_TARGET})
    else()
        message(WARNING "famulusd built without an OS adapter: ${FAMULUS_LEAF_TARGET} has no sources yet")
    endif()
else()
    message(STATUS "famulus: famulusd skipped (no sources yet)")
endif()
