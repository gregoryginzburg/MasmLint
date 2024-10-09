macro(ML_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(SEND_ERROR "IPO is not supported: ${output}")
    endif()
endmacro()


macro(ML_WARNINGS_AS_ERRORS projectName)
    # Warnings as errors from the compiler 
    # set(CMAKE_COMPILE_WARNING_AS_ERROR On)
    if(MSVC)
        target_compile_options(${projectName} INTERFACE /WX)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        target_compile_options(${projectName} INTERFACE -Werror)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${projectName} INTERFACE -Werror)
    else()
        message(AUTHOR_WARNING "No compiler warnings as errors for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'")
    endif()

    # Warnings as errors from cppcheck
    list(APPEND CMAKE_CXX_CPPCHECK --error-exitcode=2)

    # Warnings as errors from clang-tidy
    list(APPEND CMAKE_CXX_CLANG_TIDY -warnings-as-errors=*)
    
endmacro()