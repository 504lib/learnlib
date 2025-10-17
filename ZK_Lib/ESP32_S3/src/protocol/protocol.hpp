#pragma once

#include <Arduino.h>
#include <stdlib.h>

enum class CmdType 
{ 
    INT = 0,
    FLOAT,
    ACK,
    PASSENGER_NUM,
    CLEAR,
    HX711_WEIGHT,
    CURRENT_USER,
};

enum class Medicine 
{ 
    Medicine1,
    Medicine2, 
};


typedef void (*IntCallback)(int32_t value);
typedef void (*FloatCallback)(float value);
typedef void (*AckCallback)(void);
typedef void (*PassengerNumCallback)(uint8_t value);
typedef void (*ClearCallback)(void);
typedef void (*HX711_WEIGHT_Callback)(float weight);
typedef void (*CURRENT_USER_Callback)(String name,Medicine medicine);

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
    HX711_WEIGHT_Callback weight_callback = nullptr;
    CURRENT_USER_Callback current_user_callback = nullptr;
public:
    protocol(uint8_t header1, uint8_t header2, uint8_t tail1, uint8_t tail2);
    ~protocol();
    void Send_Uart_Frame(int32_t num);
    void Send_Uart_Frame(float num);
    void Send_Uart_Frame_ACK();
    void Send_Uart_Frame_PASSENGER_NUM(uint8_t value);
    void Send_Uart_Frame_WEIGHT(float weight_value);
    void Send_Uart_Frame_CLEAR();
    void Send_Uart_Frame_CURRENT_USER(String name,Medicine medicine);
    void Receive_Uart_Frame(uint8_t data);
    void setIntCallback(IntCallback cb);
    void setFloatCallback(FloatCallback cb);
    void setAckCallback(AckCallback cb);
    void setPassengerNumCallback(PassengerNumCallback cb);
    void setClearCallback(ClearCallback cb);
    void setWeightCallback(HX711_WEIGHT_Callback cb);
    void setCurrentUserCallback(CURRENT_USER_Callback cb);
};
