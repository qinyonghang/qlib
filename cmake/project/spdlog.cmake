set(LIB_NAME spdlog)
set(${LIB_NAME}_URL "https://github.com/gabime/spdlog/archive/refs/tags/v1.15.0.tar.gz")
set(${LIB_NAME}_URL_HASH "sha256:9962648c9b4f1a7bbc76fd8d9172555bad1871fdb14ff4f842ef87949682caa5")
set(${LIB_NAME}_DOWNLOAD_DIR ${ROOT_DIR}/third_party)
set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_DOWNLOAD_DIR}/${LIB_NAME})
set(${LIB_NAME}_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DSPDLOG_BUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/linux)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/linux)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/windows)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/windows)
set(${LIB_NAME}_CMAKE_ARGS "${${LIB_NAME}_CMAKE_ARGS} -A x64")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/win32)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/win32)
set(${LIB_NAME}_CMAKE_ARGS "${${LIB_NAME}_CMAKE_ARGS} -A Win32")
else()
message(FATAL_ERROR "Unsupported platform")
endif()

if(NOT EXISTS ${${LIB_NAME}_INSTALL_DIR})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            ${LIB_NAME} ${${LIB_NAME}_URL}
            --url_hash ${${LIB_NAME}_URL_HASH}
            --download_dir ${${LIB_NAME}_DOWNLOAD_DIR}
            --source_dir ${${LIB_NAME}_SOURCE_DIR}
            --build_dir ${${LIB_NAME}_BUILD_DIR}
            --install_dir ${${LIB_NAME}_INSTALL_DIR}
            --cmake_args ${${LIB_NAME}_CMAKE_ARGS}
        WORKING_DIRECTORY ${ROOT_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${LIB_NAME}!")
endif()

endif()

find_package(spdlog REQUIRED PATHS ${${LIB_NAME}_INSTALL_DIR} NO_DEFAULT_PATH)
target_link_libraries(qlib PUBLIC spdlog::spdlog)
