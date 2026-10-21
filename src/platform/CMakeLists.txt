# ============================================================================
#  src/platform/ - the platform adapter, the only holder of system authority
#
#  famulus_platform_api carries src/platform/api/famulus_platform.h, the pure C99
#  contract. An adapter may include that header and nothing else from this
#  project - it does not link core, never includes a core header, and has no idea
#  what an artifact or a session is.
#
#  Exactly one adapter is built as famulus_platform_<platform> and linked into
#  famulusd. The null adapter builds anywhere and returns FAM_E_UNSUPPORTED from
#  every entry point: it is both the cloud/server variant and the portability
#  gate that proves src/core holds no platform code.
#
#  Every directory here is created by whoever adds the first file to it: the
#  globs and the include guard below make an empty src/platform/ legitimate, so
#  nothing has to be stubbed out in advance.
# ============================================================================

add_library(famulus_platform_api INTERFACE)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/api")
    target_include_directories(famulus_platform_api INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/api")
endif()

famulus_library(${FAMULUS_LEAF_TARGET} STATIC "${FAMULUS_PLATFORM}/src")

if(TARGET ${FAMULUS_LEAF_TARGET})
    target_link_libraries(${FAMULUS_LEAF_TARGET} PUBLIC famulus_platform_api)

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${FAMULUS_PLATFORM}/include")
        target_include_directories(${FAMULUS_LEAF_TARGET} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/${FAMULUS_PLATFORM}/include")
    endif()
endif()
