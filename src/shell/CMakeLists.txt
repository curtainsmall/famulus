# ============================================================================
#  src/shell/ - every surface is a protocol client
#
#  A shell speaks proto/ messages over IPC and owns nothing but presentation and
#  input. It must never link famulus_core or an platform adapter: that is what keeps
#  the daemon replaceable underneath it and lets several shells exist at once.
#
#  Each shell declares a different capability set, which is what makes the
#  artifact degradation ladder testable instead of theoretical.
#
#  Shells are split by platform, not by technology: the window behaviour a shell
#  needs - overlay without focus stealing, IME, DPI - is platform code, and
#  hiding it behind a shared directory only pretends otherwise.
# ============================================================================

if(FAMULUS_BUILD_SHELL)
    add_subdirectory(cli)
    add_subdirectory(win-desktop)
endif()
