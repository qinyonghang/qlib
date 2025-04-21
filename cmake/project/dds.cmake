get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)

set(download_dir ${PROJECT_SOURCE_DIR}/third_party)

set(memory_url https://github.com/foonathan/memory/archive/refs/tags/v0.7-3.tar.gz)
set(memory_source_dir ${download_dir}/memory)
set(memory_build_dir ${memory_source_dir}/build2/linux)
set(memory_install_dir ${memory_source_dir}/install2/linux)
set(memory_cmake_args "-DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF -DFOONATHAN_MEMORY_BUILD_TOOLS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON -Wno-dev")

set(fastcdr_url https://github.com/eProsima/Fast-CDR/archive/refs/tags/v2.3.0.tar.gz)
set(fastcdr_source_dir ${download_dir}/fastcdr)
set(fastcdr_build_dir ${fastcdr_source_dir}/build2/linux)
set(fastcdr_install_dir ${fastcdr_source_dir}/install2/linux)
set(fastcdr_cmake_args "-DBUILD_SHARED_LIBS=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON")

set(LIB_NAME dds)
set(${LIB_NAME}_URL "https://github.com/eProsima/Fast-DDS/archive/refs/tags/v3.2.1.tar.gz")
# set(${LIB_NAME}_URL_HASH "sha256:9962648c9b4f1a7bbc76fd8d9172555bad1871fdb14ff4f842ef87949682caa5")
set(${LIB_NAME}_SOURCE_DIR ${download_dir}/${LIB_NAME})
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/linux)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/linux)
set(${LIB_NAME}_CMAKE_ARGS "-DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON -DCOMPILE_TOOLS=ON -Dfastcdr_DIR=${fastcdr_install_dir}/lib/cmake/fastcdr -Dfoonathan_memory_DIR=${memory_install_dir}/lib/foonathan_memory/cmake")

if(NOT EXISTS ${${LIB_NAME}_INSTALL_DIR})

if(NOT EXISTS ${memory_install_dir})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            memory ${memory_url}
            --download_dir ${download_dir}
            --source_dir ${memory_source_dir}
            --build_dir ${memory_build_dir}
            --install_dir ${memory_install_dir}
            --cmake_args ${memory_cmake_args}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile memory!")
endif()

endif()

if(NOT EXISTS ${fastcdr_install_dir})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            fastcdr ${fastcdr_url}
            --download_dir ${download_dir}
            --source_dir ${fastcdr_source_dir}
            --build_dir ${fastcdr_build_dir}
            --install_dir ${fastcdr_install_dir}
            --cmake_args ${fastcdr_cmake_args}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile fastcdr!")
endif()

endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            ${LIB_NAME} ${${LIB_NAME}_URL}
            # --url_hash ${${LIB_NAME}_URL_HASH}
            --download_dir ${download_dir}
            --source_dir ${${LIB_NAME}_SOURCE_DIR}
            --build_dir ${${LIB_NAME}_BUILD_DIR}
            --install_dir ${${LIB_NAME}_INSTALL_DIR}
            --cmake_args ${${LIB_NAME}_CMAKE_ARGS}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${LIB_NAME}!")
endif()

endif()

set(foonathan_memory_DIR ${memory_install_dir}/lib/foonathan_memory/cmake)
set(fastcdr_DIR ${fastcdr_install_dir}/lib/cmake/fastcdr)
find_package(fastcdr REQUIRED PATHS ${fastcdr_install_dir} NO_DEFAULT_PATH)
find_package(foonathan_memory REQUIRED PATHS ${memory_install_dir} NO_DEFAULT_PATH)
find_package(fastdds 3 REQUIRED PATHS ${${LIB_NAME}_INSTALL_DIR} NO_DEFAULT_PATH)

add_library(dds INTERFACE)
target_link_directories(dds INTERFACE ${fastcdr_install_dir}/lib ${memory_install_dir}/lib)
target_link_libraries(dds INTERFACE fastcdr foonathan_memory fastdds)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--disable-new-dtags")
