# ============================================================================
#  src/proto/ - the contract
#
#  Content only: schemas, message types, constants. No framing, no encoding
#  logic, no .cpp. Those live in src/base/ because they are implementation.
#
#    schema/*.json          hand written, the authoritative definition
#    include/               hand written C++ types that are not generated
#    <build>/proto/gen/     types generated from the schema
#
#  Include paths are added only for directories that actually exist, so an empty
#  proto/ is not a broken proto/.
# ============================================================================

set(CMAKE_FOLDER "lib")

add_library(famulus_proto INTERFACE)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen")

target_include_directories(famulus_proto INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/gen>")

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(famulus_proto INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
endif()
