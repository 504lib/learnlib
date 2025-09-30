#include <Arduino.h>
#include "./protocol/protocol.hpp"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
// #include "

const char* ssid = "ESP32-Access-Point"; // AP 名称
const char* password = "12345678"; // AP 密码
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
  pinMode(11, INPUT); // 设置为输�?
  pinMode(12, INPUT); // 设置为输�?
  pinMode(13, INPUT); // 设置为输�?
  attachInterrupt(11, handleInterrupt1, RISING); // 上升沿触�?
  attachInterrupt(12, handleInterrupt2, RISING); // 上升沿触�?
  attachInterrupt(13, handleInterrupt3, RISING); // 上升沿触�?
 Serial.println("�������� AP ģʽ...");
  
  // ���� AP
  if (WiFi.softAP(ssid, password)) {
    Serial.println("AP ģʽ�����ɹ�!");
    
    // ��ȡ AP �� IP ��ַ
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP ��ַ: ");
    Serial.println(myIP);
    
    // ��ʾ������Ϣ
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("����: ");
    Serial.println(password);
  } else {
    Serial.println("AP ģʽ����ʧ��!");
    return;
  }

  // ���� Web ������·��
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><title>ESP32-S3 APģʽ</title></head>";
    html += "<body><h1>��ӭʹ��ESP32-S3 APģʽ</h1>";
    html += "<p>IP��ַ: " + WiFi.softAPIP().toString() + "</p>";
    html += "<p>�������豸��: " + String(WiFi.softAPgetStationNum()) + "</p>";
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

  // ����������
  server.begin();
  Serial.println("HTTP ������������");
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
  if (millis() - lastPrint > 10000) {  // �?10秒打印一�?
    lastPrint = millis();
    Serial.print("�����Ӵӻ����� ");
    Serial.println(WiFi.softAPgetStationNum());
  }
  // 设置Web服务器路�?
  // delay(10);
}