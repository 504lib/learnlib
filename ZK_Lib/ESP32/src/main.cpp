#include <Arduino.h>
#include "./protocol/protocol.hpp"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
// #include "

const char* ssid = "ESP32-Access-Point"; // AP 鍚嶇О
const char* password = "12345678"; // AP 瀵嗙爜
#define LED_Pin GPIO_NUM_48
AsyncWebServer server(80);
static int count = 0;
static float fcount = 0.0;
uint32_t lastPrint = 0;
bool Flag_INT = false;
bool Flag_FLOAT = false;
bool Flag_ACK = false;

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);

void IRAM_ATTR handleInterrupt1() {
  Flag_INT = true;
}

void IRAM_ATTR handleInterrupt2() {
  
  Flag_FLOAT = true;
}


void IRAM_ATTR handleInterrupt3() {
  Flag_ACK = true;
}


void setup() 
{
  
  pinMode(LED_Pin,OUTPUT);
  pinMode(11, INPUT); // 璁剧疆涓鸿緭鍏?
  pinMode(12, INPUT); // 璁剧疆涓鸿緭鍏?
  pinMode(13, INPUT); // 璁剧疆涓鸿緭鍏?
  attachInterrupt(11, handleInterrupt1, RISING); // 涓婂崌娌胯Е鍙?
  attachInterrupt(12, handleInterrupt2, RISING); // 涓婂崌娌胯Е鍙?
  attachInterrupt(13, handleInterrupt3, RISING); // 涓婂崌娌胯Е鍙?
 Serial.println("正在启动 AP 模式...");
  
  // 启动 AP
  if (WiFi.softAP(ssid, password)) {
    Serial.println("AP 模式启动成功!");
    
    // 获取 AP 的 IP 地址
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP 地址: ");
    Serial.println(myIP);
    
    // 显示连接信息
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("密码: ");
    Serial.println(password);
  } else {
    Serial.println("AP 模式启动失败!");
    return;
  }

  // 设置 Web 服务器路由
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><title>ESP32-S3 AP模式</title></head>";
    html += "<body><h1>欢迎使用ESP32-S3 AP模式</h1>";
    html += "<p>IP地址: " + WiFi.softAPIP().toString() + "</p>";
    html += "<p>已连接设备数: " + String(WiFi.softAPgetStationNum()) + "</p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"ssid\":\"" + String(ssid) + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  // 启动服务器
  server.begin();
  Serial.println("HTTP 服务器已启动");
}

void loop() 
{
  if(Serial1.available())
  {
    uart_protocol.Receive_Uart_Frame(Serial1.read());
    // Serial.write(Serial1.read());
  }
  if (Flag_INT)
  {
    uart_protocol.Send_Uart_Frame(count++);
    // Serial.println(count++);
    Flag_INT = false;
  }
  if (Flag_FLOAT)
  {
    uart_protocol.Send_Uart_Frame(fcount);
   fcount += 0.1;
    // Serial.println(fcount, 1);
    Flag_FLOAT = false;
  }
  if (Flag_ACK)
  {
    uart_protocol.Send_Uart_Frame_ACK();
    // Serial.println("ACK");
    Flag_ACK = false;
  }
  if (millis() - lastPrint > 10000) {  // 姣?10绉掓墦鍗颁竴娆?
    lastPrint = millis();
    Serial.print("已连接从机数量 ");
    Serial.println(WiFi.softAPgetStationNum());
  }
  // 璁剧疆Web鏈嶅姟鍣ㄨ矾鐢?
  // delay(10);
}