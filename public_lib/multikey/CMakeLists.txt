cmake_minimum_required(VERSION 3.10)

add_library(multikey STATIC
    multikey.c
)

target_include_directories(multikey PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
