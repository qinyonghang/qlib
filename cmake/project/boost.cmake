
if (NOT DEFINED BOOST_INCLUDE_CMAKE)

set(BOOST_INCLUDE_CMAKE 1)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
set(arch "64")
else()
set(arch "32")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
set(b2 "./b2")
set(bootstrap "sh bootstrap.sh")
if (arch STREQUAL "64")
set(prefix "${ROOT_DIR}/third_party/boost/install2/linux")
else()
set(prefix "${ROOT_DIR}/third_party/boost/install2/linux32")
endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
set(b2 "b2.exe")
set(bootstrap "bootstrap.bat")
if (arch STREQUAL "64")
set(prefix "${ROOT_DIR}/third_party/boost/install2/windows")
else()
set(prefix "${ROOT_DIR}/third_party/boost/install2/win32")
endif()
endif()

if (NOT EXISTS ${prefix})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            boost https://archives.boost.io/release/1.80.0/source/boost_1_80_0.tar.gz
            --url_hash sha256:4b2136f98bdd1f5857f1c3dea9ac2018effe65286cf251534b6ae20cc45e1847
            --download_dir ${ROOT_DIR}/third_party
            --system ${CMAKE_SYSTEM_NAME}
            --arch ${arch}
            --custom_compile "${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/replace.py ${ROOT_DIR}/third_party/boost/libs/python/src/numpy/dtype.cpp \"reinterpret_cast<PyArray_Descr*>(ptr())->elsize\" \"0\"" "${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/replace.py ${ROOT_DIR}/third_party/boost/tools/build/src/tools/msvc.jam \"(14.3)\" \"(14.[34])\"" "${bootstrap}" "${b2} install address-model=${arch} --with-python --link=static runtime-link=static cxxflags=-fPIC linkflags=-fPIC --prefix=${prefix}"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${LIB_NAME}!")
endif()

endif()

set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED COMPONENTS python PATHS ${prefix} NO_DEFAULT_PATH)

add_library(boost STATIC ${ROOT_DIR}/src/boost.cpp)
add_library(qlib::boost ALIAS boost)

target_compile_definitions(boost PUBLIC BOOST_PYTHON_STATIC_LIB)
target_link_libraries(boost PUBLIC ${Boost_LIBRARIES})

endif()
