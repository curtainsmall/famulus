# ============================================================================
#  src/shell/win-desktop/ - famulus_ui, the Windows desktop shell
#
#  Native, not a cross-platform shell pretending: an overlay that does not steal
#  focus, a tabbed main window, and a notification adapter that hands artifacts
#  to the OS. Those three are the same product surfaces on every OS but not the
#  same code - frameless, always-on-top, non-activating focus and IME behave
#  differently on each platform, so each platform gets its own shell directory.
#
#  Links famulus_proto only. Adding famulus_core or famulus_os_* here would
#  break crash isolation, multi-client support and the single permission
#  identity, so do not.
# ============================================================================

famulus_executable(famulus_ui app)

if(TARGET famulus_ui)
    target_link_libraries(famulus_ui PRIVATE famulus_proto)

    if(WIN32)
        # A GUI subsystem binary: no console window behind the overlay.
        set_target_properties(famulus_ui PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()