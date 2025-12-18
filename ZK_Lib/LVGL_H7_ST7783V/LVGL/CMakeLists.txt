cmake_minimum_required(VERSION 3.22)

set(LVGL_LIB lvgl)
add_library(${LVGL_LIB} STATIC)

file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/porting/lv_port_disp.c
)

target_sources(${LVGL_LIB} PRIVATE ${LVGL_SOURCES})

target_include_directories(${LVGL_LIB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc   # spi.h, usart.h 等
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/porting
    ${CMAKE_SOURCE_DIR}/LCD  # LCD.h
)

