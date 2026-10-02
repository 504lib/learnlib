cmake_minimum_required(VERSION 3.16)

project(Log LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(Log STATIC Log.c)

target_include_directories(Log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_log test_log.c)
target_link_libraries(test_log PRIVATE Log)
