add_library(Log STATIC
    Log.c
)

target_include_directories(Log PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)