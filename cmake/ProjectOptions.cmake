include(CMakeDependentOption)

macro(ML_declare_options)
    # Compiler settings
    option(ML_ENABLE_IPO "Enable IPO/LTO" ON)
    option(ML_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)

    # Static Analysis Tools
    option(ML_ENABLE_COMPILER_WARNINGS "Enable extensive compiler warnings" ON)
    option(ML_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(ML_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)

    # Dynamic Analysis Tools
    option(ML_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ON)
    option(ML_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ML_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(ML_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ML_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    if(ML_ENABLE_SANITIZER_ADDRESS OR ML_ENABLE_SANITIZER_LEAK OR ML_ENABLE_SANITIZER_UNDEFINED 
        OR ML_ENABLE_SANITIZER_THREAD OR ML_ENABLE_SANITIZER_MEMORY)
        set(ML_ENABLE_SANITIZERS ON CACHE BOOL "Enable sanitizers" FORCE)
    endif()

    # Tests
    option(BUILD_TESTING "Enable tesing" ON)

    # Code Coverage
    option(ML_ENABLE_COVERAGE "Enable coverage reporting" OFF)

endmacro()


macro(ML_apply_options)
    add_library(ml_options INTERFACE)
    add_library(ml_warnings INTERFACE)
    
    # General options
    include(cmake/StandartProjectSettings.cmake)
    ML_setup_build_type()
    RL_enable_colored_warnings() # TODO: not working, why???  

    # Compiler settings
    include(cmake/CompilerSettings.cmake)
    if(ML_ENABLE_IPO)
        ML_ENABLE_IPO()
    endif()

    if (ML_WARNINGS_AS_ERRORS)
        ML_WARNINGS_AS_ERRORS(ml_warnings)
    endif()

    # Static Analysis Tools
    # General warnings
    include(cmake/CompilerWarnings.cmake)
    if (ML_ENABLE_COMPILER_WARNINGS)
        ML_set_project_warnings(ml_warnings)
    endif()
    # clang-tidy
    include(cmake/StaticAnalyzers.cmake)
    if(ML_ENABLE_CLANG_TIDY)
        ML_ENABLE_CLANG_TIDY()
    endif()
    # cppcheck
    if(ML_ENABLE_CPPCHECK)
        ML_ENABLE_CPPCHECK()
    endif()

    # Dynamic Analysis Tools
    include(cmake/DynamicAnalyzers.cmake)
    if(ML_ENABLE_SANITIZERS)
        ML_ENABLE_SANITIZERS(ml_options)
    endif()

    # Code Coverage
    include(cmake/CodeCoverage.cmake)
    if(ML_ENABLE_COVERAGE)
        ML_ENABLE_COVERAGE(ml_options)
    endif()

    # run vcvarsall when msvc is used
    include(cmake/VCEnviroment.cmake)
    run_vcvarsall()

endmacro()

