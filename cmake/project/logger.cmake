if (NOT LOGGER_CMAKE_INCLUDED)

set(LOGGER_CMAKE_INCLUDED TRUE)

include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(spdlog
    "https://github.com/gabime/spdlog/archive/refs/tags/v1.15.0.tar.gz"
    "sha256:9962648c9b4f1a7bbc76fd8d9172555bad1871fdb14ff4f842ef87949682caa5"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPDLOG_BUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF"
)
# target_link_libraries(qlib PUBLIC spdlog::spdlog)

add_library(qlib_logger STATIC ${ROOT_DIR}/src/logger.cpp)
add_library(qlib::logger ALIAS qlib_logger)

target_include_directories(qlib_logger PUBLIC
    "$<BUILD_INTERFACE:${ROOT_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(qlib_logger PUBLIC spdlog::spdlog)

endif()
