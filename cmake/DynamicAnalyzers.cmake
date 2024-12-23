macro(ML_ENABLE_SANITIZERS project_name)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(SANITIZERS "")

        if(${ML_ENABLE_SANITIZER_ADDRESS})
            list(APPEND SANITIZERS "address")
        endif()

        if(${ML_ENABLE_SANITIZER_LEAK})
            list(APPEND SANITIZERS "leak")
        endif()

        if(${ML_ENABLE_SANITIZER_UNDEFINED})
            list(APPEND SANITIZERS "undefined")
        endif()

        if(${ML_ENABLE_SANITIZER_THREAD})
            if("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
                message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
            else()
                list(APPEND SANITIZERS "thread")
            endif()
        endif()

        if(${ML_ENABLE_SANITIZER_MEMORY} AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
            message(
                WARNING
                "Memory sanitizer requires all the code (including libc++) to be MSan-instrumented otherwise it reports false positives"
            )
            if("address" IN_LIST SANITIZERS
                OR "thread" IN_LIST SANITIZERS
                OR "leak" IN_LIST SANITIZERS)
                message(WARNING "Memory sanitizer does not work with Address, Thread or Leak sanitizer enabled")
        else()
            list(APPEND SANITIZERS "memory")
        endif()
    endif()

    elseif(MSVC)
        if(${ML_ENABLE_SANITIZER_ADDRESS})
            list(APPEND SANITIZERS "address")
        endif()

        if("${ML_ENABLE_SANITIZER_LEAK}"
            OR "${ML_ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}"
            OR "${ML_ENABLE_SANITIZER_THREAD}"
            OR "${ML_ENABLE_SANITIZER_MEMORY}")
            message(WARNING "MSVC only supports address sanitizer")
        endif()
    endif()

    list(
        JOIN
        SANITIZERS
        ","
        LIST_OF_SANITIZERS)

    if(LIST_OF_SANITIZERS)
        if(NOT
        "${LIST_OF_SANITIZERS}"
        STREQUAL
        "")
            if(NOT MSVC)
                target_compile_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
                target_link_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
                # When linking with clang on windows, there's a mismatch MT_StaticRelease - asan and MT_StaticRelease - app.obj
                # So set -MT -MTd flags accordingly (setting them by hand doesn't work?)
                # Also doesn't work with Debug mode on windows
                # We are linking to static CRT library (libcmt)
                # https://stackoverflow.com/questions/14714877/mismatch-detected-for-runtimelibrary
                if (WIN32)
                    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
                endif()
            else()
                target_compile_options(${project_name} INTERFACE /fsanitize=${LIST_OF_SANITIZERS} /Zi /INCREMENTAL:NO)
                target_compile_definitions(${project_name} INTERFACE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
                target_link_options(${project_name} INTERFACE /INCREMENTAL:NO)
            endif()
        endif()
  endif()


endmacro()
