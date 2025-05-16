
include(${ROOT_DIR}/cmake/find_thirdparty.cmake)

find_thirdparty(psdk
    "https://github.com/dji-sdk/Payload-SDK/archive/refs/tags/3.11.1.tar.gz"
    "sha256:4631c136105f288d1c520f8af60f5ae4f7724ccb304842c17c1e97e6a4deb1a5"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    ""
)

find_thirdparty(libusb
    "https://github.com/libusb/libusb/archive/refs/tags/v1.0.0.tar.gz"
    "sha256:9c3bfe7472ad618c1c09ab8b5602150efe37b3fe8baf15391742cb55635aec46"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    ""
)

add_library(psdk STATIC ${ROOT_DIR}/src/psdk.cpp)
add_library(qlib::psdk ALIAS psdk)

target_compile_definitions(psdk PUBLIC
    -DPLATFORM_ARCH_AARCH64=1
    -DSYSTEM_ARCH_LINUX=1
)

target_include_directories(psdk PUBLIC
    "$<BUILD_INTERFACE:${ROOT_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    ${ROOT_DIR}/third_party/psdk/psdk_lib/include
    ${ROOT_DIR}/third_party/libusb/out_aarch64/include
)

target_link_libraries(psdk PUBLIC
    ${ROOT_DIR}/third_party/psdk/psdk_lib/lib/aarch64-linux-gnu-gcc/libpayloadsdk.a
    ${ROOT_DIR}/third_party/libusb/out_aarch64/lib/libusb-1.0.a
    spdlog::spdlog
)
