cmake_minimum_required(VERSION 3.22)

set(LVGL_LIB lvgl)
add_library(${LVGL_LIB} STATIC)

file(GLOB_RECURSE LVGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/porting/lv_port_disp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/porting/lv_port_indev.c
)

target_sources(${LVGL_LIB} PRIVATE ${LVGL_SOURCES})

target_include_directories(${LVGL_LIB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/porting
    ${CMAKE_SOURCE_DIR}/LCD
    ${CMAKE_SOURCE_DIR}/TOUCH
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

target_compile_definitions(${LVGL_LIB} PUBLIC USE_HAL_DRIVER STM32H743xx)

