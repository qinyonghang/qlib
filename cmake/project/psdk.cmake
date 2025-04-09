set(LIB_NAME psdk)
set(${LIB_NAME}_URL "https://github.com/dji-sdk/Payload-SDK/archive/refs/tags/3.11.0.tar.gz")
# set(${LIB_NAME}_URL_HASH "sha256:9962648c9b4f1a7bbc76fd8d9172555bad1871fdb14ff4f842ef87949682caa5")
set(${LIB_NAME}_DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_DOWNLOAD_DIR}/${LIB_NAME})

if(NOT EXISTS ${${LIB_NAME}_SOURCE_DIR})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${PROJECT_SOURCE_DIR}
        ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/compile.py
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

add_library(psdk STATIC IMPORTED)
# Find library path.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
set_target_properties(psdk
    PROPERTIES
    IMPORTED_LOCATION ${${LIB_NAME}_SOURCE_DIR}/psdk_lib/lib/aarch64-linux-gnu-gcc/libpayloadsdk.a
    INTERFACE_INCLUDE_DIRECTORIES ${${LIB_NAME}_SOURCE_DIR}/psdk_lib/include
)
else()
message(FATAL_ERROR "Unsupported platform")
endif()

target_compile_definitions(psdk INTERFACE
    -DPLATFORM_ARCH_AARCH64=1
    -DSYSTEM_ARCH_LINUX=1
)

message(STATUS "Linked ${LIB_NAME} for ${LINKED_TARGET}. ROOT=${${LIB_NAME}_SOURCE_DIR}")
target_link_libraries(${LINKED_TARGET} PUBLIC psdk)

