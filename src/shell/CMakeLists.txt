# ============================================================================
#  src/shell/ - every surface is a protocol client
#
#  A shell speaks the protocol over the local transport and owns nothing but
#  presentation and input. It links base and platform, never core: that is what
#  keeps the daemon replaceable underneath it and lets several shells exist at
#  once.
#
#  Shells are split by platform, not by technology: the window behaviour a shell
#  needs - overlay without focus stealing, IME, DPI - is platform code, and
#  hiding it behind a shared directory only pretends otherwise. The CLI is the
#  exception because a terminal has no window behaviour to speak of, so it stays
#  a single portable implementation.
# ============================================================================

if(FAMULUS_BUILD_SHELL)
    add_subdirectory(cli)
    add_subdirectory(win-desktop)
endif()
