# ============================================================================
#  src/base/ - our own foundation
#
#  Platform-independent code that both the daemon and every shell may use:
#  message framing, encoding and decoding, and small utilities as they appear.
#
#  Dependency rule: base depends on nothing else in this tree. If something here
#  reaches for src/platform/, src/core/ or a shell, it belongs somewhere else.
#  This is the same role VS Code gives src/vs/base and systemd gives src/basic.
# ============================================================================

set(CMAKE_FOLDER "lib")

famulus_library(famulus_base STATIC src)

if(NOT TARGET famulus_base)
    # An empty layer is still a layer: consumers keep their link line and this
    # becomes a real library the moment a source lands here.
    add_library(famulus_base INTERFACE)
endif()
