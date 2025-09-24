cmake_minimum_required(VERSION 3.18)

project(cuda LANGUAGES CXX CUDA)

# find_package(CUDA REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(${PROJECT_NAME} main.cpp kernel.cu)

set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g -O0>
    -Wall
)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE benchmark)
