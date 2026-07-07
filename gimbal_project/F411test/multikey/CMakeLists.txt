add_library(multikey STATIC
    multikey.c
)

target_include_directories(multikey PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(multikey PUBLIC
    stm32cubemx
)