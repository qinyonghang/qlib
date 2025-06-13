
if (NOT DEFINED ARGPARSE_INCLUDE_CMAKE)

set(ARGPARSE_INCLUDE_CMAKE 1)

include(${ROOT_DIR}/cmake/project/logger.cmake)

# find_thirdparty(argparse
#     "https://github.com/p-ranav/argparse/archive/refs/tags/v3.1.tar.gz"
#     "sha256:d01733552ca4a18ab501ae8b8be878131baa32e89090fafdeef018ebfa4c6e46"
#     ${ROOT_DIR}
#     ${ROOT_DIR}/third_party
#     "-DARGPARSE_BUILD_TESTS=OFF -DARGPARSE_BUILD_SAMPLES=OFF"
# )

add_library(qlib_argparse STATIC ${ROOT_DIR}/src/argparse.cpp)
add_library(qlib::argparse ALIAS qlib_argparse)

target_include_directories(qlib_argparse PUBLIC
    ${ROOT_DIR}/include
)

# target_link_libraries(qlib_argparse PRIVATE argparse::argparse)
target_link_libraries(qlib_argparse PRIVATE qlib::logger)

endif()
