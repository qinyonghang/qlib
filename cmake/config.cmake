
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(WITH_NEON true)
else()
    set(WITH_NEON false)
endif()

set(WITH_ONNXRUNTIME false)
set(WITH_OPENCV false)
set(WITH_BENCHMARK false)
set(DUMP_TENSORS false)

option(CMAKE_POSITION_INDEPENDENT_CODE "Build as a PIC library" ON)
