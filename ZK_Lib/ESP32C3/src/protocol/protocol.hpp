#pragma once

#include <Arduino.h>
enum class Rounter
{
    Route_1 = 0,
    Route_2,
    Route_3,
    Ring_road
};

enum class CmdType 
{ 
    INT = 0,
    FLOAT,
    ACK,
    PASSENGER_NUM,
    CLEAR,
};
typedef void (*IntCallback)(int32_t value);
typedef void (*FloatCallback)(float value);
typedef void (*AckCallback)(void);
typedef void (*PassengerNumCallback)(Rounter rounter,uint8_t value);
typedef void (*ClearCallback)(Rounter rounter);
class protocol
{
private:
    uint8_t Headerframe1;
    uint8_t Headerframe2;
    uint8_t Tailframe1;
    uint8_t Tailframe2;
    uint16_t calculateChecksum(const uint8_t* data, size_t length);
    IntCallback intCallback = nullptr;
    FloatCallback floatCallback = nullptr;
    AckCallback ackCallback = nullptr;
    PassengerNumCallback passengerNumCallback = nullptr;
    ClearCallback clearcallback = nullptr;
public:
    protocol(uint8_t header1, uint8_t header2, uint8_t tail1, uint8_t tail2);
    ~protocol();
    void Send_Uart_Frame(int32_t num);
    void Send_Uart_Frame(float num);
    void Send_Uart_Frame_ACK();
    void Send_Uart_Frame_PASSENGER_NUM(Rounter rounter,uint8_t value);
    void Send_Uart_Frame_CLEAR(Rounter rounter);
    void Receive_Uart_Frame(uint8_t data);
    void setIntCallback(IntCallback cb);
    void setFloatCallback(FloatCallback cb);
    void setAckCallback(AckCallback cb);
    void setPassengerNumCallback(PassengerNumCallback cb);
    void setClearCallback(ClearCallback cb);
};