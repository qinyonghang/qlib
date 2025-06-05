cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED PROJECT_NAME)
project(qlib C CXX)
endif()

# message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}.")

# add_library(qlib STATIC src/qlib.cpp)
# add_library(qlib::qlib ALIAS qlib)

# set_target_properties(qlib PROPERTIES
#     C_STANDARD 17
#     C_STANDARD_REQUIRED ON
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
# )

# if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
# target_compile_options(qlib PRIVATE
#     $<$<CONFIG:Release>:-O3>
#     $<$<CONFIG:Debug>:-g -O0>
#     # $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wl,-unresolved-symbols=ignore-in-shared-libs>
#     -Wall
#     # -Wno-deprecated-declarations -Wno-int-in-bool-context -Wno-unused-function
# )
# endif ()

# target_include_directories(qlib PUBLIC
#     "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
#     "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
# )

# if (WITH_BENCHMARK STREQUAL true)
#     file(GLOB dhrystone2_files ${CMAKE_CURRENT_SOURCE_DIR}/src/dhrystone2/*.c)
#     add_library(dhrystone2 STATIC ${dhrystone2_files})

#     target_compile_options(dhrystone2 PRIVATE
#         $<$<OR:$<COMPILE_LANG_AND_ID:CXX,GNU>,$<COMPILE_LANG_AND_ID:CXX,Clang>>:-Wno-unused-variable -O3>
#     )

#     target_link_libraries(dhrystone2 PRIVATE -lc -lm)
#     target_link_libraries(qlib PRIVATE dhrystone2)
# endif ()

# set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "Root directory")
# include(${CMAKE_CURRENT_LIST_DIR}/cmake/python3.cmake)
# file(GLOB cmake_source_files ${CMAKE_CURRENT_LIST_DIR}/cmake/project/*.cmake)
# foreach(cmake_file IN LISTS cmake_source_files)
#     message(STATUS "Including: ${cmake_file}")
#     include(${cmake_file})
# endforeach()

option(BUILD_PYTHON_MODULE "Build Python Module!" OFF)

if (BUILD_PYTHON_MODULE)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "Root directory")
include(${ROOT_DIR}/cmake/defines.cmake)
include(${ROOT_DIR}/cmake/python3.cmake)
include(${ROOT_DIR}/cmake/project/boost.cmake)
include(${ROOT_DIR}/cmake/project/logger.cmake)
include(${ROOT_DIR}/cmake/project/dds.cmake)

add_library(qlib_python MODULE src/dds_python.cpp)

set_target_properties(qlib_python PROPERTIES PREFIX "")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
set_target_properties(qlib_python PROPERTIES SUFFIX ".pyd")
endif()

set_target_properties(qlib_python PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
target_compile_options(qlib_python PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g -O0>
    -Wall
)
endif ()

target_include_directories(qlib_python PRIVATE ${Python3_INCLUDE_DIRS})
target_link_libraries(qlib_python PRIVATE qlib::boost qlib::dds qlib::logger)

endif()

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "Root directory")
include(${ROOT_DIR}/cmake/tests.cmake)
