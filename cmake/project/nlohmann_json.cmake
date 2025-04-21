
set(LIB_NAME json)
set(${LIB_NAME}_URL "https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz")
# set(${LIB_NAME}_URL_HASH "sha256:81452dba004a7cac5ca9cc43f1d92542fcb5d7c1b6d554f1e913f36e580099d8")
set(${LIB_NAME}_DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_DOWNLOAD_DIR}/${LIB_NAME})

if (NOT EXISTS ${${LIB_NAME}_SOURCE_DIR})

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            ${LIB_NAME} ${${LIB_NAME}_URL}
            # --url_hash ${${LIB_NAME}_URL_HASH}
            --download_dir ${${LIB_NAME}_DOWNLOAD_DIR}
            --source_dir ${${LIB_NAME}_SOURCE_DIR}
            --skip_compile
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${LIB_NAME}!")
endif()

endif()

add_library(json INTERFACE)
target_include_directories(json INTERFACE ${${LIB_NAME}_SOURCE_DIR}/include)

# message(STATUS "Linked ${LIB_NAME} for ${LINKED_TARGET}. ROOT=${${LIB_NAME}_SOURCE_DIR}")
# target_link_libraries(${LINKED_TARGET} PUBLIC json)
