/**
 * @file protocol.h
 * @author whyP762 (3046961251@qq.com)
 * @brief  串口通信简易协议帧
 * @version 0.1
 * @date 2025-10-11
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef UART_PROTOCOL_FRAME_BUFFER_LEN
#define UART_PROTOCOL_FRAME_BUFFER_LEN 16
#endif

#include <string.h>
#include "static_queue.h"
#include "Log.h"
#include <stdint.h>

typedef struct
{
    uint8_t Headerframe1;
    uint8_t Headerframe2;
    uint8_t Tailframe1;
    uint8_t Tailframe2;
} Custom_Frame_HT_T;

typedef struct
{
    void* Queue_instance;
    bool (*Queue_pushback)(void* Queue_instance, const uint8_t data);
    bool (*Queue_popfront)(void* Queue_instance, uint8_t* data);
} Queue_Operations;

typedef struct
{
    Custom_Frame_HT_T uart_frame_struct;
    uint32_t hander_flags;
    uint8_t Frame_Process_Type;
    uint8_t frame_buffer[UART_PROTOCOL_FRAME_BUFFER_LEN];
    size_t data_len;
    size_t payload_index;
    struct
    {
        uint8_t current_frame_type;
        void (*frame_received_handler)(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len);
    } event_handler;
    struct
    {
        bool (*transmit_function)(const uint8_t* data, uint16_t len);
    } Send_Operations;
    struct
    {
        uint8_t temp_transmit_buffer[UART_PROTOCOL_FRAME_BUFFER_LEN];
        size_t temp_buffer_len;
        uint32_t tick;
        uint32_t lastTick;
        uint32_t timeout_threshold;
        size_t try_times;
        size_t max_try_times;
        void (*timeout_handler)(uint8_t current_frame_type);
        uint32_t (*GetTick)(void);
    } tickBased_timeout;

    Queue_Operations queue_ops;
} UART_protocol_t;

/**
 * @brief    函数指针参数
 * @details  必要参数,若无参数初始化失败
 */
typedef struct
{
    const Custom_Frame_HT_T Head_Tial_Frame_struct;
    bool (*transmit_function)(const uint8_t* data, uint16_t len);
    void (*frame_received_handler)(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len);
    const Queue_Operations queue_ops;
} Uart_Protocol_FunctionsParameters;

/**
 * @brief    可选函数指针参数
 * @details  时间基可以选择不提供,若不提供,默认Ack失能
 */
typedef struct
{
    void (*timeout_handler)(uint8_t current_frame_type);
    uint32_t (*GetTick)(void);
} Uart_Protocol_OptionalFunctionsParameters;

uint32_t rd_u32_be(const uint8_t* p);
float rd_f32_be(const uint8_t* p);
void wr_u32_be(uint8_t* p, uint32_t v);
void wr_f32_be(uint8_t* p, float f);

bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,
                        Uart_Protocol_FunctionsParameters RequiredParam,
                        Uart_Protocol_OptionalFunctionsParameters OptionalParam);
bool Uart_Protocol_ProcessReceivedData8bit(UART_protocol_t* protocol_instance, uint8_t data);
bool Uart_Protocol_ProcessReceivedDataBuffer(UART_protocol_t* protocol_instance, uint8_t* data, size_t len);
bool Uart_Protocol_Transmit_Frame(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type, uint8_t len);
bool Uart_Protocol_Loop(UART_protocol_t* protocol_instance);
