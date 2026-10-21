# ============================================================================
#  famulus - diagnostics and sanitizers
#
#  .clang-format owns layout, this file owns diagnostics. Warnings are applied
#  per target through famulus_apply_warnings() rather than globally on purpose:
#  global flags would also hit third-party code arriving through FetchContent,
#  and holding someone else's headers to -Werror only teaches people to ignore
#  warnings. This is a function, not a target: it is a build setting, not a
#  component.
# ============================================================================

function(famulus_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8                  # sources and literals are UTF-8, see .editorconfig
            /Zc:__cplusplus         # report the real standard in __cplusplus
            /Zc:preprocessor)       # conforming preprocessor, required by many headers

        if(FAMULUS_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow)

        # C++-only diagnostics must not reach the C translation units in
        # src/platform/; an unknown -W flag would become an error under -Werror.
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
            $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>)

        if(FAMULUS_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if(FAMULUS_SANITIZE)
        if(MSVC)
            message(WARNING "FAMULUS_SANITIZE is ignored on MSVC; use an asan preset instead")
        else()
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined)
        endif()
    endif()
endfunction()
