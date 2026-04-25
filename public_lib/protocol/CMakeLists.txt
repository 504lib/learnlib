cmake_minimum_required(VERSION 3.16)

project(protocol LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(protocol STATIC
    protocol.c
    Log/Log.c
)

target_include_directories(protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Log
    ${CMAKE_CURRENT_SOURCE_DIR}/static_queue
)
