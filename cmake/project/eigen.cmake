
if (NOT DEFINED EIGEN_INCLUDE_CMAKE)

set(EIGEN_INCLUDE_CMAKE 1)

include(${ROOT_DIR}/cmake/compile.cmake)

compile(eigen
    "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz"
    "sha256:8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    " "
    ""
)

set(Eigen3_DIR ${ROOT_DIR}/third_party/eigen/install2/linux/share/eigen3/cmake)
find_package(Eigen3 REQUIRED)

add_library(qlib_eigen STATIC ${ROOT_DIR}/src/eigen.cpp)
add_library(qlib::eigen ALIAS qlib_eigen)

target_include_directories(qlib_eigen PUBLIC
    ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(qlib_eigen PUBLIC argparse::argparse)

endif()
