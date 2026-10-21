# ============================================================================
#  src/os/ - the OS adapter, the only holder of system authority
#
#  famulus_os_api carries src/os/api/famulus_os.h, the pure C99 contract. An
#  adapter may include that header and nothing else from this project - it does
#  not link core, never includes a core header, and has no idea what an artifact
#  or a session is.
#
#  Exactly one adapter is built as famulus_os_<platform> and linked into
#  famulusd. The null adapter builds anywhere and returns FAM_E_UNSUPPORTED from
#  every entry point, which is both the server variant and the portability gate.
#
#  Every directory here is created by whoever adds the first file to it: the
#  globs and the include guard below make an empty os/ legitimate, so nothing
#  has to be stubbed out in advance.
# ============================================================================

add_library(famulus_os_api INTERFACE)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/api")
    target_include_directories(famulus_os_api INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/api")
endif()

famulus_library(${FAMULUS_LEAF_TARGET} STATIC "${FAMULUS_OS}/src")

if(TARGET ${FAMULUS_LEAF_TARGET})
    target_link_libraries(${FAMULUS_LEAF_TARGET} PUBLIC famulus_os_api)

    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${FAMULUS_OS}/include")
        target_include_directories(${FAMULUS_LEAF_TARGET} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/${FAMULUS_OS}/include")
    endif()
endif()
