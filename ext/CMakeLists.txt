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
    set_target_properties(doctest PROPERTIES CXX_CLANG_TIDY "")
endblock()

# fmt library
block()
    add_subdirectory(fmt)
    set_target_properties(fmt PROPERTIES SYSTEM ON)
    set_target_properties(fmt PROPERTIES FOLDER "ext")
    set_target_properties(fmt PROPERTIES CXX_CLANG_TIDY "")
    set_target_properties(fmt PROPERTIES CXX_CPPCHECK "")
endblock()

# utf8proc library
block()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    add_subdirectory(utf8proc)
    set_target_properties(utf8proc PROPERTIES SYSTEM ON)
    set_target_properties(utf8proc PROPERTIES FOLDER "ext")
    set_target_properties(utf8proc PROPERTIES CXX_CLANG_TIDY "")
    set_target_properties(utf8proc PROPERTIES CXX_CPPCHECK "")
endblock()

# utf8proc library
block()
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(json)
    set_target_properties(nlohmann_json PROPERTIES SYSTEM ON)
    set_target_properties(nlohmann_json PROPERTIES FOLDER "ext")
    set_target_properties(nlohmann_json PROPERTIES CXX_CLANG_TIDY "")
    set_target_properties(nlohmann_json PROPERTIES CXX_CPPCHECK "")
endblock()

# # print helper!!!
# find_package(OpenGL REQUIRED)
# cmake_print_properties(TARGETS OpenGL::GL PROPERTIES
#                        INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES 
#                        LINK_LIBRARIES INTERFACE_LINK_LIBRARIES
#                        COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS
#                        COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS
#                        IMPORTED_LOCATION)









