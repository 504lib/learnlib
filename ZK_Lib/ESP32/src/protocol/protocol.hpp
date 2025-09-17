#pragma once

#include <Arduino.h>

enum class CmdType 
{ 
    INT = 0,
    FLOAT,
    ACK
};

class protocol
{
private:
    uint8_t Headerframe1;
    uint8_t Headerframe2;
    uint8_t Tailframe1;
    uint8_t Tailframe2;
    uint16_t calculateChecksum(const uint8_t* data, size_t length);
public:
    protocol(uint8_t header1, uint8_t header2, uint8_t tail1, uint8_t tail2);
    ~protocol();
    void Send_Uart_Frame(int num);
    void Send_Uart_Frame(float num);
    void Send_Uart_Frame_ACK();
};
