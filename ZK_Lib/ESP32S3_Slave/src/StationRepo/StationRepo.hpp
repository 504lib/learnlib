#pragma once
/**
 * @file StationRepo.hpp
 * @author whyP762 (3046961251@qq.com)
 * @brief  此文件作为站点链表和操作的定义头文件
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Station/Station.hpp"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../Log/Log.h"

/**
 * @brief    站点最大数量
 * 
 */
#define N 20

class StationRepo
{
private:
    Station_t station_list[N];                          // 站点链表
    uint8_t used_num = 0;                               // 已使用站点数量
    uint8_t current_index = 0;                          // 当前站点索引
    int8_t find_Station(const String& target_name);     // 查找站点索引
public:
    StationRepo() = default;
    ~StationRepo() = default;

    bool Change_current_index(uint8_t index);
    bool Add_Station_to_Tail(const Station_t& station);     
    bool Add_Station_ByIndex(const Station_t& station, uint8_t index);
    bool Add_StationToLater_ByString(const Station_t& station, const String& target_name);
    bool Remove_Station_Head();
    bool Remove_Station_Tail();
    bool Remove_Station_ByString(const String& target_name);
    bool Remove_Station_ByIndex(uint8_t index);
    Station_t& Get_Index_Station(uint8_t index);
    String Get_Index_Station_Name(uint8_t index,bool is_chinese = false);
    String Get_Index_Station_SSID(uint8_t index);
    String Get_Index_Station_Password(uint8_t index);
    String Get_Index_Station_IP(uint8_t index);
    int8_t Get_Index_Station_LastRSSI(uint8_t index);
    uint32_t Get_Index_Station_LastVisitTime(uint8_t index);
    int Get_Index_Station_VisitCount(uint8_t index);
    bool is_Processed(uint8_t index);
    bool is_Connected(uint8_t index);
    uint8_t Get_Current_Index();
    String Get_Current_Station();
    String Get_Current_Station_Chinese();
    bool Move_To_Next_Station();
    bool Move_To_Previous_Station();
    uint8_t Get_Station_Count();
    bool Is_Empty();
    bool Mark_Current_Processed();
    bool Is_Current_Processed();
    bool Change_Station_Password(uint8_t index, String new_password);
    void Print_Station_List(HardwareSerial &Serial);
    void Clear_Station_List();
    void Reset_Processing_Status();
    String Get_StationList_JSON();
}; // end of class StationRepo
