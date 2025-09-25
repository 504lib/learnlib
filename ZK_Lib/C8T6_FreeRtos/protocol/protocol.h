#pragma once

#include "main.h"
#include "usart.h"
#include "stdbool.h"

typedef struct {
    uint8_t buffer[32]; // 缓冲区大小，比如64或128
    volatile uint16_t head;      // 写指针
    volatile uint16_t tail;      // 读指针
} RingBuffer;

typedef enum
{
    INT = 0,
    FLOAT,
    ACK
}CmdType;

typedef struct
{
    uint8_t Headerframe1;
    uint8_t Headerframe2;
    uint8_t Tailframe1;
    uint8_t Tailframe2;
}UART_protocol;

typedef void (*INT_Callback)(int32_t value);
typedef void (*FLOAT_Callback)(float value);
typedef void (*ACK_Callback)(void);

typedef struct {
    uint16_t Size;
    RingBuffer ring_buffer;
}UartFrame;

void UART_Protocol_INT(UART_protocol UART_protocol_structure,int32_t value);
void UART_Protocol_FLOAT(UART_protocol UART_protocol_structure,float value);
void UART_Protocol_ACK(UART_protocol UART_protocol_structure);
void Receive_Uart_Frame(UART_protocol UART_protocol_structure, uint8_t* data,uint16_t size);

void set_INT_Callback(INT_Callback cb);
void set_FLOAT_Callback(FLOAT_Callback cb);
void set_ACK_Callback(ACK_Callback cb);

UartFrame* Get_Uart_Frame_Buffer(void);
void Uart_Buffer_Put_frame(UartFrame* frame,uint8_t* data,uint8_t size);
void Uart_Buffer_Get_frame(UartFrame* frame,uint8_t* data);

