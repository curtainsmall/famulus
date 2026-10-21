# ============================================================================
#  src/shell/win-desktop/ - famulus_shell_win_desktop
#
#  Native, not a cross-platform shell pretending: an overlay that does not steal
#  focus, a tabbed main window, and a notification adapter that hands artifacts
#  to the platform. Those three are the same product surfaces everywhere but not
#  the same code - frameless, always-on-top, non-activating focus and IME behave
#  differently on each platform, so each platform gets its own shell directory.
#
#  Links base and platform, never core.
# ============================================================================

famulus_executable(famulus_shell_win_desktop src)

if(TARGET famulus_shell_win_desktop)
    target_link_libraries(famulus_shell_win_desktop PRIVATE
        famulus_proto
        famulus_base
        famulus_platform)

    # Target name follows the path; the artifact is the product name, because
    # this is the thing a person double-clicks.
    set_target_properties(famulus_shell_win_desktop PROPERTIES OUTPUT_NAME famulus)

    if(WIN32)
        # A GUI subsystem binary: no console window behind the overlay.
        set_target_properties(famulus_shell_win_desktop PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
