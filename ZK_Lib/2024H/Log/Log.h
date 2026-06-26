#pragma once

#include <assert.h>
#include <stdio.h>
// #include "usart.h"
#include <string.h>
#include <stdarg.h>


#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 64
#endif // !LOG_BUFFER_SIZE


#if __cplusplus
enum class StaticBufferError {
    OK,
    NOT_ENOUGH_MEMORY,
    INVALID_POINTER,
};
#else
typedef enum {
    OK,
    NOT_ENOUGH_MEMORY,
    INVALID_POINTER,
} StaticBufferError;
#endif

#define LOG_USE_QUEUE 1


typedef enum LOG_LEVEL
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4,
    LOG_LEVEL_RAW = 5,
}LOG_LEVEL_T;


#if LOG_USE_QUEUE == 1
void LOG_Process(void);
#endif
void LOG_Init(void (*tx)(const char* buffer, size_t buffer_size));
void LOG_Set_Level(LOG_LEVEL_T level);
LOG_LEVEL_T LOG_Get_Level(void);
void LOG_Set_Transmit(void (*tx)(const char* buffer, size_t buffer_size));
void _LOG_DEBUG(const char* fmt,...);
void _LOG_INFO(const char* fmt,...);
void _LOG_WARN(const char* fmt,...);
void _LOG_FATAL(const char* fmt,...);
void _LOG_ERROR(const char* fmt,...);
void LOG_RAW(const char* fmt,...);
int LOG_Snprintf(char* buffer, size_t buffer_size, const char* fmt, ...);
StaticBufferError check_pool(const char* buffer, size_t buffer_size,size_t required_size);

// 修正文件名提取宏
#define LOG_FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#define LOG_DEBUG(fmt, ...)   _LOG_DEBUG("%s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    _LOG_INFO("%s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    _LOG_WARN("%s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)   _LOG_FATAL("%s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   _LOG_ERROR("%s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__)

#define LOG_ASSERT(x) do { if (!(x)) { LOG_FATAL("ASSERT ERROR!!!"); for (volatile int _log_assert_block = 1; _log_assert_block; ) { } } } while (0)


