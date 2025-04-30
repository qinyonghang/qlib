include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(nlohmann_json
    "https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz"
    "sha256:4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DJSON_BuildTests=OFF"
)

target_link_libraries(qlib PUBLIC nlohmann_json::nlohmann_json)
