# Set to OFF, because some external libraries rely on this behaviour (for example glfw)
set(CMAKE_LINK_LIBRARIES_ONLY_TARGETS OFF)


# Doctest library
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/doctest-2.4.11/scripts/cmake" PARENT_SCOPE) # can't place this in block scope
block()
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF)
    add_subdirectory(doctest-2.4.11)
    set_target_properties(doctest PROPERTIES SYSTEM ON)
    set_target_properties(doctest PROPERTIES FOLDER "ext")
    # get_target_property(DOCTEST_INCLUDE_DIRS doctest INTERFACE_INCLUDE_DIRECTORIES)
    # set_target_properties(doctest PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${DOCTEST_INCLUDE_DIRS})
    #target_include_directories(doctest SYSTEM INTERFACE ${DOCTEST_INCLUDE_DIRS})
    # ----------------------
endblock()

# fmt library
block()
    add_subdirectory(fmt)
    set_target_properties(fmt PROPERTIES SYSTEM ON)
    set_target_properties(fmt PROPERTIES FOLDER "ext")
endblock()

# utf8proc library
block()
    add_subdirectory(utf8proc)
    set_target_properties(utf8proc PROPERTIES SYSTEM ON)
    set_target_properties(utf8proc PROPERTIES FOLDER "ext")
endblock()

# spdlog library
block()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    set(SPDLOG_FMT_EXTERNAL "ON")
    add_subdirectory(spdlog-1.x)
    set_target_properties(spdlog PROPERTIES SYSTEM ON)
    set_target_properties(spdlog PROPERTIES FOLDER "ext")
    # disable clang-tidy
    set_target_properties(spdlog PROPERTIES CXX_CLANG_TIDY "")
    # disable cppcheck
    set_target_properties(spdlog PROPERTIES CXX_CPPCHECK "")

    #target_compile_definitions(spdlog PRIVATE _ITERATOR_DEBUG_LEVEL=1)

    # add_library(spdlog_header_only INTERFACE)

    # target_include_directories(spdlog_header_only INTERFACE "${CMAKE_CURRENT_LIST_DIR}/spdlog-1.x/include")

    # set_target_properties(spdlog_header_only PROPERTIES SYSTEM ON)
    # set_target_properties(spdlog_header_only PROPERTIES FOLDER "ext")
    # # disable clang-tidy
    # set_target_properties(spdlog_header_only PROPERTIES CXX_CLANG_TIDY "")
    # # disable cppcheck
    # set_target_properties(spdlog_header_only PROPERTIES CXX_CPPCHECK "")
endblock()


# # print helper!!!
# find_package(OpenGL REQUIRED)
# cmake_print_properties(TARGETS OpenGL::GL PROPERTIES
#                        INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES 
#                        LINK_LIBRARIES INTERFACE_LINK_LIBRARIES
#                        COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS
#                        COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS
#                        IMPORTED_LOCATION)









