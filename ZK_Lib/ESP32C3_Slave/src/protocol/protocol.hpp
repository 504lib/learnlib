#pragma once

#include <Arduino.h>
#include "../Vehicle/Vehicle.hpp"
#include "../protocol/protocol.hpp"
#include "../Log/Log.h"

#ifndef MAX_CMD_TYPE_NUM
#define MAX_CMD_TYPE_NUM 10
#endif // !MAX_CMD_TYPE_NUM

enum class CmdType 
{ 
    INT = 0,
    FLOAT,
    ACK,
    PASSENGER_NUM,
    CLEAR,
    VEHICLE_STATUS
};

using CmdHandler = void (*)(CmdType cmd, const uint8_t* payload, uint8_t len);
typedef struct
{
  CmdType type;
  union
  {
    int32_t int_value;
    float   float_value;
    VehicleStatus status;
     struct
    {
      Rounter router;
      uint8_t passenger_num;
    }passenger; 
  }value;
}ACK_Queue_t;

// enum class VehicleStatus 
// {
//   WAITING,     // 候车中
//   ARRIVING,     // 即将进站
//   LEAVING      // 离站
// };
typedef void (*IntCallback)(int32_t value);
typedef void (*FloatCallback)(float value);
typedef void (*AckCallback)(void);
typedef void (*PassengerNumCallback)(Rounter rounter,uint8_t value);
typedef void (*ClearCallback)(Rounter rounter);
typedef void (*VehicleStatusCallback)(VehicleStatus status);
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
    VehicleStatusCallback vehicleStatusCallback = nullptr;
    CmdHandler handlers[6] = { nullptr };
public:
    protocol(uint8_t header1, uint8_t header2, uint8_t tail1, uint8_t tail2);
    ~protocol();
    void Send_Uart_Frame(int32_t num);
    void Send_Uart_Frame(float num);
    void Send_Uart_Frame_ACK();
    void Send_Uart_Frame(Rounter rounter,uint8_t value);
    // void Send_Uart_Frame_PASSENGER_NUM(Rounter rounter,uint8_t value);
    void Send_Uart_Frame(Rounter rounter);
    // void Send_Uart_Frame_CLEAR(Rounter rounter);
    // void Set_Vehicle_Status(VehicleStatus status);
    void Send_Uart_Frame(VehicleStatus status);
    void Receive_Uart_Frame(uint8_t data);
    void Register_Hander(CmdType cmd, CmdHandler handler);
    // todo: remove
    void setIntCallback(IntCallback cb);
    void setFloatCallback(FloatCallback cb);
    void setAckCallback(AckCallback cb);
    void setPassengerNumCallback(PassengerNumCallback cb);
    void setClearCallback(ClearCallback cb);
    void setVehicleStatusCallback(VehicleStatusCallback cb);
};

// 序列化工具（线上统一采用大端序 BE）
// 反序列化：从字节数组（BE）读取为本机数值
uint32_t rd_u32_be(const uint8_t* p);   // 大端 → 本机 uint32_t
float    rd_f32_be(const uint8_t* p);   // 大端 → 本机 float

// 序列化：将本机数值写成字节数组（BE）用于发送
void     wr_u32_be(uint8_t* p, uint32_t v); // 本机 → 大端 uint32_t
void     wr_f32_be(uint8_t* p, float f);    // 本机 → 大端 float