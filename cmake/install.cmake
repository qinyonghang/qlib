
set(LIB_NAME qlib)
set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_BINDIR bin)
set(export_dest_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME})

install(FILES ${PROJECT_SOURCE_DIR}/cmake/tests.cmake DESTINATION . RENAME CMakeLists.txt)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/tests DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION .)
install(TARGETS ${LIB_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)
set(project_config_in "${PROJECT_SOURCE_DIR}/cmake/${LIB_NAME}Config.cmake.in")
set(project_config_out "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}Config.cmake")
configure_package_config_file(${project_config_in} ${project_config_out}
    INSTALL_DESTINATION ${export_dest_dir})
install(FILES ${project_config_out} DESTINATION ${export_dest_dir})
