#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class NetworkClient 
{
private:
    uint8_t Max_SSID_NUM = 0;                    // 最大wiFi数量
    bool isScanning = false;
public:
    void startWiFiScan();
    bool checkWiFiScan();
    int8_t RSSI_intesify(String ssid);
    uint8_t getMaxSSIDNum();
    bool sendGetRequest(const String& url, JsonDocument& response); 
    bool sendPostRequest(const String& url, const String& payload); 
    bool ensureWiFiConnected(const char* ssid, const char* password);
};