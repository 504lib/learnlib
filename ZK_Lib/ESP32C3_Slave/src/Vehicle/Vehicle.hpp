#pragma once

/**
 * @file Vehicle.hpp
 * @author whyP762 (3046961251@qq.com)
 * @brief   此文件作为车辆的数据结构定义头文件
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../Log/Log.h"

#define MAX_VEHICLE_STATUS_STRING_LENGTH 16
#define MAX_VEHICLE_JSON_LENGTH 128
#define MAX_VEHICLE_PLATE_LENGTH 16
#define MAX_VEHICLE_SSID_LENGTH 16
#


enum class Rounter
{
    Route_1 = 0,            // 路线1
    Route_2,                // 路线2
    Route_3,                // 路线3
    Ring_road               // 环线
};

enum class VehicleStatus
{ 
    STATUS_SCANNING,            // 扫描中
    STATUS_IDLE,                // 空闲中
    STATUS_GROPE,               // 试图连接
    STATUS_CONNECTING,          // 连接中
    STATUS_CONNECTED,           // 已连接
    STAUS_DISCONNECTED,         // 已断开
    STATUS_WAITING,             // 等待中
    STATUS_ARRIVING,            // 即将进站
    STATUS_LEAVING              // 离站中
};

struct Vehicle_t
{
    const char* Plate;                                       // 车牌号
    const char* SSID;                                        // 车辆WiFi名称
    const char* Password = "12345678";                       // 车辆WiFi密码
    const char* StationServer = "http://192.168.4.1";       // 车辆站点服务器地址
    Rounter currentRoute;                               // 当前路线
    VehicleStatus status = VehicleStatus::STATUS_SCANNING;  // 当前状态
    VehicleStatus last_status = VehicleStatus::STATUS_SCANNING; // 上次状态
    Vehicle_t()
        : Plate("")
        , SSID("")
        , Password("12345678")
        , StationServer("http://192.168.4.1")
        , currentRoute(Rounter::Route_1)
        , status(VehicleStatus::STATUS_SCANNING)
        , last_status(VehicleStatus::STATUS_SCANNING)
    {}
    Vehicle_t(const char* plate, const char* ssid, const char* password, const char* stationServer, Rounter route, VehicleStatus status)
        : Plate(plate), SSID(ssid), Password(password), StationServer(stationServer), currentRoute(route), status(status) {}
};

class Vehicle_Info
{
    private:
        Vehicle_t vehicle;
    public:
        Vehicle_Info(Vehicle_t vehicle_info) : vehicle(vehicle_info) {
            LOG_INFO("Vehicle_Info: 车辆信息初始化完成，车牌号：%s，WiFi名称：%s，站点服务器：%s",
                     vehicle.Plate, vehicle.SSID, vehicle.StationServer);
        }
        String Get_Vehicle_Plate();
        String Get_Vehicle_SSID();
        String Get_Vehicle_Password();
        String Get_Vehicle_StationServer();
        Rounter Get_Vehicle_Rounter();
        VehicleStatus Get_Vehicle_Status();
        String Get_Status_Str(Vehicle_t vehicle);
        String Get_Status_Str(VehicleStatus status);
        bool Update_Vehicle_Status(VehicleStatus new_status);
        String Vehiicle_Json();


        // C风格兼容写法 后续逐步替换 避免动态分配内存
        size_t Get_Vehicle_Plate(char* buffer, size_t buffer_size);
        size_t Get_Vehicle_SSID(char* buffer, size_t buffer_size);
        size_t Get_Vehicle_Password(char* buffer, size_t buffer_size);
        size_t Get_Vehicle_StationServer(char* buffer, size_t buffer_size);
        size_t Get_Status_Str(char* buffer, size_t buffer_size, VehicleStatus status);
        size_t Get_Status_Str(char* buffer, size_t buffer_size, Vehicle_t vehicle);
        size_t Vehiicle_Json(char* buffer, size_t buffer_size);
};