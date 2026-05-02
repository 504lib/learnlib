add_library(protocol STATIC
    protocol.c
)

target_include_directories(protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(protocol PUBLIC
    Log
    static_queue
)
