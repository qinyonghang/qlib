include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(psdk
    "https://github.com/dji-sdk/Payload-SDK/archive/refs/tags/3.11.1.tar.gz"
    "sha256:4631c136105f288d1c520f8af60f5ae4f7724ccb304842c17c1e97e6a4deb1a5"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    ""
)

add_library(psdk STATIC IMPORTED)
# Find library path.
set_target_properties(psdk
    PROPERTIES
    IMPORTED_LOCATION ${ROOT_DIR}/third_party/psdk/psdk_lib/lib/aarch64-linux-gnu-gcc/libpayloadsdk.a
    INTERFACE_INCLUDE_DIRECTORIES ${ROOT_DIR}/third_party/psdk/psdk_lib/include
)

target_compile_definitions(psdk INTERFACE
    -DPLATFORM_ARCH_AARCH64=1
    -DSYSTEM_ARCH_LINUX=1
)

target_link_libraries(qlib PUBLIC psdk)
