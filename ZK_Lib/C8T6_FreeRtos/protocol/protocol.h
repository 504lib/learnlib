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


void UART_Protocol_INT(UART_protocol UART_protocol_structure,int32_t value);
void UART_Protocol_FLOAT(UART_protocol UART_protocol_structure,float value);
void UART_Protocol_ACK(UART_protocol UART_protocol_structure);
