#pragma once

/**
 * @file Station.hpp
 * @author whyP762 (3046961251@qq.com)
 * @brief 此文件作为站点的数据结构定义头文件   
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <Arduino.h>

struct Station_t
{
    const char* name;                // 站点名称
    const char* name_ch;             // 站点中文名称
    bool isProcessed;           // 站点是否已处理
    const char* ssid;                // 站点WiFi名称
    const char* password;            // 站点WiFi密码
    const char* ip;                  // 站点IP地址
    int8_t lastRSSI;            // 上次连接的RSSI值
    uint32_t lastVisitTime;     // 上次访问时间戳
    int visitCount;             // 访问次数
    bool isConnnectd;           // 是否已连接
    Station_t() = default;

    // 自定义构造函数
    Station_t(const char* name, const char* name_ch, const char* ssid, const char* password, const char* ip)
        : name(name), name_ch(name_ch), ssid(ssid), password(password), ip(ip) {}
};

