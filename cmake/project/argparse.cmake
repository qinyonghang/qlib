set(LIB_NAME argparse)
set(${LIB_NAME}_URL "https://github.com/p-ranav/argparse/archive/refs/tags/v3.1.tar.gz")
set(${LIB_NAME}_URL_HASH "sha256:d01733552ca4a18ab501ae8b8be878131baa32e89090fafdeef018ebfa4c6e46")
set(${LIB_NAME}_DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_DOWNLOAD_DIR}/${LIB_NAME})

if(NOT EXISTS ${${LIB_NAME}_SOURCE_DIR})

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            ${LIB_NAME} ${${LIB_NAME}_URL}
            --url_hash ${${LIB_NAME}_URL_HASH}
            --download_dir ${${LIB_NAME}_DOWNLOAD_DIR}
            --source_dir ${${LIB_NAME}_SOURCE_DIR}
            --skip_compile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to download ${LIB_NAME}!")
endif()

endif()

message(STATUS "Linked ${LIB_NAME} for ${LINKED_TARGET}. ROOT=${${LIB_NAME}_SOURCE_DIR}")
target_include_directories(${LINKED_TARGET} PUBLIC ${${LIB_NAME}_SOURCE_DIR}/include)
