# ============================================================================
#  src/shell/cli/ - famulusctl
#
#  The headless shell. A terminal is a surface like any other, just with the
#  smallest capability set: no notifications, no rich text, no forms. Artifacts
#  that need a form are therefore not renderable here and land in `pending`,
#  which is exactly the degradation path desktop-ui will exercise later.
#
#  Links famulus_proto only. Same rule as every other shell.
# ============================================================================

famulus_executable(famulusctl src)

if(TARGET famulusctl)
    target_link_libraries(famulusctl PRIVATE famulus_proto)
endif()
