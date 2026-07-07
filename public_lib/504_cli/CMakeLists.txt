cmake_minimum_required(VERSION 3.16)

project(504_CLI_LIB LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(static_queue)
add_subdirectory(Log)

add_library(504_CLI_LIB STATIC
    504_cli.c
)

target_include_directories(504_CLI_LIB PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(504_CLI_LIB PRIVATE
    Log
    static_queue
)
