
set(LIB_NAME yaml-cpp)
set(${LIB_NAME}_URL "https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz")
# set(${LIB_NAME}_URL_HASH "sha256:81452dba004a7cac5ca9cc43f1d92542fcb5d7c1b6d554f1e913f36e580099d8")
set(${LIB_NAME}_DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(${LIB_NAME}_SOURCE_DIR ${${LIB_NAME}_DOWNLOAD_DIR}/${LIB_NAME})
set(${LIB_NAME}_CMAKE_ARGS "\"-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -DYAML_BUILD_SHARED_LIBS=OFF\"")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/linux)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/linux)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/windows)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/windows)
set(${LIB_NAME}_CMAKE_ARGS "-A x64 ${${LIB_NAME}_CMAKE_ARGS}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
set(${LIB_NAME}_BUILD_DIR ${${LIB_NAME}_SOURCE_DIR}/build2/win32)
set(${LIB_NAME}_INSTALL_DIR ${${LIB_NAME}_SOURCE_DIR}/install2/win32)
set(${LIB_NAME}_CMAKE_ARGS "-A Win32 ${${LIB_NAME}_CMAKE_ARGS}")
else()
message(FATAL_ERROR "Unsupported platform")
endif()

if (NOT EXISTS ${${LIB_NAME}_INSTALL_DIR})

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

find_package(yaml-cpp REQUIRED PATHS ${${LIB_NAME}_INSTALL_DIR} NO_DEFAULT_PATH)

# message(STATUS "Linked ${LIB_NAME} for ${LINKED_TARGET}. ROOT=${${LIB_NAME}_INSTALL_DIR}")
# target_link_libraries(${LINKED_TARGET} PUBLIC yaml-cpp::yaml-cpp)
