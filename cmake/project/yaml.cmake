include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(yaml-cpp
    "https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz"
    "sha256:fbe74bbdcee21d656715688706da3c8becfd946d92cd44705cc6098bb23b3a16"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DYAML_BUILD_SHARED_LIBS=OFF"
)

add_library(qlib_yaml STATIC ${ROOT_DIR}/src/yaml.cpp)
add_library(qlib::yaml ALIAS qlib_yaml)

target_include_directories(qlib_yaml PUBLIC
    "$<BUILD_INTERFACE:${ROOT_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(qlib_yaml PUBLIC yaml-cpp::yaml-cpp qlib::logger)
