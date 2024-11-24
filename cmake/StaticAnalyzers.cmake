macro (ML_ENABLE_CLANG_TIDY)
    find_program(CLANGTIDY clang-tidy)
    if(CLANGTIDY)

    # construct the clang-tidy command line
    set(CMAKE_CXX_CLANG_TIDY
        ${CLANGTIDY}
        # enabled checks are in .clang-tidy file       
        -extra-arg=-Wno-unknown-warning-option
        -extra-arg=-Wno-ignored-optimization-argument
        -extra-arg=-Wno-unused-command-line-argument
        -extra-arg=-std=c++${CMAKE_CXX_STANDARD}
        -p ${CMAKE_BINARY_DIR}
        # if warnings as errors (-warnings-as-errors=clang-analyzer*)
        ${CMAKE_CXX_CLANG_TIDY}
    )

    # CMAKE_CXX_CLANG_TIDY automatically works only for Makefile and Ninja generators
    # if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
    #     list(APPEND CMAKE_CXX_CPPCHECK src)
    #     add_custom_target(clangtidy ALL
    #         COMMAND ${CMAKE_CXX_CLANG_TIDY}
    #         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    #         COMMENT "Static code analysis using clang-tidy"
    #     )
    # endif()

    else()
        message(WARNING ${WARNING_MESSAGE} "clang-tidy requested but executable not found")
    endif()

endmacro()


macro(ML_ENABLE_CPPCHECK)
    find_program(CPPCHECK cppcheck)
    if(CPPCHECK)
        if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
            set(CPPCHECK_TEMPLATE "vs")
        else()
            set(CPPCHECK_TEMPLATE "gcc")
        endif()

        if (WIN32)
            set(CPPCHECK_PLATFORM "win64")
        else()
            set(CPPCHECK_PLATFORM "unix64")
        endif()      

        set(CMAKE_CXX_CPPCHECK
            ${CPPCHECK}
            --template=${CPPCHECK_TEMPLATE}
            --enable=style,performance,warning,portability
            --inline-suppr
            # We cannot act on a bug/missing feature of cppcheck
            --suppress=cppcheckError
            --suppress=internalAstError
            # if a file does not have an internalAstError, we get an unmatchedSuppression error
            --suppress=unmatchedSuppression
            # noisy and incorrect sometimes
            --suppress=passedByValue
            # ignores code that cppcheck thinks is invalid C++
            --suppress=syntaxError
            --suppress=preprocessorErrorDirective
            --inconclusive
            # not useful suggestions
            --suppress=useInitializationList
            --suppress=functionStatic
            --suppress=unusedPrivateFunction
            --suppress=unusedStructMember
            --suppress=useStlAlgorithm
            # if warnings as errors (--error-exitcode=2)
            ${CMAKE_CXX_CPPCHECK}
            # always set globally
            --std=c++${CMAKE_CXX_STANDARD}
            # 
            --platform=${CPPCHECK_PLATFORM}
        )

        # CMAKE_CXX_CPPCHECK automatically works only for Makefile and Ninja generators
        # if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
        #     list(APPEND CMAKE_CXX_CPPCHECK src)
        #     add_custom_target(cppcheck ALL
        #         COMMAND ${CMAKE_CXX_CPPCHECK} .
        #         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        #         COMMENT "Static code analysis using cppcheck"
        #     )
        # endif()

    else()
        message(WARNING "cppcheck requested but executable not found")
    endif()
endmacro()


macro(RS_enable_include_what_you_use)
    find_program(INCLUDE_WHAT_YOU_USE include-what-you-use)
    if(INCLUDE_WHAT_YOU_USE)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE})
    else()
        message(${WARNING_MESSAGE} "include-what-you-use requested but executable not found")
    endif()
endmacro()

