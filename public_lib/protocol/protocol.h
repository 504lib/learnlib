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
    uint8_t Headerframe1;       // 帧头一
    uint8_t Headerframe2;       // 帧头二
    uint8_t Tailframe1;         // 帧尾一
    uint8_t Tailframe2;         // 帧尾二
}Custom_Frame_HT_T;


typedef struct
{
  void* Queue_instance;                                           // 串口数据队列实例指针
  bool (*Queue_pushback)(void* Queue_instance, const uint8_t data); // 数据入队回调函数
  bool (*Queue_popfront)(void* Queue_instance, uint8_t* data);      // 数据出队回调函数
}Queue_Operations;                                                // 串口数据队列操作函数集合

typedef struct 
{
  const Custom_Frame_HT_T uart_frame_struct;   // 帧结构定义
  uint32_t hander_flags;  
  uint8_t Frame_Process_Type;                  // 数据帧处理状态枚举
  uint8_t frame_buffer[UART_PROTOCOL_FRAME_BUFFER_LEN];       // 整帧接收缓冲区（帧头+类型+长度+载荷+校验+帧尾）
  size_t data_len;
  size_t payload_index;
  struct
  {
    uint8_t current_frame_type;                                   // 当前接收到的帧类型
    void (*frame_received_handler)(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len); // 数据帧接收处理函数
  }event_handler;                                                // 事件处理函数集合
  struct
  {
    bool (*transmit_function)(const uint8_t* data, uint16_t len); // 数据发送函数
  }Send_Operations;                                                  // 数据发送函数集合
  struct
  {
    uint8_t temp_transmit_buffer[UART_PROTOCOL_FRAME_BUFFER_LEN];    // 临时发送缓冲区（帧头+类型+长度+载荷+校验+帧尾）
    size_t temp_buffer_len;
    uint32_t tick;                                                // 定时器周期（ms）
    uint32_t lastTick;                                      
    uint32_t timeout_threshold;                                   // 接收超时阈值（ms）
    size_t try_times;                                             // 接收重试次数
    size_t max_try_times;                                   // 最大重试次数（编译时常量）
    void (*timeout_handler)(uint8_t current_frame_type);                          // 超时处理函数
    uint32_t (*GetTick)(void);
  }tickBased_timeout;                                                // 定时器相关配置和处理函数

  Queue_Operations queue_ops;                                                // 串口数据队列操作函数集合
}UART_protocol_t;

/**
 * @brief    函数指针参数
 * @details  必要参数,若无参数初始化失败
 */
typedef struct
{
    const Custom_Frame_HT_T Head_Tial_Frame_struct;                   // 帧结构定义
    const bool (*transmit_function)(const uint8_t* data, uint16_t len); // 数据发送函数
    const void (*frame_received_handler)(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len); // 数据帧接收处理函数
    const Queue_Operations queue_ops;                                                // 串口数据队列操作函数集合
}Uart_Protocol_FunctionsParameters;

/**
 * @brief    可选函数指针参数
 * @details  时间基可以选择不提供,若不提供,默认Ack失能
 */
typedef struct
{
    const void (*timeout_handler)(uint8_t current_frame_type);                          // 超时处理函数
    const uint32_t (*GetTick)(void);
}Uart_Protocol_OptionalFunctionsParameters;

// 序列化工具（线上统一采用大端序 BE）
// 反序列化：从字节数组（BE）读取为本机数值
uint32_t rd_u32_be(const uint8_t* p);   // 大端 → 本机 uint32_t
float    rd_f32_be(const uint8_t* p);   // 大端 → 本机 float

// 序列化：将本机数值写成字节数组（BE）用于发送
void     wr_u32_be(uint8_t* p, uint32_t v); // 本机 → 大端 uint32_t
void     wr_f32_be(uint8_t* p, float f);    // 本机 → 大端 float





/************************ 函数接口 *********************** */
bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,
                        Uart_Protocol_FunctionsParameters RequiredParam,
                        Uart_Protocol_OptionalFunctionsParameters OptionalParam);
bool Uart_Protocol_ProcessReceivedData8bit(UART_protocol_t* protocol_instance, uint8_t data);
bool Uart_Protocol_ProcessReceivedDataBuffer(UART_protocol_t* protocol_instance, uint8_t* data,size_t len);
bool Uart_Protocol_Transmit_Frame(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type , uint8_t len);
bool Uart_Protocol_Loop(UART_protocol_t* protocol_instance);
/************************ 发送/接受 *********************** */




