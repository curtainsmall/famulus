# ============================================================================
#  src/platform/ - the abstracted environment
#
#  Three targets, and the split is what lets a shell link the platform without
#  gaining the right to call a hotkey from the wrong process:
#
#    famulus_platform_api    INTERFACE  the headers: famulus_platform.h (C
#                            capabilities) and famulus_transport.h (C++ byte
#                            stream). No code, no selection.
#    famulus_platform_<name> STATIC     one adapter: src/platform/windows,
#                            src/platform/null. Capabilities plus the local
#                            transport implementation.
#    famulus_platform        INTERFACE  the facade consumers link. Carries the
#                            headers and whichever adapter the build selected.
#
#  The selection happens here and nowhere else, so no consumer repeats a
#  platform question.
#
#  Dependency rule: src/platform may use src/base and nothing else from this
#  tree. It never includes a core header and has no idea what a session is.
# ============================================================================

set(FAMULUS_PLATFORM_ADAPTER "famulus_platform_${FAMULUS_PLATFORM}")

# --- the headers -----------------------------------------------------------

add_library(famulus_platform_api INTERFACE)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/api")
    target_include_directories(famulus_platform_api INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/api")
endif()

# --- the selected adapter --------------------------------------------------

famulus_library(${FAMULUS_PLATFORM_ADAPTER} STATIC "${FAMULUS_PLATFORM}")

if(TARGET ${FAMULUS_PLATFORM_ADAPTER})
    target_link_libraries(${FAMULUS_PLATFORM_ADAPTER} PRIVATE famulus_platform_api)

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${FAMULUS_PLATFORM}/include")
        target_include_directories(${FAMULUS_PLATFORM_ADAPTER} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/${FAMULUS_PLATFORM}/include")
    endif()
endif()

# --- the facade ------------------------------------------------------------

add_library(famulus_platform INTERFACE)

target_link_libraries(famulus_platform INTERFACE famulus_platform_api)

if(TARGET ${FAMULUS_PLATFORM_ADAPTER})
    target_link_libraries(famulus_platform INTERFACE ${FAMULUS_PLATFORM_ADAPTER})
else()
    message(STATUS "famulus: ${FAMULUS_PLATFORM_ADAPTER} has no sources yet; famulus_platform carries headers only")
endif()
