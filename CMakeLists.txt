cmake_minimum_required(VERSION 3.22)

project(qlib)
set(LIB_NAME qlib)

option(CMAKE_POSITION_INDEPENDENT_CODE "Build as a PIC library" ON)

if (NOT DEFINED CONFIG_PATH)
message(STATUS "Using default config.")
include(cmake/config.cmake)
else ()
message(STATUS "Using custom config: ${CONFIG_PATH}")
include(${CONFIG_PATH})
endif()

if (WITH_ONNXRUNTIME STREQUAL true)
    set(DFLAGS "${DFLAGS}" "WITH_ONNXRUNTIME")
endif()

if (WITH_NEON STREQUAL true)
    set(DFLAGS "${DFLAGS}" "WITH_NEON")
endif()

if (WITH_OPENCV STREQUAL true)
    set(DFLAGS "${DFLAGS}" "WITH_OPENCV")
endif()

if (WITH_BENCHMARK STREQUAL true)
    set(DFLAGS "${DFLAGS}" "WITH_BENCHMARK")
endif()

if (DUMP_TENSORS STREQUAL true)
    set(DFLAGS "${DFLAGS}" "DUMP_TENSORS")
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}.")
message(STATUS "DFLAGS: ${DFLAGS}")

file(GLOB source_files ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_library(${LIB_NAME} STATIC ${source_files})
add_library(qlib::qlib ALIAS ${LIB_NAME})
set_target_properties(${LIB_NAME} PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_compile_definitions(${LIB_NAME} PUBLIC ${DFLAGS})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
target_compile_options(${LIB_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g -O0>
    -Wall
    -Wno-deprecated-declarations -Wno-int-in-bool-context -Wno-unused-function
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wl,-unresolved-symbols=ignore-in-shared-libs>
    -fPIC
)
endif ()

target_include_directories(${LIB_NAME} PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if (WITH_BENCHMARK STREQUAL true)
    file(GLOB dhrystone2_files ${CMAKE_CURRENT_SOURCE_DIR}/src/dhrystone2/*.c)
    add_library(dhrystone2 STATIC ${dhrystone2_files})

    target_compile_options(dhrystone2 PRIVATE
        $<$<OR:$<COMPILE_LANG_AND_ID:CXX,GNU>,$<COMPILE_LANG_AND_ID:CXX,Clang>>:-Wno-unused-variable -O3>
    )

    target_link_libraries(dhrystone2 PRIVATE -lc -lm)
    target_link_libraries(${LIB_NAME} PRIVATE dhrystone2)
endif ()

include(${PROJECT_SOURCE_DIR}/cmake/python3.cmake)

set(LINKED_TARGET ${LIB_NAME} CACHE STRING "Linked target" FORCE)
file(GLOB cmake_source_files ${PROJECT_SOURCE_DIR}/cmake/project/*.cmake)
foreach(cmake_file IN LISTS cmake_source_files)
    if (NOT ${cmake_file} STREQUAL ${PROJECT_SOURCE_DIR}/cmake/python3.cmake)
        message(STATUS "Including: ${cmake_file}")
        include(${cmake_file})
    endif()
endforeach()

include(${PROJECT_SOURCE_DIR}/cmake/install.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/tests.cmake)
