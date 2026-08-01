cmake_minimum_required(VERSION 3.16)

project(protocol LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PROTOCOL_BUILD_TESTS "Build protocol standalone tests" OFF)

add_library(protocol STATIC
    protocol.c
    Log/Log.c
)

target_include_directories(protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Log
    ${CMAKE_CURRENT_SOURCE_DIR}/static_queue
)

if(PROTOCOL_BUILD_TESTS)
    add_executable(protocol_test
        tests/protocol_test.c
    )

    target_link_libraries(protocol_test PRIVATE protocol)
endif()
