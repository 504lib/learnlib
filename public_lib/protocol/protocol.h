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

DECLARE_STATIC_QUEUE(UART_PROTOCOL_QUEUE, uint8_t, UART_PROTOCOL_FRAME_BUFFER_LEN * 2)

typedef struct
{
    uint8_t Headerframe1;
    uint8_t Headerframe2;
    uint8_t Tailframe1;
    uint8_t Tailframe2;
} Custom_Frame_HT_T;


typedef struct
{
    void*  instance;
    bool (*push)(void* instance, const uint8_t data);
    bool (*pop)(void* instance, uint8_t* data);
} Queue_Operations;

typedef struct
{
    Custom_Frame_HT_T frame_cfg;                                 // 帧头尾配置
    uint32_t           flags;                                    // 运行时标志 (Flag_Type)
    uint8_t            parse_state;                              // 当前解析状态 (Frame_Process_Type)
    uint8_t            frame_buf[UART_PROTOCOL_FRAME_BUFFER_LEN];
    size_t             data_len;
    size_t             payload_index;

    uint8_t            parsing_frame_type;                       // 正在解析的帧类型
    void             (*on_frame)(uint8_t type, const uint8_t* payload, uint16_t len);

    bool             (*tx)(const uint8_t* data, uint16_t len);
    UART_PROTOCOL_QUEUE_t   rx_queue;                                 // 默认串口数据接收队列
    Queue_Operations   queue;                                   // 自定义队列

    struct
    {
        uint8_t  backup[UART_PROTOCOL_FRAME_BUFFER_LEN];
        size_t   backup_len;
        uint8_t  pending_type;                                  // 正在等 ACK 的帧类型
        uint32_t last_tick;
        uint32_t timeout_ms;
        size_t   retry_n;
        size_t   retry_max;
        uint32_t(*get_tick)(void);
        void   (*on_timeout)(uint8_t type);
    } ack;
    struct
    {
        uint8_t last_frame_type;
        uint32_t last_tick;
        uint32_t timeout_ms;
        uint32_t(*get_tick)(void);
    } parse_watchdog;
} UART_protocol_t;

/**
 * @brief    函数指针参数
 * @details  必要参数,若无参数初始化失败
 */
typedef struct
{
    const Custom_Frame_HT_T Head_Tial_Frame_struct;                   // 帧结构定义
    bool (*transmit_function)(const uint8_t* data, uint16_t len); // 数据发送函数
    void (*frame_received_handler)(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len); // 数据帧接收处理函数
}Uart_Protocol_FunctionsParameters;

/**
 * @brief    可选函数指针参数
 * @details  时间基可以选择不提供,若不提供,默认Ack失能
 */
typedef struct
{
  void (*timeout_handler)(uint8_t current_frame_type);                          // 超时处理函数
  uint32_t (*GetTick)(void);
}Uart_Protocol_ACKFunctionsParameters;



// 序列化工具（线上统一采用大端序 BE）
// 反序列化：从字节数组（BE）读取为本机数值
uint32_t rd_u32_be(const uint8_t* p);   // 大端 → 本机 uint32_t
int16_t  rd_i16_be(const uint8_t* p);   // 大端 → 本机 int16_t
float    rd_f32_be(const uint8_t* p);   // 大端 → 本机 float

// 序列化：将本机数值写成字节数组（BE）用于发送
void     wr_u32_be(uint8_t* p, uint32_t v); // 本机 → 大端 uint32_t
void     wr_i16_be(uint8_t* p, int16_t v);  // 本机 → 大端 int16_t
void     wr_f32_be(uint8_t* p, float f);    // 本机 → 大端 float





/************************ 函数接口 *********************** */
bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,Uart_Protocol_FunctionsParameters RequiredParam);
bool Uart_Protocol_Register_ACK(UART_protocol_t* protocol_instance,Uart_Protocol_ACKFunctionsParameters ACKParam);
bool Uart_Protocol_Register_Parse_WatchDog(UART_protocol_t* protocol_instance,uint32_t (*get_tick)(void),const uint32_t time_out);
bool Uart_Protocol_Register_CustomQueue(UART_protocol_t* protocol_instance,Queue_Operations queue_ops);
bool Uart_Protocol_ProcessReceivedData8bit(UART_protocol_t* protocol_instance, uint8_t data);
bool Uart_Protocol_ProcessReceivedDataBuffer(UART_protocol_t* protocol_instance, uint8_t* data,size_t len);
bool Uart_Protocol_Transmit_Frame(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type , uint8_t len);
bool Uart_Protocol_Loop(UART_protocol_t* protocol_instance);
/************************ 发送/接受 *********************** */




