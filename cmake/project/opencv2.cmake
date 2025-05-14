
include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(OpenCV
    "https://github.com/opencv/opencv/archive/refs/tags/4.9.0.tar.gz"
    "sha256:ddf76f9dffd322c7c3cb1f721d0887f62d747b82059342213138dc190f28bc6c"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_opencv_apps=OFF -DBUILD_opencv_world=ON -Wno-dev"
)

set(OpenCV_DIR ${ROOT_DIR}/third_party/OpenCV/install2/linux/lib/cmake/opencv4)
find_package(OpenCV REQUIRED COMPONENTS opencv_world)
# message(STATUS "OpenCV_LIBRARIES=${OpenCV_LIBRARIES}")
target_link_libraries(qlib PUBLIC ${OpenCV_LIBRARIES})

# install(FILES ${ROOT_DIR}/third_party/OpenCV/install2/linux/lib/libopencv_world.so.4.9.0 DESTINATION lib RENAME libopencv_world.so.409)
