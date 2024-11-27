set(SOURCES_LIST
    add.cpp
    parser.cpp
    preprocessor.cpp
    symbol_table.cpp
    tokenize.cpp
    source_map.cpp
    span.cpp
    session.cpp
    diagnostic.cpp
    emitter.cpp
    diag_ctxt.cpp
    parser_errors_reporting.cpp
    semantic_analyzer.cpp
    semantic_analyzer_errors_reporting.cpp
    
)

set(HEADERS_LIST
    add.h
    log.h
    timer.h
    parser.h
    preprocessor.h
    symbol_table.h
    tokenize.h
    source_map.h
    span.h
    diagnostic.h
    emitter.h
    diag_ctxt.h
    error_codes.h
    tokenize.h
    ast.h
    semantic_analyzer.h
    token.h
)

add_library(masmlint_lib ${SOURCES_LIST} ${HEADERS_LIST})


target_link_libraries(
    masmlint_lib
    PRIVATE ml_options
            ml_warnings

            nlohmann_json
            fmt
            utf8proc
)

target_include_directories(masmlint_lib PUBLIC .)


# App
if (WIN32)
    add_executable(masmlint main.cpp)
else()
    add_executable(masmlint main.cpp)
endif()


target_link_libraries(
    masmlint
    PRIVATE 
            masmlint_lib
            ml_options
            ml_warnings

            nlohmann_json
            fmt
            utf8proc
)


# Visual Studio organization
set_target_properties(masmlint PROPERTIES FOLDER "src")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES_LIST} ${HEADERS_LIST})

# set_property(TARGET masmlint PROPERTY
#   MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# set_target_properties(masmlint PROPERTIES
#     OUTPUT_NAME_DEBUG "$masmlint_dbg"
#     OUTPUT_NAME_RELEASE "masmlint"
#     OUTPUT_NAME_RELWITHDEBINFO "$masmlint_reldbg"
# )


set(TARGET_DIR "${CMAKE_SOURCE_DIR}/bin")

# Add a post-build command to copy the executable to the desired directory
add_custom_command(TARGET masmlint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:masmlint>
            "${TARGET_DIR}/$<TARGET_FILE_BASE_NAME:masmlint>$<$<CONFIG:Debug>:_dbg>$<LOWER_CASE:$<TARGET_FILE_SUFFIX:masmlint>>")


# # Define the path to the compile_commands.json file
# set(COMPILE_COMMANDS_JSON "${CMAKE_BINARY_DIR}/compile_commands.json")
# # Add a custom target that depends on your project's targets
# add_custom_target(print_compile_commands ALL
#     COMMAND ${CMAKE_COMMAND} -E echo "Contents of compile_commands.json:"
#     COMMAND ${CMAKE_COMMAND} -E cat "${COMPILE_COMMANDS_JSON}"
#     DEPENDS masmlint
#     COMMENT "Printing compile_commands.json contents..."
# )



