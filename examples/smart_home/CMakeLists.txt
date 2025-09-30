
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message(STATUS "Build ${TARGET_NAME}:")
add_executable(${TARGET_NAME} main.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_link_libraries(${TARGET_NAME} PRIVATE qlib::qlib -lasound)

target_compile_definitions(${TARGET_NAME} PRIVATE RESOUCES_PATH="${CMAKE_SOURCE_DIR}/resources")

if (ENABLE_ASAN)
target_compile_options(${TARGET_NAME} PRIVATE
    -fsanitize=address -fno-omit-frame-pointer
)

target_link_options(${TARGET_NAME} PRIVATE
    -fsanitize=address
)
endif()

target_compile_options(${TARGET_NAME} PRIVATE -Wall -march=native $<$<CONFIG:Release>:-O3> $<$<CONFIG:Debug>:-O0 -g>)

if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /utf-8)
endif()

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(spdlog)
target_link_libraries(${TARGET_NAME} PRIVATE spdlog::spdlog)

# FetchContent_Declare(
#     yaml-cpp
#     GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
#     GIT_TAG 0.8.0
#     GIT_SHALLOW ON
# )

# FetchContent_MakeAvailable(yaml-cpp)
# target_link_libraries(${TARGET_NAME} PRIVATE yaml-cpp)

FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG v19.7.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(portaudio)
target_link_libraries(${TARGET_NAME} PRIVATE portaudio)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/include)
target_link_libraries(${TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libsherpa-onnx-cxx-api.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libsherpa-onnx-c-api.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libsherpa-onnx-core.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libsherpa-onnx-kaldifst-core.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libsherpa-onnx-fst.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libsherpa-onnx-fstfar.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libkaldi-native-fbank-core.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libkaldi-decoder-core.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libonnxruntime.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libkissfft-float.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libssentencepiece_core.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libpiper_phonemize.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libespeak-ng.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libucd.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libcppinyin_core.a
    ${CMAKE_SOURCE_DIR}/sherpa-onnx/install/lib/libcargs.a
)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/kws)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/kws
    ${CMAKE_BINARY_DIR}/kws
    COMMENT "Copying kws directory to ${CMAKE_BINARY_DIR}"
    VERBATIM
)
endif()


if (NOT EXISTS ${CMAKE_BINARY_DIR}/wenetspeech)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/wenetspeech
    ${CMAKE_BINARY_DIR}/wenetspeech
    COMMENT "Copying wenetspeech directory to ${CMAKE_BINARY_DIR}"
    VERBATIM
)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/silero_vad.onnx")
  add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/silero_vad.onnx"
      "${CMAKE_BINARY_DIR}/silero_vad.onnx"
    COMMENT "Copying silero_vad.onnx to build directory"
    VERBATIM
  )
else()
  message(WARNING "silero_vad.onnx not found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rules.json")
  add_custom_command(
    TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/rules.json"
      "${CMAKE_BINARY_DIR}/rules.json"
    COMMENT "Copying rules.json to build directory"
    VERBATIM
  )
else()
  message(WARNING "rules.json not found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${TARGET_NAME})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.json DESTINATION ${CMAKE_INSTALL_PREFIX}/${TARGET_NAME})
