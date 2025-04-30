include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(spdlog
    "https://github.com/gabime/spdlog/archive/refs/tags/v1.15.0.tar.gz"
    "sha256:9962648c9b4f1a7bbc76fd8d9172555bad1871fdb14ff4f842ef87949682caa5"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DYAML_BUILD_SHARED_LIBS=OFF -DSPDLOG_BUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF"
)
target_link_libraries(qlib PUBLIC spdlog::spdlog)
