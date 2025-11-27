#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>


class NetworkClient 
{
private:
    uint8_t Max_SSID_NUM = 0;                    // 最大wiFi数量
    bool isScanning = false;
    AsyncWebServer server; // Web 服务器实例
public:
    NetworkClient() : server(80) {}
    void startWiFiScan();
    bool checkWiFiScan();
    bool isWiFiscanning();
    bool startWiFiAP(String ssid, String password,String ip = "http://192.168.4.1");
    int8_t RSSI_intesify(String ssid);
    uint8_t getMaxSSIDNum();
    bool sendGetRequest(const String& url, JsonDocument& response); 
    bool sendPostRequest(const String& url, const String& payload); 
    bool ensureWiFiConnected(const char* ssid, const char* password);
    void addWebRoute(const String& path, ArRequestHandlerFunction handler);
    void beginWebServer();
};