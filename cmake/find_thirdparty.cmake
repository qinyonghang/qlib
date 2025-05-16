function(find_thirdparty lib_name lib_url lib_url_hash root_dir download_dir cmake_args)

# find_package(${lib_name} QUIET)

# if (NOT ${lib_name}_FOUND)

set(source_dir ${download_dir}/${lib_name})
if (NOT DEFINED cmake_args OR "${cmake_args}" STREQUAL "")

if (NOT EXISTS ${source_dir})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${root_dir}
        ${Python3_EXECUTABLE} ${root_dir}/scripts/compile.py
            ${lib_name} ${lib_url}
            --url_hash "${lib_url_hash}"
            --download_dir ${download_dir}
            --source_dir ${source_dir}
            --skip_compile
        WORKING_DIRECTORY ${root_dir}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${lib_name}!")
endif()

endif()

message(STATUS "Find Thirdparty in ${source_dir}!")

else()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
set(build_dir ${source_dir}/build2/windows)
set(install_dir ${source_dir}/install2/windows)
set(cmake_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -A x64 ${cmake_args}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
set(build_dir ${source_dir}/build2/win32)
set(install_dir ${source_dir}/install2/win32)
set(cmake_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} -A Win32 ${cmake_args}")
else()
set(build_dir ${source_dir}/build2/linux)
set(install_dir ${source_dir}/install2/linux)
set(cmake_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} ${cmake_args}")
endif()

if (NOT EXISTS ${install_dir})

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env 
        PYTHONPATH=${root_dir}
        ${Python3_EXECUTABLE} ${root_dir}/scripts/compile.py
            ${lib_name} ${lib_url}
            --url_hash ${lib_url_hash}
            --download_dir ${download_dir}
            --source_dir ${source_dir}
            --build_dir ${build_dir}
            --install_dir ${install_dir}
            --cmake_args ${cmake_args}
        WORKING_DIRECTORY ${root_dir}
        RESULT_VARIABLE result
        COMMAND_ECHO STDOUT
)

if(result)
message(FATAL_ERROR "Failed to compile ${lib_name}!")
endif()

endif()

find_package(${lib_name} REQUIRED PATHS ${install_dir} NO_DEFAULT_PATH)

if (${lib_name}_FOUND)
message(STATUS "Find Thirdparty ${lib_name} in ${install_dir}!")
else()
message(FATAL_ERROR "Find Thirdparty ${lib_name} Failed!")
endif()

endif()

# else()

# message(STATUS "Find Thirdparty!")

# endif()

endfunction()
