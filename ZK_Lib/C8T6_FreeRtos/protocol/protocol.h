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

#ifndef MAX_PAYLOAD_LEN
#define MAX_PAYLOAD_LEN 32
#endif

#ifndef MAX_CMD_TYPE_HANDLERS
#define MAX_CMD_TYPE_HANDLERS 20
#endif // !MAX_CMD_TYPE_HANDLERS

#include "main.h"
#include "usart.h"
#include "stdbool.h"
#include "Log.h"
typedef struct {
    uint8_t buffer[32]; // 缓冲区大小，比如64或128
    volatile uint16_t head;      // 写指针
    volatile uint16_t tail;      // 读指针
} RingBuffer;

typedef enum
{
    INT = 0,                // 整形
    FLOAT,                  // 浮点
    ACK,                    // ACK 
    PASSENGER_NUM,          // 乘客数量
    CLEAR,                  // 清理指令
    VEHICLE_STATUS
}CmdType;

typedef enum
{
    Route_1 = 0,
    Route_2,
    Route_3,
    Ring_road
}Rounter;

typedef enum 
{ 
    STATUS_SCANNING,
    STATUS_IDLE,
    STATUS_GROPE,
    STATUS_CONNECTING,
    STATUS_CONNECTED,
    STAUS_DISCONNECTED,
    STATUS_WAITING,
    STATUS_ARRIVING,
    STATUS_LEAVING
}VehicleStatus;

typedef struct
{
    uint8_t Headerframe1;       // 帧头一
    uint8_t Headerframe2;       // 帧头二
    uint8_t Tailframe1;         // 帧尾一
    uint8_t Tailframe2;         // 帧尾二
}UART_protocol;


typedef struct
{
    CmdType cmd_type;           // 命令类型
    uint8_t param_value[MAX_PAYLOAD_LEN];         // 参数值 
}prama_Cmd_packet;

// todo: 待淘汰
typedef void (*INT_Callback)(int32_t value);                // 整形回调函数模板
typedef void (*FLOAT_Callback)(float value);                // 浮点回调函数模板
typedef void (*ACK_Callback)(void);                         // ACK信号回调函数模板
typedef void (*PASSENGER_NUM_Callback)(Rounter router,uint8_t value);      // 乘客回调函数模板
typedef void (*CLEAR_Callback)(Rounter router);                       // 清理指令回调函数模板
typedef void (*VehicleStatusCallback)(VehicleStatus status);               // 车辆状态回调函数模板a

typedef void (*FrameHandler)(uint8_t* data); // 帧处理函数模板
typedef void (*FrameTransmit)(uint8_t* data,uint8_t length); // 帧发送函数模板

// 序列化工具（线上统一采用大端序 BE）
// 反序列化：从字节数组（BE）读取为本机数值
uint32_t rd_u32_be(const uint8_t* p);   // 大端 → 本机 uint32_t
float    rd_f32_be(const uint8_t* p);   // 大端 → 本机 float

// 序列化：将本机数值写成字节数组（BE）用于发送
void     wr_u32_be(uint8_t* p, uint32_t v); // 本机 → 大端 uint32_t
void     wr_f32_be(uint8_t* p, float f);    // 本机 → 大端 float


typedef struct {
    uint16_t Size;                                          // 当前数据帧的大小
    RingBuffer ring_buffer;                                 // 环形缓冲区
}UartFrame;




/************************ 函数接口 *********************** */
void UART_Protocol_Init(UART_protocol UART_protocol_structure,FrameTransmit transmit_function);




/************************ 发送/接受 *********************** */

void UART_Protocol_Transmit(prama_Cmd_packet* cmd_packet);
void UART_Protocol_Register_Hander(CmdType cmd_type, FrameHandler handler,uint8_t pram_length);

// todo: 待淘汰
void UART_Protocol_INT(UART_protocol UART_protocol_structure,int32_t value);
void UART_Protocol_FLOAT(UART_protocol UART_protocol_structure,float value);
void UART_Protocol_ACK(UART_protocol UART_protocol_structure);
void UART_Protocol_Passenger(UART_protocol UART_protocol_structure,Rounter router,uint8_t value);
void UART_Protocol_Clear(UART_protocol UART_protocol_structure,Rounter router);
void UART_Protocol_VehicleStatus(UART_protocol UART_protocol_structure,VehicleStatus status);


////////////////////////////

void Receive_Uart_Frame(UART_protocol UART_protocol_structure, uint8_t* data,uint16_t size);

/************************ 设置自定义功能 *********************** */
void set_INT_Callback(INT_Callback cb);
void set_FLOAT_Callback(FLOAT_Callback cb);
void set_ACK_Callback(ACK_Callback cb);
void set_PASSENGER_Callback(PASSENGER_NUM_Callback cb);
void set_Clear_Callback(CLEAR_Callback cb);
void set_VehicleStatus_Callback(VehicleStatusCallback cb);

/************************ 环形缓冲区接收/放置 *********************** */
UartFrame* Get_Uart_Frame_Buffer(void);
void Uart_Buffer_Put_frame(UartFrame* frame,uint8_t* data,uint8_t size);
void Uart_Buffer_Get_frame(UartFrame* frame,uint8_t* data);

