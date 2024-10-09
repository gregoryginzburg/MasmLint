set(SOURCES_LIST
    test_add.cpp

)

set(HEADERS_LIST

)

add_executable(tests ${SOURCES_LIST} ${HEADERS_LIST})
target_link_libraries(tests PRIVATE masmlint_lib)
target_link_libraries(tests PRIVATE doctest::doctest ml_options ml_warnings)


include(doctest)
doctest_discover_tests(tests EXTRA_ARGS "--force-colors")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES_LIST} ${HEADERS_LIST})



# cmake_print_properties(TARGETS tests PROPERTIES
#                        INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES 
#                        LINK_LIBRARIES INTERFACE_LINK_LIBRARIES
#                        COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS
#                        COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)


# function(make_test test source)
#     add_executable(test-${test} ${source})
#     target_link_libraries(test-${test} PRIVATE doctest::doctest rs_lib)
#     add_test(NAME ${test} COMMAND test-${test})
# endfunction()

# make_test(test_add test_add.cpp)