
option(BUILD_TEST "Build tests" ON)
if (BUILD_TEST)
    if (NOT TARGET qlib::qlib)
        cmake_minimum_required(VERSION 3.22)
        project(tests)
        set(out_dir ${CMAKE_CURRENT_BINARY_DIR})
        find_package(qlib REQUIRED PATHS ${CMAKE_CURRENT_LIST_DIR} NO_DEFAULT_PATH)
    else ()
        set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/tests)
    endif ()
    enable_testing()
    file(GLOB tests ${PROJECT_SOURCE_DIR}/tests/*.cpp)
    foreach(test IN LISTS tests)
        get_filename_component(test_name ${test} NAME_WE)
        set(test_name ${test_name} CACHE INTERNAL "test=${test_name}")
        message(STATUS "Build Target: ${test_name}")
        add_executable(${test_name} ${test})
        set_target_properties(${test_name} PROPERTIES
            C_STANDARD 17
            C_STANDARD_REQUIRED ON
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
        )
        target_link_libraries(${test_name} PRIVATE qlib::qlib)
        if(MSVC)
            target_compile_options(${test_name} PRIVATE /utf-8)
        endif()
    endforeach()
endif ()
