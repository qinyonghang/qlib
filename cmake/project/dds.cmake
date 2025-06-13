if (NOT DEFINED DDS_INCLUDE_CMAKE)

set(DDS_INCLUDE_CMAKE 1)

include(${ROOT_DIR}/cmake/find_thirdparty.cmake)

find_thirdparty(foonathan_memory
    "https://github.com/foonathan/memory/archive/refs/tags/v0.7-3.tar.gz"
    "sha256:4203d15db22a94a3978eeb1afb59a37d35c57c0f148733f0f1a53a6281cb74dd"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF -DFOONATHAN_MEMORY_BUILD_TOOLS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON -Wno-dev"
)

find_thirdparty(fastcdr
    "https://github.com/eProsima/Fast-CDR/archive/refs/tags/v2.3.0.tar.gz"
    "sha256:d85ee9e24e105581b49de3e2b6b2585335a8dc98c4cabd288232fffc4b9b6a24"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DBUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON"
)

find_thirdparty(asio
    "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-18-1.tar.gz"
    "sha256:39c721b987b7a0d2fe2aee64310bd128cd8cc10f43481604d18cb2d8b342fd40"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    ""
)

find_thirdparty(tinyxml2
    "https://github.com/leethomason/tinyxml2/archive/refs/tags/11.0.0.tar.gz"
    "sha256:5556deb5081fb246ee92afae73efd943c889cef0cafea92b0b82422d6a18f289"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DBUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON"
)

find_thirdparty(fastdds
    "https://github.com/eProsima/Fast-DDS/archive/refs/tags/v3.2.1.tar.gz"
    "sha256:8c088fb967d39785f1f83454d75308ae6c55fc74fc31de173713bc293635b2fd"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON -DCOMPILE_TOOLS=ON -Dfastcdr_DIR=${ROOT_DIR}/third_party/fastcdr/install2/linux/lib/cmake/fastcdr -Dfoonathan_memory_DIR=${ROOT_DIR}/third_party/foonathan_memory/install2/linux/lib/foonathan_memory/cmake -DAsio_INCLUDE_DIR=${ROOT_DIR}/third_party/asio/asio/include -DTinyXML2_DIR=${ROOT_DIR}/third_party/tinyxml2/install2/linux/lib/cmake/tinyxml2"
)

# target_link_libraries(qlib PUBLIC fastcdr foonathan_memory fastdds)

install(FILES ${ROOT_DIR}/third_party/tinyxml2/install2/linux/lib/libtinyxml2.so.11.0.0 DESTINATION lib RENAME libtinyxml2.so.11)
install(FILES ${ROOT_DIR}/third_party/foonathan_memory/install2/linux/lib/libfoonathan_memory-0.7.3.so DESTINATION lib)
install(FILES ${ROOT_DIR}/third_party/fastcdr/install2/linux/lib/libfastcdr.so.2.3.0 DESTINATION lib RENAME libfastcdr.so.2)
install(FILES ${ROOT_DIR}/third_party/fastdds/install2/linux/lib/libfastdds.so.3.2.1 DESTINATION lib RENAME libfastdds.so.3.2)

add_library(qlib_dds STATIC ${ROOT_DIR}/src/dds.cpp)
add_library(qlib::dds ALIAS qlib_dds)

target_compile_options(qlib_dds PRIVATE -fPIC)

target_include_directories(qlib_dds PUBLIC
    "$<BUILD_INTERFACE:${ROOT_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(qlib_dds PRIVATE fastcdr foonathan_memory fastdds)

include(${ROOT_DIR}/cmake/project/logger.cmake)

target_link_libraries(qlib_dds PUBLIC qlib::logger)

endif()
