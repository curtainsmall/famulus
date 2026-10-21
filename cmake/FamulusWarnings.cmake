# ============================================================================
#  famulus - diagnostics and sanitizers
#
#  .clang-format owns layout, this file owns diagnostics. Warnings apply to
#  first-party targets only: third-party code arrives through FetchContent as
#  SYSTEM targets and is never linked against famulus_warnings.
# ============================================================================

add_library(famulus_warnings INTERFACE)

if(MSVC)
    target_compile_options(famulus_warnings INTERFACE
        /W4
        /permissive-
        /utf-8                  # sources and literals are UTF-8, see .editorconfig
        /Zc:__cplusplus         # report the real standard in __cplusplus
        /Zc:preprocessor)       # conforming preprocessor, required by many headers

    if(FAMULUS_WERROR)
        target_compile_options(famulus_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(famulus_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow)

    # C++-only diagnostics must not reach the C translation units in src/platform/;
    # an unknown -W flag would turn into an error under -Werror.
    target_compile_options(famulus_warnings INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
        $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>)

    if(FAMULUS_WERROR)
        target_compile_options(famulus_warnings INTERFACE -Werror)
    endif()
endif()

if(FAMULUS_SANITIZE)
    if(MSVC)
        message(WARNING "FAMULUS_SANITIZE is ignored on MSVC; use the *_asan presets instead")
    else()
        target_compile_options(famulus_warnings INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer)
        target_link_options(famulus_warnings INTERFACE
            -fsanitize=address,undefined)
    endif()
endif()
