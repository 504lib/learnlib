#pragma once

#include <assert.h>
#include <stdio.h>
// #include "usart.h"
#include <string.h>
#include <stdarg.h>


// 先定义默认级别，再检查是否已定义
#ifndef CURRENT_LOG_LEVEL  
    #define CURRENT_LOG_LEVEL 3
#endif


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

// 修正文件名提取宏
#define LOG_FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

// 修正所有日志宏（添加参数列表）
#if CURRENT_LOG_LEVEL >= 4
    #define LOG_DEBUG(fmt, ...)  printf("[DEBUG] %s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)   // 修正这里
#endif

#if CURRENT_LOG_LEVEL >= 3
    #define LOG_INFO(fmt, ...)  printf("[INFO] %s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_INFO(fmt, ...) ((void)0)    // 修正这里
#endif

#if CURRENT_LOG_LEVEL >= 2
    #define LOG_WARN(fmt, ...)   printf("[WARN] %s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_WARN(fmt, ...) ((void)0)    // 修正这里（注意：WRAN → WARN）
#endif

#if CURRENT_LOG_LEVEL >= 1
    #define LOG_FATAL(fmt, ...)  printf("[FATAL] %s:%d " fmt "\r\n", LOG_FILE_NAME, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_FATAL(fmt, ...) ((void)0)   // 修正这里
#endif

#define LOG_ASSERT(x) do { if (!(x)) { LOG_FATAL("ASSERT ERROR!!!"); for (volatile int _log_assert_block = 1; _log_assert_block; ) { } } } while (0)


StaticBufferError check_pool(const char* buffer, size_t buffer_size,size_t required_size);
