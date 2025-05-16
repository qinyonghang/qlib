
include(${ROOT_DIR}/cmake/find_thirdparty.cmake)
find_thirdparty(live555
    "https://github.com/rgaufman/live555/archive/refs/heads/master.zip"
    ""
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    ""
)
