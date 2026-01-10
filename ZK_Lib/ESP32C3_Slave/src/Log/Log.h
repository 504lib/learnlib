#pragma once

#include <Arduino.h>
// #include <stdio.h>
// #include "usart.h"
// #include "string.h"
// #include <stdarg.h>

// 先定义默认级别，再检查是否已定义
#ifndef CURRENT_LOG_LEVEL  
    #define CURRENT_LOG_LEVEL 0
#endif

// 修正文件名提取宏
#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

// 修正所有日志宏（添加参数列表）
#if CURRENT_LOG_LEVEL >= 4
    #define LOG_DEBUG(fmt, ...)  Serial.printf("[DEBUG] %s:%d " fmt "\r\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)   // 修正这里
#endif

#if CURRENT_LOG_LEVEL >= 3
    #define LOG_INFO(fmt, ...)  Serial.printf("[INFO] %s:%d " fmt "\r\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_INFO(fmt, ...) ((void)0)    // 修正这里
#endif

#if CURRENT_LOG_LEVEL >= 2
    #define LOG_WARN(fmt, ...)   Serial.printf("[WARN] %s:%d " fmt "\r\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_WARN(fmt, ...) ((void)0)    // 修正这里（注意：WRAN → WARN）
#endif

#if CURRENT_LOG_LEVEL >= 1
    #define LOG_FATAL(fmt, ...)  Serial.printf("[FATAL] %s:%d " fmt "\r\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#else
    #define LOG_FATAL(fmt, ...) ((void)0)   // 修正这里
#endif

// int _write(int file, char* ptr, int len);

int fputc(int ch,FILE* s);
void DMA_UART_printf(const char* fmt, ...);


