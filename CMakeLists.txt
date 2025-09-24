cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED PROJECT_NAME)
project(qlib C CXX)
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}.")

add_library(qlib INTERFACE)
add_library(qlib::qlib ALIAS qlib)

set_target_properties(qlib PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES 
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>;$<INSTALL_INTERFACE:include>"
)

option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_ASAN "Enable address sanitizer" OFF)

if (BUILD_EXAMPLES)
file(GLOB examples ${CMAKE_CURRENT_LIST_DIR}/examples/*)
foreach(example ${examples})
    if (IS_DIRECTORY ${example} AND EXISTS ${example}/CMakeLists.txt)
        add_subdirectory(${example})
    endif ()
endforeach()
endif()

if (BUILD_TESTS)
enable_testing()
add_subdirectory(tests)
endif()

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
install(TARGETS qlib EXPORT qlib INCLUDES DESTINATION include)
install(EXPORT qlib FILE qlib-targets.cmake DESTINATION lib/cmake/qlib)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qlib-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/qlib-config.cmake"
    INSTALL_DESTINATION lib/cmake/qlib
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qlib-config.cmake" DESTINATION lib/cmake/qlib)
