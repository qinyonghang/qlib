function(compile lib_name lib_url lib_url_hash root_dir download_dir cmake_args custom_command)

    set(source_dir ${download_dir}/${lib_name})

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(target_dir windows)
        else()
            set()
            set(target_dir win32)
        endif()
    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(target_dir linux)
        else()
            set(target_dir linux32)
        endif()
    else()
        set(target_dir "")
    endif()

    set(build_dir ${source_dir}/build2/${target_dir})
    set(install_dir ${source_dir}/install2/${target_dir})

    # 根据参数类型选择构建方式
    if (NOT "${cmake_args}" STREQUAL "")

        if (NOT EXISTS ${install_dir})
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E env
                    PYTHONPATH=${root_dir}
                    ${Python3_EXECUTABLE} ${root_dir}/scripts/compile.py
                        ${lib_name} ${lib_url}
                        --url_hash "${lib_url_hash}"
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

    elseif (NOT "${custom_command}" STREQUAL "")

        if (NOT EXISTS ${install_dir})
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E env
                    PYTHONPATH=${root_dir}
                    ${Python3_EXECUTABLE} ${root_dir}/scripts/compile.py
                        ${lib_name} ${lib_url}
                        --url_hash "${lib_url_hash}"
                        --download_dir ${download_dir}
                        --source_dir ${source_dir}
                        --build_dir ${build_dir}
                        --install_dir ${install_dir}
                        --custom_compile ${custom_command}
                WORKING_DIRECTORY ${root_dir}
                RESULT_VARIABLE result
                COMMAND_ECHO STDOUT
            )
            if(result)
                message(FATAL_ERROR "Failed to compile ${lib_name}!")
            endif()
        endif()

    else()

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

    endif()

endfunction()
