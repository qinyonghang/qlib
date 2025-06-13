
option(BUILD_TEST "Build tests" ON)
if (BUILD_TEST)
    enable_testing()

    include(${ROOT_DIR}/cmake/defines.cmake)
    include(${ROOT_DIR}/cmake/python3.cmake)
    include(${ROOT_DIR}/cmake/project/argparse.cmake)
    include(${ROOT_DIR}/cmake/project/logger.cmake)
    include(${ROOT_DIR}/cmake/project/dds.cmake)
    include(${ROOT_DIR}/cmake/project/ffmpeg.cmake)

    set(out_dir ${CMAKE_CURRENT_BINARY_DIR})
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
        target_link_libraries(${test_name} PRIVATE qlib::dds qlib::logger qlib::argparse qlib::ffmpeg)
        if(MSVC)
            target_compile_options(${test_name} PRIVATE /utf-8)
        endif()
    endforeach()
endif ()
