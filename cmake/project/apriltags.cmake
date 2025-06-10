if (NOT DEFINED APRILTAGS_INCLUDE_CMAKE)

set(APRILTAGS_INCLUDE_CMAKE 1)

include(${ROOT_DIR}/cmake/compile.cmake)
include(${ROOT_DIR}/cmake/project/opencv2.cmake)
include(${ROOT_DIR}/cmake/project/eigen.cmake)

compile(apriltags
    "https://github.com/robodhruv/apriltags-cpp/archive/refs/heads/master.zip"
    "sha256:a6aea70dfc81d3a1dfb9852fb1ff7ef61abfd4306b3457f2a7ed73b8aee9abf6"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    ""
    ""
)

# -DOpenCV_DIR=${ROOT_DIR}/third_party/OpenCV/install2/linux/lib/cmake/opencv4 -DEigen3_DIR=${ROOT_DIR}/third_party/eigen/install2/linux/share/eigen3/cmake -DAPRILTAGS_BUILD_TESTS=OFF

file(GLOB source_files ${ROOT_DIR}/third_party/apriltags/src/*.cc)
file(GLOB header_files ${ROOT_DIR}/third_party/apriltags/AprilTags/*.h)
add_library(qlib_apriltags STATIC ${source_files})
add_library(qlib::apriltags ALIAS qlib_apriltags)

set_target_properties(qlib_apriltags PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(qlib_apriltags PUBLIC
    ${ROOT_DIR}/third_party/apriltags
)

target_include_directories(qlib_apriltags PRIVATE
    ${ROOT_DIR}/third_party/apriltags/AprilTags
)

target_link_libraries(qlib_apriltags PUBLIC qlib::opencv qlib::eigen)

endif()
