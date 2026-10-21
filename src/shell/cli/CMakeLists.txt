# ============================================================================
#  src/shell/cli/ - famulus_shell_cli
#
#  The headless surface. A terminal is a surface like any other, just with the
#  smallest capability set: no notifications, no rich text, no forms. Artifacts
#  that need a form are therefore not renderable here and stay pending, which is
#  what makes the degradation ladder testable from the first milestone.
#
#  Links base and platform (for framing and the transport), never core.
# ============================================================================

set(CMAKE_FOLDER "shell")

famulus_executable(famulus_shell_cli src)

if(TARGET famulus_shell_cli)
    target_link_libraries(famulus_shell_cli PRIVATE
        famulus_proto
        famulus_base
        famulus_platform)

    # Target name follows the path; the artifact is named for the person typing
    # it. Only executables differ - a library's name is its identity.
    set_target_properties(famulus_shell_cli PROPERTIES OUTPUT_NAME famulusctl)
endif()
