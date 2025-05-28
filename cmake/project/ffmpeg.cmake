
include(${ROOT_DIR}/cmake/find_thirdparty.cmake)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            z https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
            --url_hash sha256:9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
            --download_dir ${ROOT_DIR}/third_party
            --cmake_args "\"-DZLIB_BUILD_EXAMPLES=OFF\""
        WORKING_DIRECTORY ${ROOT_DIR}
        COMMAND_ECHO STDOUT
)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
set(compile_cmd "./configure --prefix=${INSTALL_DIR} --enable-static --host=aarch64-linux-gnu --cross-prefix=aarch64-linux-gnu- --disable-opencl --enable-pic --disable-asm")
else()
set(compile_cmd "./configure --prefix=${INSTALL_DIR} --enable-static")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            x264 https://github.com/mirror/x264/archive/refs/heads/master.zip
            --download_dir ${ROOT_DIR}/third_party
            --custom_compile
                "chmod a+x ./configure"
                "chmod a+x ./config.sub"
                "chmod a+x ./config.guess"
                "chmod a+x ./tools/cltostr.sh"
                "chmod a+x ./version.sh"
                "${compile_cmd}"
                "make -j10"
                "make install"
        WORKING_DIRECTORY ${ROOT_DIR}
        COMMAND_ECHO STDOUT
)

# https://github.com/ImageMagick/lzma.git

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
set(compile_cmd "./configure --prefix=${INSTALL_DIR} --disable-asm --disable-shared --enable-static --enable-libx264 --enable-gpl --cross-prefix=aarch64-linux-gnu- --enable-cross-compile --arch=aarch64  --target-os=linux --extra-ldflags=\"-L ../x264/install2/linux/lib\" --extra-cflags=\"-I ../x264/install2/linux/include\"")
else()
set(compile_cmd "./configure --prefix=${INSTALL_DIR} --disable-shared --enable-static --enable-libx264 --enable-gpl --extra-ldflags=\"-L ../x264/install2/linux/lib\" --extra-cflags=\"-I ../x264/install2/linux/include\"")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            ffmpeg https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n4.4.6.tar.gz
            --url_hash "sha256:9cdeaab9ddc0c0ddb63bad319adc977eba8ac946b15d5b4d9635222d8ba27a8b"
            --download_dir ${ROOT_DIR}/third_party
            --custom_compile
                "${compile_cmd}"
                "make -j10"
                "make install"
        WORKING_DIRECTORY ${ROOT_DIR}
        COMMAND_ECHO STDOUT
)

add_library(qlib_ffmpeg STATIC ${ROOT_DIR}/src/ffmpeg.cpp)
add_library(qlib::ffmpeg ALIAS qlib_ffmpeg)

target_include_directories(qlib_ffmpeg PUBLIC ${ROOT_DIR}/include ${ROOT_DIR}/third_party/ffmpeg/install2/linux/include)

target_link_directories(qlib_ffmpeg PUBLIC
    ${ROOT_DIR}/third_party/ffmpeg/install2/linux/lib
)
target_link_libraries(qlib_ffmpeg PUBLIC
    avformat avcodec avutil swscale swresample
    ${ROOT_DIR}/third_party/z/install2/linux/lib/libz.a
    ${ROOT_DIR}/third_party/x264/install2/linux/lib/libx264.a
)
