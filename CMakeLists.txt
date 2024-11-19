cmake_minimum_required(VERSION 3.25)

project(
    MasmLint
    VERSION 0.1
    LANGUAGES CXX
)

# Temporary because of the new bugged Visual Studio version
# https://github.com/microsoft/cpprestsdk/issues/1768
add_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# To many bugs with c++20 (modmap, chrono bug in github actions default enviroment)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CMAKE_LINK_LIBRARIES_ONLY_TARGETS YES)
# Support folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# For clang-tidy and cppcheck
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT masmlint)

# Turn off deprecation warnings
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

# Includes
include(cmake/PreventInSourceBuilds.cmake)
include(cmake/ProjectOptions.cmake)
include(CMakePrintHelpers)

# Prevent in source builds
ML_assure_out_of_source_builds()
# Create all cache varaibles
ML_declare_options()
# Setup all the tooling
ML_apply_options()


# Third party libraries
add_subdirectory(ext)

# Main sources directory
add_subdirectory(src)

# Tests
if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()


# Installing
# Packaging
