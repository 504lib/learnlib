cmake_minimum_required(VERSION 3.22)

set(GUIDER_LIB Guider)
add_library(${GUIDER_LIB} STATIC)

file(GLOB_RECURSE GUIDER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/custom/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dclock/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c
)

target_sources(${GUIDER_LIB} PRIVATE ${GUIDER_SOURCES})

target_include_directories(${GUIDER_LIB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/custom
    ${CMAKE_CURRENT_SOURCE_DIR}/dclock
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
    ${CMAKE_SOURCE_DIR}/LVGL
    ${CMAKE_SOURCE_DIR}/LVGL/src             # 新增：让 core/misc/widgets 等头可见
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

target_link_libraries(${GUIDER_LIB} PUBLIC lvgl)  # 继承 lvgl 的 PUBLIC 包含目录与宏
target_compile_definitions(${GUIDER_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE) # 