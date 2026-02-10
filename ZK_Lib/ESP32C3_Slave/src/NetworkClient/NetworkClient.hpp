#pragma once

/**
 * @file NetworkClient.hpp
 * @author whyP762 (3046961251@qq.com)
 * @brief    网络客户端类定义头文件
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "../Log/Log.h"


class NetworkClient 
{
private:
    uint8_t Max_SSID_NUM = 0;                    // 最大wiFi数量
    bool isScanning = false;                        // WiFi扫描状态
    AsyncWebServer server;                      // Web 服务器实例
public:
    NetworkClient() : server(80) {}             // 初始化服务器端口为80
    ~NetworkClient() = default;                 // 默认析构函数
    void startWiFiScan();                   
    bool checkWiFiScan();
    bool isWiFiscanning();
    bool startWiFiAP(String ssid, String password,String ip = "http://192.168.5.1");
    int8_t RSSI_intesify(String ssid);
    uint8_t getMaxSSIDNum();
    bool sendGetRequest(const String& url, JsonDocument& response); 
    bool sendPostRequest(const String& url, const String& payload); 
    bool ensureWiFiConnected(const char* ssid, const char* password);
    void addWebRoute(const String& path, ArRequestHandlerFunction handler);
    void beginWebServer();

    // C风格兼容写法 后续逐步替换 避免动态分配内存
    bool startWiFiAP(const char* ssid, const char* password,const char* ip = "http://192.168.5.1");
    int8_t RSSI_intesify(const char* ssid);
    bool sendGetRequest(const char* url, JsonDocument& response); 
    bool sendPostRequest(const char* url, const char* payload); 
    void addWebRoute(const char* path, ArRequestHandlerFunction handler);
};