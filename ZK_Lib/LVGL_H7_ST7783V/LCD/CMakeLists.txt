cmake_minimum_required(VERSION 3.22)

# Build the LCD support code as a static library
add_library(lcd STATIC)


file(GLOB LCD_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

target_sources(lcd PRIVATE ${LCD_SOURCES})

target_include_directories(lcd PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/Core/Inc
)

target_link_libraries(lcd PUBLIC
    stm32cubemx
)

