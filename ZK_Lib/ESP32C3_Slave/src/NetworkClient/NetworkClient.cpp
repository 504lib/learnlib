#include "NetworkClient.hpp"


void NetworkClient::startWiFiScan()
{
    WiFi.scanNetworks();
    isScanning = true;
}

bool NetworkClient::sendGetRequest(const String& url, JsonDocument& response) 
{
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != 200) 
    {
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();
    DeserializationError error = deserializeJson(response, payload);
    return (!error);
}

bool NetworkClient::sendPostRequest(const String& url, const String& payload) 
{
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(payload);
    http.end();
    return (httpCode == 200);
}

bool NetworkClient::ensureWiFiConnected(const char* ssid, const char* password) 
{
    if (WiFi.status() == WL_CONNECTED) 
    {
        return true;
    }
    WiFi.begin(ssid, password);
    for (int i = 0; i < 10; ++i) 
    {
        if (WiFi.status() == WL_CONNECTED) 
        {
            return true;
        }
        delay(1000);
    }
    return false;
}


bool NetworkClient::startWiFiAP(String ssid, String password,String ip)
{
    IPAddress locale_ip;
    locale_ip.fromString(ip);

    WiFi.softAPConfig(locale_ip, locale_ip, IPAddress(255, 255, 255, 0));
    return WiFi.softAP(ssid, password);
}

void NetworkClient::addWebRoute(const String& path, ArRequestHandlerFunction handler) {
    server.on(path.c_str(), HTTP_GET, handler);
}

void NetworkClient::beginWebServer() {
    server.begin();
    Serial.println("Web server started!");
}



int8_t NetworkClient::RSSI_intesify(String ssid)
{
    int scasnResults = WiFi.scanComplete();
    if(Max_SSID_NUM <= 0) return -1;  // 修复：返回-1表示无效
    
    int8_t bestRSSI = -100;
    bool found = false;
    
    for (uint8_t i = 0; i < scasnResults; i++)  // 添加边界检查
    {
        if (WiFi.SSID(i) == ssid)
        {
            int8_t rssi = WiFi.RSSI(i);
            if (rssi > bestRSSI) 
            {
                bestRSSI = rssi;
                found = true;
            }
        }
    }
    WiFi.scanDelete();
    return found ? bestRSSI : -1;
}


uint8_t NetworkClient::getMaxSSIDNum()
{
    return Max_SSID_NUM;
}

bool NetworkClient::checkWiFiScan() 
{
        if (!isScanning) 
        {
            return false; // 未触发扫描
        }

        int scanResults = WiFi.scanComplete();
        if (scanResults == WIFI_SCAN_RUNNING) 
        {
            return false; // 扫描仍在进行中
        } 
        else if (scanResults == WIFI_SCAN_FAILED) 
        {
            isScanning = false;
            return false; // 扫描失败
        }
        Max_SSID_NUM = scanResults;
        isScanning = false; // 扫描完成
        return true; // 返回扫描到的 WiFi 网络数量
}


bool NetworkClient::isWiFiscanning()
{
    return isScanning;
}