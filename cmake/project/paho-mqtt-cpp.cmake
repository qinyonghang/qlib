include(${ROOT_DIR}/cmake/find_thirdparty.cmake)

find_thirdparty(eclipse-paho-mqtt-c
    "https://github.com/eclipse-paho/paho.mqtt.c/archive/refs/tags/v1.3.14.tar.gz"
    "sha256:7af7d906e60a696a80f1b7c2bd7d6eb164aaad908ff4c40c3332ac2006d07346"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DPAHO_BUILD_SHARED=OFF -DPAHO_BUILD_STATIC=ON -DPAHO_ENABLE_TESTING=OFF"
)

find_thirdparty(PahoMqttCpp
    "https://github.com/eclipse-paho/paho.mqtt.cpp/archive/refs/tags/v1.5.2.tar.gz"
    "sha256:3d8d9bfee614d74fa2e28dc244733c79e4868fa32a2d49af303ec176ccc55bfb"
    ${ROOT_DIR}
    ${ROOT_DIR}/third_party
    "-DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=OFF -DPAHO_WITH_SSL=OFF -DPAHO_WITH_MQTT_C=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -Declipse-paho-mqtt-c_DIR=${ROOT_DIR}/third_party/eclipse-paho-mqtt-c/install2/linux/lib/cmake/eclipse-paho-mqtt-c"
)

add_library(qlib_mqtt STATIC ${ROOT_DIR}/src/mqtt.cpp)
add_library(qlib::mqtt ALIAS qlib_mqtt)

target_include_directories(qlib_mqtt PUBLIC
    "$<BUILD_INTERFACE:${ROOT_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(qlib_mqtt PUBLIC PahoMqttCpp::paho-mqttpp3)
