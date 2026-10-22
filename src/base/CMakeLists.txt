# ============================================================================
#  src/base/ - our own foundation
#
#  Platform-independent code that both the daemon and every shell may use:
#  logging, message framing, encoding and decoding, and small utilities as they
#  appear.
#
#  Dependency rule: base depends on nothing else in this tree, and on one
#  third-party library (spdlog, populated by FetchContent). If something here
#  reaches for src/platform/, src/core/ or a shell, it belongs somewhere else.
# ============================================================================

set(CMAKE_FOLDER "lib")

famulus_library(famulus_base STATIC impl)

if(NOT TARGET famulus_base)
    # An empty layer is still a layer: consumers keep their link line and this
    # becomes a real library the moment a source lands here.
    add_library(famulus_base INTERFACE)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    get_target_property(FAMULUS_BASE_TYPE famulus_base TYPE)

    if(FAMULUS_BASE_TYPE STREQUAL "INTERFACE_LIBRARY")
        target_include_directories(famulus_base INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/include")
    else()
        # PUBLIC: the library's own sources need the header, and so do consumers.
        target_include_directories(famulus_base PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include")

        if(NOT TARGET spdlog::spdlog)
            message(FATAL_ERROR
                "famulus: src/base needs spdlog. Configure with -DFAMULUS_FETCH_DEPS=ON once, "
                "the fetched copy is reused from build/ afterwards.")
        endif()

        target_link_libraries(famulus_base PRIVATE spdlog::spdlog)
    endif()
endif()
