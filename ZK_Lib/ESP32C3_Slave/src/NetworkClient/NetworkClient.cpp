#include "NetworkClient.hpp"

/**
 * @brief    异步开始WiFi扫描
 * 
 */
void NetworkClient::startWiFiScan()
{
    if (WiFi.status() != WL_DISCONNECTED && WiFi.status() != WL_IDLE_STATUS) 
    {
        Serial.printf("NetworkClient: WiFi 模块未处于空闲状态，无法开始扫描,状态为:%d\n",WiFi.status());
        return;
    }

    Serial.printf("NetworkClient: 可用内存: %d bytes\n", ESP.getFreeHeap());

    int result = WiFi.scanNetworks(true); // 异步扫描
    if (result == WIFI_SCAN_FAILED) 
    {
        Serial.println("NetworkClient: WiFi 扫描启动失败");
        isScanning = false;
    } 
    else 
    {
        isScanning = true;
        Serial.println("NetworkClient: WiFi 扫描已启动");
    }
}

/**
 * @brief    发送HTTP GET请求并解析JSON响应
 * 
 * @param    url       目标URL字符串
 * @param    response  用于存储解析后的JSON响应的JsonDocument引用
 * @return   true      Json解析成功
 * @return   false     Json解析失败
 */
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

/**
 * @brief    发送HTTP POST请求
 * 
 * @param    url       目标URL字符串
 * @param    payload   POST请求的负载字符串
 * @return   true      POST请求成功
 * @return   false     POST请求失败
 */
bool NetworkClient::sendPostRequest(const String& url, const String& payload) 
{
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST(payload);
    http.end();
    return (httpCode == 200);
}

/**
 * @brief    WiFi连接和重连确保函数
 * 
 * @param    ssid      目标WiFi SSID字符串
 * @param    password  目标WiFi密码字符串
 * @return   true      WiFi已连接
 * @return   false     WiFi连接失败
 */
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

/**
 * @brief    开始WiFi热点模式
 * 
 * @param    ssid      AP的SSID名称
 * @param    password  AP的密码
 * @param    ip        AP的IP地址，默认为 "http://
 * @return   true      AP启动成功
 * @return   false     AP启动失败
 */
bool NetworkClient::startWiFiAP(String ssid, String password,String ip)
{
    IPAddress local_ip(192, 168, 5, 1); // 更改为其他子网
    IPAddress gateway = local_ip;
    IPAddress subnet(255, 255, 255, 0);

    WiFi.mode(WIFI_AP_STA); // 设置为双模式
    WiFi.softAPConfig(local_ip, gateway, subnet); // 配置 AP 的 IP 地址
    bool result = WiFi.softAP(ssid.c_str(), password.c_str());
    if (result) 
    {
        Serial.println("NetworkClient: AP 模式启动成功");
    } 
    else 
    {
        Serial.println("NetworkClient: AP 模式启动失败");
    }
    return result;
}

/**
 * @brief    添加Web服务器路由
 * 
 * @param    path      路由路径字符串
 * @param    handler   路由处理函数
 */
void NetworkClient::addWebRoute(const String& path, ArRequestHandlerFunction handler) {
    server.on(path.c_str(), HTTP_GET, handler);
}

/**
 * @brief    启动Web服务器
 * 
 */
void NetworkClient::beginWebServer() {
    server.begin();
    Serial.println("Web server started!");
}


/**
 * @brief    获取指定SSID的信号强度
 * 
 * @param    ssid      目标SSID字符串
 * @return   int8_t    信号强度值，单位dBm，范围通常为-100到0，-100表示极弱信号，0表示极强信号
 */
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

/**
 * @brief    获取扫描到的最大SSID数量
 * 
 * @return   uint8_t   最大SSID数量
 */
uint8_t NetworkClient::getMaxSSIDNum()
{
    return Max_SSID_NUM;
}


/**
 * @brief    扫描状态检查函数
 * 
 * @return   true      扫描完成,并更新Max_SSID_NUM
 * @return   false     扫描未完成或失败
 */
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
        isScanning = false; // 扫描失败后更新状态
        Serial.println("NetworkClient: WiFi 扫描失败");
        return false; // 扫描失败
    }
    Max_SSID_NUM = scanResults;
    isScanning = false; // 扫描完成后更新状态
    Serial.printf("NetworkClient: WiFi 扫描完成，共找到 %d 个网络\n", Max_SSID_NUM);
    return true; // 返回扫描到的 WiFi 网络数量
}

/**
 * @brief    获取当前WiFi扫描状态
 * 
 * @return   true      WiFi正在扫描
 * @return   false     WiFi未在扫描
 */
bool NetworkClient::isWiFiscanning()
{
    return isScanning;
}