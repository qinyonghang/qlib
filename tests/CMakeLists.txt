
file(GLOB TARGET_NAMES ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(TARGET_NAME ${TARGET_NAMES})

  if (IS_DIRECTORY ${TARGET_NAME})
    get_filename_component(TARGET_NAME ${TARGET_NAME} NAME)

    message(STATUS "Build ${TARGET_NAME}:")
    add_executable(${TARGET_NAME} ${TARGET_NAME}/main.cpp)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

    if (NOT ${TARGET_NAME} MATCHES ".*cpp14")
      set_target_properties(${TARGET_NAME} PROPERTIES
          C_STANDARD 17
          C_STANDARD_REQUIRED ON
          CXX_STANDARD 17
          CXX_STANDARD_REQUIRED ON
      )
    else()
      set_target_properties(${TARGET_NAME} PROPERTIES
          C_STANDARD 11
          C_STANDARD_REQUIRED ON
          CXX_STANDARD 14
          CXX_STANDARD_REQUIRED ON
      )
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    target_link_libraries(${TARGET_NAME} PRIVATE qlib::qlib -lgtest)
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -march=native $<$<CONFIG:Release>:-O3> $<$<CONFIG:Debug>:-O0 -g>)

    if (ENABLE_ASAN)
      target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
      target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    endif()

    if(MSVC)
      target_compile_options(${TARGET_NAME} PRIVATE /utf-8)
    endif()
  endif ()

endforeach()
