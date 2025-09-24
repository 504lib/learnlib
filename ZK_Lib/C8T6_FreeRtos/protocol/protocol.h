#pragma once

#include "main.h"
#include "usart.h"

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
    uint16_t len;
    uint8_t *data;
} UartFrame;

void UART_Protocol_INT(UART_protocol UART_protocol_structure,int32_t value);
void UART_Protocol_FLOAT(UART_protocol UART_protocol_structure,float value);
void UART_Protocol_ACK(UART_protocol UART_protocol_structure);
void Receive_Uart_Frame(UART_protocol UART_protocol_structure, UartFrame frame);

void set_INT_Callback(INT_Callback cb);
void set_FLOAT_Callback(FLOAT_Callback cb);
void set_ACK_Callback(ACK_Callback cb);
