#include <Arduino.h>
#include "./protocol/protocol.hpp"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
// #include "
#define AP_MODE 1 
const char* ssid = "ESP32-Access-Point"; // AP 鍚嶇О
const char* password = "12345678"; // AP 瀵嗙爜
const char* station_server = "http://192.168.4.1";

#define LED_Pin GPIO_NUM_48
AsyncWebServer server(80);
static int count = 0;
static float fcount = 0.0;
uint32_t lastPrint = 0;
bool Flag_INT = false;
bool Flag_FLOAT = false;
bool Flag_ACK = false;
uint8_t passenger_num = 0;
String station_ch = "normal university";
String station_name = "师范学院";

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);
void set_passenger(uint8_t value)
{
  passenger_num = value;
}

void clear_passenger()
{
  passenger_num = 0;
}
// void IRAM_ATTR handleInterrupt1() {
//   Flag_INT = true;
// }

// void IRAM_ATTR handleInterrupt2() {
  
//   Flag_FLOAT = true;
// }


// void IRAM_ATTR handleInterrupt3() {
//   Flag_ACK = true;
// }
#if AP_MDOE != 1
void getStationPassengerData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        // 构建请求URL
        String url = String(station_server) + "/api/info";
        
        Serial.print("📡 请求站点数据: ");
        Serial.println(url);
        
        http.begin(url);
        
        int httpCode = http.GET();
        
        if (httpCode == 200) {
            String payload = http.getString();
            Serial.print("✅ 收到站点响应: ");
            Serial.println(payload);
            
            // 解析JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                int station_passengers = doc["passengers"];
                String station_name = doc["station"];
                
                Serial.print("🏠 ");
                Serial.print(station_name);
                Serial.print(" - 候车人数: ");
                Serial.println(station_passengers);
                
                // 可以通过串口发送给STM32
                // uart_protocol.Send_Uart_Frame_PASSENGER_NUM(station_passengers);
                
            } else {
                Serial.println("❌ JSON解析失败");
            }
            
        } else {
            Serial.print("❌ HTTP请求失败: ");
            Serial.println(httpCode);
        }
        
        http.end();
    } else {
        Serial.println("❌ WiFi未连接，无法获取站点数据");
    }
}
#endif

void setup() 
{
  
  pinMode(LED_Pin,OUTPUT);
  // pinMode(11, INPUT); // 璁剧疆涓鸿緭鍏?
  // pinMode(12, INPUT); // 璁剧疆涓鸿緭鍏?
  // pinMode(13, INPUT); // 璁剧疆涓鸿緭鍏?
  // attachInterrupt(11, handleInterrupt1, RISING); // 涓婂崌娌胯Е鍙?
  // attachInterrupt(12, handleInterrupt2, RISING); // 涓婂崌娌胯Е鍙?
  // attachInterrupt(13, handleInterrupt3, RISING); // 涓婂崌娌胯Е鍙?
  uart_protocol.setPassengerNumCallback(set_passenger);
  uart_protocol.setClearCallback(clear_passenger);
  Serial.println("正在启动 AP 模式...");
  
  // 启动 AP
  #if AP_MODE == 1
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
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>设备监控</title>
    <meta charset="UTF-8">
     <style>
        body { 
            font-family: Arial, sans-serif; 
            max-width: 800px; 
            margin: 0 auto; 
            padding: 20px; 
            background: #f5f5f5;
        }
        .card { 
            background: white; 
            padding: 20px; 
            margin: 15px 0; 
            border-radius: 10px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .value { 
            font-size: 2em; 
            color: #2196F3; 
            font-weight: bold;
        }
        .header { 
            color: #666; 
            margin-bottom: 10px;
        }
        .station-header {
            text-align: center;
            color: #333;
            margin-bottom: 30px;
        }
    </style>
</head>
<body>
    <!-- 修正的站点名称显示 -->
    <h1 class="station-header" id="station">--</h1>
    
    <div class="card">
        <div class="header">乘客数量</div>
        <div class="value" id="passengers">--</div>
    </div>
    
    <div class="card">
        <div class="header">连接状态</div>
        <div id="connection">--</div>
    </div>

    <script>
        function updateData() {
            fetch('/api/info')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('station').textContent = data.station;
                    document.getElementById('passengers').textContent = data.passengers;
                    document.getElementById('connection').textContent = `IP: ${data.ip}, 设备: ${data.clients}`;
                });
        }
        setInterval(updateData, 3000);
        updateData();
    </script>
</body>
</html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"station_ch\":\"" + station_ch + "\",";
    json += "\"station\":\"" + station_name + "\",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"ssid\":\"" + String(ssid) + "\",";
    json += "\"passengers\":" + String(passenger_num);
    json += "}";
    request->send(200, "application/json", json);
  });

  // 启动服务器
  server.begin();
  Serial.println("HTTP 服务器已启动");

  #else 
    WiFi.begin(ssid,password);
    uint16_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ 成功连接到站点AP!");
        Serial.print("IP地址: ");
        Serial.println(WiFi.localIP());
        Serial.print("站点服务器: ");
    } else {
        Serial.println("\n❌ 连接站点AP失败!");
        return;
    }
  #endif
}

void loop() 
{
  if(Serial1.available())
  {
    uart_protocol.Receive_Uart_Frame(Serial1.read());
    // Serial.write(Serial1.read());
  }
  #if AP_MODE != 1
  static uint32_t last_tick = 0;
  static bool LED_ON = true;
  if(WiFi.status() == WL_CONNECTED)
  {
    if (millis() - last_tick > 3000)
    {
      getStationPassengerData();
      digitalWrite(LED_Pin,LED_ON);
      LED_ON ^= 1;
      last_tick = millis();
    }
    
  }
  else
  {
    if (millis() - last_tick > 250)
    {
      digitalWrite(LED_Pin,LED_ON);
      LED_ON ^= 1;
      last_tick = millis();
    }
  }
  #else
  // if (Flag_INT)
  // {
  //   uart_protocol.Send_Uart_Frame(count++);
  //   // Serial.println(count++);
  //   Flag_INT = false;
  // }
  // if (Flag_FLOAT)
  // {
  //   uart_protocol.Send_Uart_Frame(fcount);
  //  fcount += 0.1;
  //   // Serial.println(fcount, 1);
  //   Flag_FLOAT = false;
  // }
  // if (Flag_ACK)
  // {
  //   uart_protocol.Send_Uart_Frame_ACK();
  //   // Serial.println("ACK");
  //   Flag_ACK = false;
  // }
  if (millis() - lastPrint > 3000) {  // 姣?10绉掓墦鍗颁竴娆?
    lastPrint = millis();
    // Serial.print("已连接从机数量 ");
    // Serial.println(WiFi.softAPgetStationNum());
    uart_protocol.Send_Uart_Frame_PASSENGER_NUM(passenger_num);
  }
  #endif
  // 璁剧疆Web鏈嶅姟鍣ㄨ矾鐢?
  // delay(10);
}