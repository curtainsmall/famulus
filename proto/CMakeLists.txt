# ============================================================================
#  proto/ - the only contract every process shares
#
#  The contract itself does not exist yet. This target exists so core/ and the
#  shells have one thing to depend on once the schema lands; the directories it
#  will use are created by whoever adds the first file, not in advance.
#
#    schema/*.json          hand written, the authoritative definition
#    include/               hand written C++ that is not generated
#    <build>/proto/gen/     types generated from the schema
#
#  Include paths are added only for directories that actually exist, so an
#  empty proto/ is not a broken proto/.
# ============================================================================

add_library(famulus_proto INTERFACE)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")

target_include_directories(famulus_proto INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/gen>")

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(famulus_proto INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
endif()
