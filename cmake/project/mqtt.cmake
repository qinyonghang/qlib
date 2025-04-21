set(download_dir ${PROJECT_SOURCE_DIR}/third_party)

set(mqtt_cpp_url "https://github.com/eclipse-paho/paho.mqtt.cpp/archive/refs/tags/v1.5.2.tar.gz")
set(mqtt_cpp_source_dir ${download_dir}/mqtt_cpp)
set(mqtt_cpp_cmake_args "-DPAHO_BUILD_STATIC=TRUE -DPAHO_WITH_SSL=OFF -DPAHO_WITH_MQTT_C=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
set(mqtt_cpp_build_dir ${mqtt_cpp_source_dir}/build2/linux)
set(mqtt_cpp_install_dir ${mqtt_cpp_source_dir}/install2/linux)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
set(mqtt_cpp_build_dir ${mqtt_cpp_source_dir}/build2/windows)
set(mqtt_cpp_install_dir ${mqtt_cpp_source_dir}/install2/windows)
set(mqtt_cpp_cmake_args "${mqtt_cpp_cmake_args} -A x64")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
set(mqtt_cpp_build_dir ${mqtt_cpp_source_dir}/build2/win32)
set(mqtt_cpp_install_dir ${mqtt_cpp_source_dir}/install2/win32)
set(mqtt_cpp_cmake_args "${mqtt_cpp_cmake_args} -A Win32")
else()
message(FATAL_ERROR "Unsupported platform")
endif()

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)

if(NOT EXISTS ${mqtt_cpp_install_dir})

if(NOT EXISTS ${mqtt_c_source_dir})

set(mqtt_c_url "https://github.com/eclipse-paho/paho.mqtt.c/archive/refs/tags/v1.3.14.tar.gz")
set(mqtt_c_source_dir ${download_dir}/paho-mqtt-c)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            paho-mqtt-c ${mqtt_c_url}
            # --url_hash ${${LIB_NAME}_URL_HASH}
            --download_dir ${download_dir}
            --source_dir ${mqtt_c_source_dir}
            --skip_compile
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)
if(result)
message(FATAL_ERROR "Failed to download mqtt_c!")
endif()
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${ROOT_DIR}
        ${Python3_EXECUTABLE} ${ROOT_DIR}/scripts/compile.py
            mqtt_cpp ${mqtt_cpp_url}
            # --url_hash ${${LIB_NAME}_URL_HASH}
            --download_dir ${download_dir}
            --source_dir ${mqtt_cpp_source_dir}
            --build_dir ${mqtt_cpp_build_dir}
            --install_dir ${mqtt_cpp_install_dir}
            --cmake_args ${mqtt_cpp_cmake_args}
            --before_compile "rm -r ${mqtt_cpp_source_dir}/externals/paho-mqtt-c" "ln -s ${mqtt_c_source_dir} ${mqtt_cpp_source_dir}/externals/"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${LIB_NAME}!")
endif()

endif()

find_package(PahoMqttCpp REQUIRED PATHS ${mqtt_cpp_install_dir} NO_DEFAULT_PATH)

# message(STATUS "Linked mqtt for ${LINKED_TARGET}. ROOT=${mqtt_cpp_install_dir}")
# target_link_libraries(${LINKED_TARGET} PUBLIC PahoMqttCpp::paho-mqttpp3)
