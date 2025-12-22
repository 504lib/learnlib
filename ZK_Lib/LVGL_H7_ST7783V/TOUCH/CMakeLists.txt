
cmake_minimum_required(VERSION 3.22)

set(FT6336_LIB FT6336u)
add_library(${FT6336_LIB} STATIC)

file(GLOB_RECURSE FT6336_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

target_sources(${FT6336_LIB} PRIVATE ${FT6336_SOURCES})

target_include_directories(${FT6336_LIB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${FT6336_LIB} PUBLIC
    stm32cubemx
)

target_compile_definitions(${FT6336_LIB} PUBLIC USE_HAL_DRIVER STM32H743xx)