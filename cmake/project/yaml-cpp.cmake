include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(yaml-cpp
    "https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz"
    "sha256:fbe74bbdcee21d656715688706da3c8becfd946d92cd44705cc6098bb23b3a16"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DYAML_BUILD_SHARED_LIBS=OFF"
)
target_link_libraries(qlib PUBLIC yaml-cpp::yaml-cpp)
