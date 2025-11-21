#include <Arduino.h>
#include "./protocol/protocol.hpp"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


typedef struct
{
  CmdType type;
  union
  {
    int32_t int_value;
    float   float_value;
     struct
    {
      Rounter router;
      uint8_t passenger_num;
    }passenger; 
  }value;
}ACK_Queue_t;

uint8_t passenger[static_cast<uint8_t>(Rounter::Ring_road) + 1] = {0};


EventGroupHandle_t evt;

#define UART_ACK_REQUIRED (1 << 0u)

#define LED_Pin LED_BUILTIN
#define AP_MODE 1 

const char* ssid            = "ESP32-Access-Point"; // AP 鍚嶇О
const char* password        = "12345678"; // AP 瀵嗙爜
const char* station_server  = "http://192.168.4.1";

AsyncWebServer server(80);
static int count            = 0;
static float fcount         = 0.0;
uint32_t lastPrint          = 0;
bool Flag_INT               = false;
bool Flag_FLOAT             = false;
bool Flag_ACK               = false;
uint8_t passenger_num       = 0;
String station_ch           = "normal university";
String station_name         = "师范学院";

QueueHandle_t xUartRxQueue;
QueueHandle_t xPassengerUpdateQueue;
QueueHandle_t xCommandQueue;

// 车辆状态枚举
enum class VehicleStatus 
{
  WAITING,     // 候车中
  ARRIVING,     // 即将进站
  LEAVING      // 离站
};

// 车辆信息结构
struct VehicleInfo 
{
  Rounter route;
  String plate;
  VehicleStatus status;
  uint32_t timestamp;  // 添加时间戳，用于清理过期数据
};

// 车辆信息数组，限制为10个
#define MAX_VEHICLES 10
VehicleInfo vehicles[MAX_VEHICLES];
uint8_t vehicleCount = 0;


protocol uart_protocol(0xAA,0x55,0x0D,0x0A);
void set_passenger(Rounter rounter,uint8_t value)
{
  Serial.printf("Receive Rounter:%d,value:%d\n",static_cast<uint8_t>(rounter),value);
  passenger[static_cast<uint8_t>(rounter)] = value;
  uint8_t sum = 0;
  for (size_t i = 0; i < sizeof(passenger)/sizeof(passenger[0]) ;i++)
  {
    sum += passenger[i];
  }
  passenger_num = sum;
  
}

void set_ACK()
{
  if (evt != NULL)
  {
    xEventGroupSetBits(evt,UART_ACK_REQUIRED);
  }
}

void clear_passenger(Rounter rounter)
{
  Serial.printf("Receive Rounter:%d\n",static_cast<uint8_t>(rounter));
  passenger[static_cast<uint8_t>(rounter)] = 0;
  uint8_t sum = 0;
  for (size_t i = 0; i < sizeof(passenger)/sizeof(passenger[0]) ;i++)
  {
    sum += passenger[i];
  }
  passenger_num = sum;
}

void VehicleCleanupTask(void* pvParameters) {
    for(;;) {
        // 每30秒清理一次过期数据（超过5分钟）
        uint32_t currentTime = millis();
        uint32_t fiveMinutes = 5 * 60 * 1000;
        
        for (int i = 0; i < vehicleCount; i++) {
            if (currentTime - vehicles[i].timestamp > fiveMinutes) {
                // 移除过期车辆，将后面的元素前移
                for (int j = i; j < vehicleCount - 1; j++) {
                    vehicles[j] = vehicles[j + 1];
                }
                vehicleCount--;
                i--; // 重新检查当前位置
            }
        }
        
        vTaskDelay(30000 / portTICK_PERIOD_MS); // 30秒
    }
}

void Rx_Task(void* pvParameters)
{
  uint8_t rx_byte = 0;
  for(;;)
  { 
    if (xQueueReceive(xUartRxQueue,&rx_byte,portMAX_DELAY))
    {
       uart_protocol.Receive_Uart_Frame(rx_byte);
      //  Serial.println("rx_byte:" + rx_byte);
    }
  }
}

void Serial_Task(void* p)
{
  uint8_t Serial_Rx_byte = 0;
  for(;;)
  {
    if (Serial1.available())
    {
      Serial_Rx_byte = Serial1.read();
      xQueueSend(xUartRxQueue,&Serial_Rx_byte,portMAX_DELAY);
    }
    vTaskDelay(5 / portTICK_PERIOD_MS); 
  }
}

void ACK_Task(void* pvParameters)
{
  ACK_Queue_t ack_queue_t;
  uint32_t flags = 0;
  for(;;)
  {
    if (xQueueReceive(xCommandQueue,&ack_queue_t,portMAX_DELAY))
    {
      bool ACK_required = true;
      const TickType_t ack_timeout = pdMS_TO_TICKS(200); // 200 ms
      for (uint8_t i = 0; i < 3 && ACK_required; i++)
      {
        switch (ack_queue_t.type)
        {
        case CmdType::INT:
          uart_protocol.Send_Uart_Frame(ack_queue_t.value.int_value);
          break;
        case CmdType::FLOAT:
          uart_protocol.Send_Uart_Frame(ack_queue_t.value.float_value);
          break;
        case CmdType::PASSENGER_NUM:
          uart_protocol.Send_Uart_Frame_PASSENGER_NUM(ack_queue_t.value.passenger.router,ack_queue_t.value.passenger.passenger_num);
          break;
        case CmdType::CLEAR:
          uart_protocol.Send_Uart_Frame_CLEAR(ack_queue_t.value.passenger.router);
          break;
        default:
          Serial.printf("unknown type!!!");
          break;
        }
        flags = xEventGroupWaitBits(evt,UART_ACK_REQUIRED,pdTRUE,pdFALSE,ack_timeout);
        if(flags & UART_ACK_REQUIRED)
        {
          ACK_required = false;
          Serial.printf("Recived ACK for %d",static_cast<uint8_t>(ack_queue_t.type));
        }
        else
        {
          Serial.printf("time out!!! (%d/3)",i+1);
          if (i >= 2)
          {
            Serial.printf("time out:%d",static_cast<uint8_t>(ack_queue_t.type));
          }
        }
        
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

#if AP_MODE == 1
void AP_Task(void* pvParameters)
{
  uint8_t pass_num = 0;
  for(;;)
  {
    pass_num = 0;
    for (size_t i = 0;i < sizeof(passenger)/sizeof(passenger[0]);i++)
    {
      pass_num += passenger[i];
    }
    passenger_num = pass_num; 
    // if (xQueueReceive(xPassengerUpdateQueue,&pass_num,portMAX_DELAY))
    // {
    //   uart_protocol.Send_Uart_Frame_PASSENGER_NUM(Rounter::Route_1,pass_num);
    // }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// void PassengerTask(void* p)
// {
//   for(;;)
//   {
//     xQueueSend(xPassengerUpdateQueue,&passenger_num,1000 / portTICK_PERIOD_MS);
//     vTaskDelay(3000 / portTICK_PERIOD_MS);
//   }
// }
#else 
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
void STA_Task(void* pvParameters)
{
  
  static uint32_t last_tick = 0;
  static bool LED_ON = true;
  for(;;)
  {
    if(WiFi.status() == WL_CONNECTED)
    {
      if (xTaskGetTickCount() - last_tick > 3000)
      {
        getStationPassengerData();
        digitalWrite(LED_Pin,LED_ON);
        LED_ON ^= 1;
        last_tick = xTaskGetTickCount();
      }
    
    }
    else
    {
      if (xTaskGetTickCount() - last_tick > 250)
      {
        digitalWrite(LED_Pin,LED_ON);
        LED_ON ^= 1;
        last_tick = xTaskGetTickCount();
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
#endif

void setup() 
{
  
  pinMode(LED_Pin,OUTPUT);
  uart_protocol.setAckCallback(set_ACK);
  uart_protocol.setPassengerNumCallback(set_passenger);
  uart_protocol.setClearCallback(clear_passenger);
  Serial.println("正在启动 AP 模式...");
  
  // 启动 AP
  #if AP_MODE == 1
  if (WiFi.softAP(ssid, password,1,0,10,false)) {
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
 // 在 setup() 函数中找到 HTML 部分，替换为以下代码：

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="zh-CN">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>多线路公交站台监控系统</title>
        <style>
            * {
                margin: 0;
                padding: 0;
                box-sizing: border-box;
                font-family: 'Microsoft YaHei', Arial, sans-serif;
            }
            
            body {
                background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
                min-height: 100vh;
                padding: 20px;
                color: #333;
            }
            
            .container {
                max-width: 1200px;
                margin: 0 auto;
            }
            
            /* 标题栏样式 */
            .header {
                background-color: #5a6c7d;
                color: white;
                height: 40px;
                line-height: 30px;
                padding: 5px 15px;
                border-radius: 8px 8px 0 0;
                box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
                font-size: 18px;
                font-weight: 600;
                margin-bottom: 0;
                animation: fadeInDown 0.8s ease;
                display: flex;
                justify-content: space-between;
                align-items: center;
            }
            
            .header-title {
                font-size: 18px;
            }
            
            .header-buttons {
                display: flex;
                gap: 10px;
            }
            
            .header-btn {
                background: rgba(255, 255, 255, 0.2);
                color: white;
                border: none;
                padding: 5px 15px;
                border-radius: 4px;
                cursor: pointer;
                transition: all 0.3s ease;
                font-size: 14px;
            }
            
            .header-btn:hover {
                background: rgba(255, 255, 255, 0.3);
                transform: translateY(-2px);
            }
            
            /* 主内容区域 */
            .main-content {
                display: flex;
                gap: 20px;
                margin-bottom: 20px;
            }
            
            /* 左侧信息面板 */
            .station-info {
                flex: 1;
                background: white;
                border-radius: 0 0 8px 8px;
                padding: 20px;
                box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
                animation: fadeInLeft 0.8s ease;
            }
            
            .station-name {
                font-size: 22px;
                font-weight: bold;
                color: #2c3e50;
                margin-bottom: 15px;
                display: flex;
                align-items: center;
            }
            
            .station-name::before {
                content: "📍";
                margin-right: 10px;
                font-size: 24px;
            }
            
            .passenger-count {
                background: linear-gradient(135deg, #3498db, #2980b9);
                color: white;
                padding: 20px;
                border-radius: 10px;
                text-align: center;
                margin-top: 15px;
                box-shadow: 0 5px 15px rgba(52, 152, 219, 0.3);
                transition: all 0.3s ease;
                animation: pulse 2s infinite;
            }
            
            .passenger-count:hover {
                transform: translateY(-5px);
                box-shadow: 0 8px 20px rgba(52, 152, 219, 0.4);
            }
            
            .passenger-label {
                font-size: 16px;
                margin-bottom: 10px;
            }
            
            .passenger-value {
                font-size: 42px;
                font-weight: bold;
            }
            
            /* 右侧车辆状态卡片 */
            .vehicle-status {
                width: 300px;
                background: white;
                border-radius: 0 0 8px 8px;
                padding: 20px;
                box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
                animation: fadeInRight 0.8s ease;
            }
            
            .status-title {
                font-size: 18px;
                font-weight: bold;
                color: #2c3e50;
                margin-bottom: 15px;
                padding-bottom: 8px;
                border-bottom: 2px solid #ecf0f1;
            }
            
            .status-card {
                background: #f8f9fa;
                border-radius: 10px;
                padding: 15px;
                margin-bottom: 15px;
                border-left: 4px solid #3498db;
                transition: all 0.3s ease;
            }
            
            .status-card:hover {
                transform: translateX(5px);
                box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
            }
            
            .vehicle-id {
                font-size: 18px;
                font-weight: bold;
                color: #2c3e50;
                margin-bottom: 5px;
            }
            
            .status-line {
                display: flex;
                justify-content: space-between;
                align-items: center;
            }
            
            .status-text {
                color: #7f8c8d;
                font-size: 14px;
            }
            
            .status-indicator {
                display: flex;
                align-items: center;
            }
            
            .indicator {
                width: 12px;
                height: 12px;
                border-radius: 50%;
                margin-right: 8px;
                animation: blink 2s infinite;
            }
            
            .online {
                background-color: #2ecc71;
            }
            
            .offline {
                background-color: #e74c3c;
            }
            
            /* 线路详情区域 */
            .routes-section {
                background: white;
                border-radius: 8px;
                padding: 20px;
                box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
                animation: fadeInUp 0.8s ease;
            }
            
            .section-title {
                font-size: 20px;
                font-weight: bold;
                color: #2c3e50;
                margin-bottom: 15px;
                padding-bottom: 10px;
                border-bottom: 2px solid #ecf0f1;
            }
            
            .routes-grid {
                display: grid;
                grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
                gap: 15px;
            }
            
            .route-card {
                background: #f8f9fa;
                border-radius: 8px;
                padding: 15px;
                text-align: center;
                transition: all 0.3s ease;
                border-top: 4px solid #3498db;
            }
            
            .route-card:hover {
                transform: translateY(-5px);
                box-shadow: 0 8px 15px rgba(0, 0, 0, 0.1);
            }
            
            .route-name {
                font-size: 16px;
                font-weight: bold;
                color: #2c3e50;
                margin-bottom: 10px;
            }
            
            .route-count {
                font-size: 24px;
                font-weight: bold;
                color: #3498db;
                transition: all 0.5s ease;
            }
            
            /* 弹窗样式 */
            .modal {
                display: none;
                position: fixed;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                background-color: rgba(0, 0, 0, 0.5);
                z-index: 1000;
                justify-content: center;
                align-items: center;
                animation: fadeIn 0.3s ease;
            }
            
            .modal-content {
                background: white;
                border-radius: 10px;
                padding: 30px;
                width: 90%;
                max-width: 500px;
                box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
                animation: zoomIn 0.3s ease;
                position: relative;
            }
            
            .modal-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 20px;
                padding-bottom: 15px;
                border-bottom: 1px solid #eee;
            }
            
            .modal-title {
                font-size: 22px;
                font-weight: bold;
                color: #2c3e50;
            }
            
            .close-btn {
                background: none;
                border: none;
                font-size: 24px;
                cursor: pointer;
                color: #7f8c8d;
                transition: color 0.3s;
            }
            
            .close-btn:hover {
                color: #e74c3c;
            }
            
            .modal-body {
                line-height: 1.6;
                color: #555;
            }
            
            .contact-info {
                margin-top: 20px;
            }
            
            .contact-item {
                display: flex;
                align-items: center;
                margin-bottom: 10px;
            }
            
            .contact-icon {
                margin-right: 10px;
                font-size: 18px;
            }
            
            .developer-options {
                margin-top: 20px;
            }
            
            .option-item {
                margin-bottom: 15px;
            }
            
            .option-label {
                display: block;
                margin-bottom: 5px;
                font-weight: bold;
            }
            
            .option-input {
                width: 100%;
                padding: 8px 12px;
                border: 1px solid #ddd;
                border-radius: 4px;
            }
            
            .option-btn {
                background: #3498db;
                color: white;
                border: none;
                padding: 10px 15px;
                border-radius: 4px;
                cursor: pointer;
                transition: background 0.3s;
            }
            
            .option-btn:hover {
                background: #2980b9;
            }
            
            /* 加载状态 */
            .loading {
                opacity: 0.7;
                pointer-events: none;
            }
            
            .loading::after {
                content: " (加载中...)";
                color: #3498db;
                font-weight: normal;
            }
            .vehicle-card {
                background: #f8f9fa;
                border-radius: 10px;
                padding: 15px;
                margin-bottom: 15px;
                border-left: 4px solid #3498db;
                transition: all 0.3s ease;
            }

            .vehicle-card:hover {
                transform: translateX(5px);
                box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
            }

            .vehicle-card.arriving {
                border-left-color: #e74c3c;
                background: #fff5f5;
            }

            .vehicle-card.waiting {
                border-left-color: #f39c12;
                background: #fffbf0;
            }

            .vehicle-plate {
                font-size: 18px;
                font-weight: bold;
                color: #2c3e50;
                margin-bottom: 8px;
                display: flex;
                align-items: center;
            }

            .vehicle-plate::before {
                content: "🚌";
                margin-right: 8px;
                font-size: 16px;
            }

            .vehicle-info {
                display: flex;
                justify-content: space-between;
                align-items: center;
                font-size: 14px;
            }

            .vehicle-route {
                color: #3498db;
                font-weight: bold;
            }

            .vehicle-status-badge {
                padding: 4px 8px;
                border-radius: 12px;
                font-size: 12px;
                font-weight: bold;
            }

            .status-arriving {
                background: #e74c3c;
                color: white;
            }

            .status-waiting {
                background: #f39c12;
                color: white;
            }
            /* 动画定义 */
            @keyframes fadeInDown {
                from {
                    opacity: 0;
                    transform: translateY(-20px);
                }
                to {
                    opacity: 1;
                    transform: translateY(0);
                }
            }
            
            @keyframes fadeInLeft {
                from {
                    opacity: 0;
                    transform: translateX(-20px);
                }
                to {
                    opacity: 1;
                    transform: translateX(0);
                }
            }
            
            @keyframes fadeInRight {
                from {
                    opacity: 0;
                    transform: translateX(20px);
                }
                to {
                    opacity: 1;
                    transform: translateX(0);
                }
            }
            
            @keyframes fadeInUp {
                from {
                    opacity: 0;
                    transform: translateY(20px);
                }
                to {
                    opacity: 1;
                    transform: translateY(0);
                }
            }
            
            @keyframes pulse {
                0% {
                    box-shadow: 0 5px 15px rgba(52, 152, 219, 0.3);
                }
                50% {
                    box-shadow: 0 5px 20px rgba(52, 152, 219, 0.6);
                }
                100% {
                    box-shadow: 0 5px 15px rgba(52, 152, 219, 0.3);
                }
            }
            
            @keyframes blink {
                0%, 100% {
                    opacity: 1;
                }
                50% {
                    opacity: 0.5;
                }
            }
            
            @keyframes fadeIn {
                from {
                    opacity: 0;
                }
                to {
                    opacity: 1;
                }
            }
            
            @keyframes zoomIn {
                from {
                    opacity: 0;
                    transform: scale(0.8);
                }
                to {
                    opacity: 1;
                    transform: scale(1);
                }
            }
            
            /* 响应式设计 */
            @media (max-width: 768px) {
                .main-content {
                    flex-direction: column;
                }
                
                .vehicle-status {
                    width: 100%;
                }
                
                .routes-grid {
                    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
                }
                
                .header {
                    flex-direction: column;
                    height: auto;
                    padding: 10px;
                }
                
                .header-title {
                    margin-bottom: 10px;
                }
            }
            
            /* 数据更新时间显示 */
            .update-time {
                text-align: center;
                color: #7f8c8d;
                font-size: 14px;
                margin-top: 20px;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <!-- 标题栏 -->
            <div class="header">
                <div class="header-title">多线路公交站台监控</div>
                <div class="header-buttons">
                    <button class="header-btn" id="homeBtn">首页</button>
                    <button class="header-btn" id="contactBtn">联系我们</button>
                    <button class="header-btn" id="devBtn">开发者模式</button>
                </div>
            </div>
            
            <!-- 主内容区域 -->
            <div class="main-content">
                <!-- 左侧站台信息 -->
                <div class="station-info">
                    <div class="station-name">
                        当前所在站名：<span id="stationName">--</span>
                    </div>
                    
                    <div class="passenger-count">
                        <div class="passenger-label">总乘客数量</div>
                        <div class="passenger-value" id="totalPassengers">--</div>
                    </div>
                </div>
                
                <!-- 右侧车辆状态 -->
                <div class="vehicle-status">
                    <meta charset="UTF-8">
                    <div class="status-title">车辆状态</div>
                    <div id="vehiclesContainer">
                        <!-- 车辆信息将通过JavaScript动态生成 -->
                    </div>
                    <div id="noVehicles" style="text-align: center; color: #7f8c8d; padding: 20px; display: none;">
                        暂无车辆信息
                    </div>
                </div>
            </div>
            
            <!-- 线路详情区域 -->
            <div class="routes-section">
                <div class="section-title">各线路乘客分布</div>
                <div class="routes-grid" id="routesContainer">
                    <!-- 线路卡片将通过JavaScript动态生成 -->
                </div>
            </div>
            
            <!-- 数据更新时间 -->
            <div class="update-time">
                最后更新: <span id="lastUpdateTime">--</span>
            </div>
        </div>

        <!-- 首页弹窗 -->
        <div class="modal" id="homeModal">
            <div class="modal-content">
                <div class="modal-header">
                    <h2 class="modal-title">系统首页</h2>
                    <button class="close-btn">&times;</button>
                </div>
                <div class="modal-body">
                    <p>欢迎使用多线路公交站台监控系统！</p>
                    <p>该系统实时监控公交站台各线路的乘客数量，帮助优化公交调度。</p>
                    <p>当前版本：v2.1.0</p>
                    <p>系统状态：<span style="color: #2ecc71;">正常运行</span></p>
                </div>
            </div>
        </div>

        <!-- 联系我们弹窗 -->
        <div class="modal" id="contactModal">
            <div class="modal-content">
                <div class="modal-header">
                    <h2 class="modal-title">联系我们</h2>
                    <button class="close-btn">&times;</button>
                </div>
                <div class="modal-body">
                    <p>如果您有任何问题或建议，请通过以下方式联系我们：</p>
                    <div class="contact-info">
                        <div class="contact-item">
                            <span class="contact-icon">📧</span>
                            <span>邮箱：support@busmonitor.com</span>
                        </div>
                        <div class="contact-item">
                            <span class="contact-icon">📞</span>
                            <span>电话：400-123-4567</span>
                        </div>
                        <div class="contact-item">
                            <span class="contact-icon">📍</span>
                            <span>地址：XX市XX区XX路XX号</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <!-- 开发者模式弹窗 -->
        <div class="modal" id="devModal">
            <div class="modal-content">
                <div class="modal-header">
                    <h2 class="modal-title">开发者模式</h2>
                    <button class="close-btn">&times;</button>
                </div>
                <div class="modal-body">
                    <p>警告：此功能仅供开发人员使用！</p>
                    <div class="developer-options">
                        <div class="option-item">
                            <label class="option-label">API端点：</label>
                            <input type="text" class="option-input" value="/api/info">
                        </div>
                        <div class="option-item">
                            <label class="option-label">数据刷新间隔（秒）：</label>
                            <input type="number" class="option-input" value="3" min="1" max="60">
                        </div>
                        <div class="option-item">
                            <label class="option-label">调试模式：</label>
                            <input type="checkbox" id="debugMode">
                            <label for="debugMode">启用</label>
                        </div>
                        <button class="option-btn">保存设置</button>
                    </div>
                </div>
            </div>
        </div>

        <script>
        const routeEnumMapping = {
                0: "路线1",
                1: "路线2", 
                2: "路线3",
                3: "环线"
            };

            // 状态映射
        const statusMapping = {
            "waiting": "候车中",
            "arriving": "即将进站"
        };
        async function fetchData() {
                try {
                    // 显示加载状态
                    // document.getElementById('stationName').classList.add('loading');
                    
                    // 发送请求到API
                    const response = await fetch('/api/info');
                    
                    if (!response.ok) {
                        throw new Error(`HTTP错误! 状态: ${response.status}`);
                    }
                    
                    // 解析JSON数据
                    const data = await response.json();
                    
                    // 移除加载状态
                    // document.getElementById('stationName').classList.remove('loading');
                    
                    return data;
                } catch (error) {
                    console.error('获取数据失败:', error);
                    
                    // 移除加载状态
                    document.getElementById('stationName').classList.remove('loading');
                    
                    // 返回一个包含错误信息的默认数据结构
                    return {
                        "station": "数据获取失败",
                        "station_ch": "error",
                        "passengers_total": 0,
                        "ip": "未知",
                        "clients": 0,
                        "ssid": "未知",
                        "passenger_list": [0, 0, 0, 0, 0],
                        "route_names": ["路线1", "路线2", "路线3", "路线4", "环线"],
                        "vehicles": []
                    };
                }
            }
            
            // 更新页面数据
            async function updatePageData() {
                try {
                    const data = await fetchData();
                    
                    // 更新站名
                    document.getElementById('stationName').textContent = data.station;
                    
                    // 更新总乘客数量
                    const totalElement = document.getElementById('totalPassengers');
                    const currentTotal = parseInt(totalElement.textContent) || 0;
                    const newTotal = data.passengers_total;
                    
                    // 添加数字变化动画
                    if (currentTotal !== newTotal) {
                        totalElement.style.color = '#e74c3c';
                        setTimeout(() => {
                            totalElement.style.color = 'white';
                        }, 500);
                    }
                    
                    totalElement.textContent = newTotal;
                    
                    // 更新线路详情
                    updateRoutes(data.passenger_list, data.route_names);
                    
                    // 更新车辆状态（使用真实数据）
                    updateVehicleStatus(data.vehicles || []);
                    
                    // 更新时间
                    document.getElementById('lastUpdateTime').textContent = new Date().toLocaleString();
                } catch (error) {
                    console.error('更新页面数据失败:', error);
                    document.getElementById('stationName').textContent = '数据更新失败';
                }
            }
            
            // 更新线路详情
            function updateRoutes(passengerList, routeNames) {
                const container = document.getElementById('routesContainer');
                container.innerHTML = '';
                
                // 检查是否所有路线乘客数量都为0
                const allZero = passengerList.every(count => count === 0);
                if (allZero) {
                    container.innerHTML = '<div style="text-align: center; color: #7f8c8d; grid-column: 1 / -1; padding: 20px;">当前所有路线均无乘客</div>';
                    return;
                }
                
                passengerList.forEach((count, index) => {
                    // 如果乘客数量为0，则不显示该路线
                    if (count === 0) return;
                    
                    const routeCard = document.createElement('div');
                    routeCard.className = 'route-card';
                    
                    // 使用服务器提供的路线名称，如果没有则使用默认名称
                    const routeName = routeNames && routeNames[index] ? routeNames[index] : `线路 ${index + 1}`;
                    
                    routeCard.innerHTML = `
                        <div class="route-name">${routeName}</div>
                        <div class="route-count">${count}</div>
                        <div style="font-size: 14px; color: #7f8c8d;">人</div>
                    `;
                    
                    container.appendChild(routeCard);
                });
            }
            
            // 更新车辆状态 - 使用真实数据
            function updateVehicleStatus(vehicles) {
                const container = document.getElementById('vehiclesContainer');
                const noVehicles = document.getElementById('noVehicles');
                
                // 清空容器
                container.innerHTML = '';
                
                // 如果没有车辆数据，显示提示
                if (!vehicles || vehicles.length === 0) {
                    container.style.display = 'none';
                    noVehicles.style.display = 'block';
                    return;
                }
                
                // 显示车辆数据
                container.style.display = 'block';
                noVehicles.style.display = 'none';
                
                // 按状态排序：即将进站的排在前面
                vehicles.sort((a, b) => {
                    if (a.status === 'arriving' && b.status !== 'arriving') return -1;
                    if (a.status !== 'arriving' && b.status === 'arriving') return 1;
                    return 0;
                });
                
                // 创建车辆卡片
                vehicles.forEach(vehicle => {
                    const vehicleCard = document.createElement('div');
                    vehicleCard.className = `vehicle-card ${vehicle.status}`;
                    
                    // 获取路线名称
                    const routeName = routeEnumMapping[vehicle.route] || `线路 ${vehicle.route + 1}`;
                    // 获取状态文本
                    const statusText = statusMapping[vehicle.status] || vehicle.status;
                    
                    vehicleCard.innerHTML = `
                        <div class="vehicle-plate">${vehicle.plate}</div>
                        <div class="vehicle-info">
                            <span class="vehicle-route">${routeName}</span>
                            <span class="vehicle-status-badge status-${vehicle.status}">${statusText}</span>
                        </div>
                    `;
                    
                    container.appendChild(vehicleCard);
                });
            }
            
            // 弹窗控制功能
            function setupModalControls() {
                // 获取按钮和弹窗元素
                const homeBtn = document.getElementById('homeBtn');
                const contactBtn = document.getElementById('contactBtn');
                const devBtn = document.getElementById('devBtn');
                
                const homeModal = document.getElementById('homeModal');
                const contactModal = document.getElementById('contactModal');
                const devModal = document.getElementById('devModal');
                
                // 获取所有关闭按钮
                const closeBtns = document.querySelectorAll('.close-btn');
                
                // 首页按钮点击事件
                homeBtn.addEventListener('click', function() {
                    homeModal.style.display = 'flex';
                });
                
                // 联系我们按钮点击事件
                contactBtn.addEventListener('click', function() {
                    contactModal.style.display = 'flex';
                });
                
                // 开发者模式按钮点击事件
                devBtn.addEventListener('click', function() {
                    devModal.style.display = 'flex';
                });
                
                // 关闭按钮点击事件
                closeBtns.forEach(btn => {
                    btn.addEventListener('click', function() {
                        homeModal.style.display = 'none';
                        contactModal.style.display = 'none';
                        devModal.style.display = 'none';
                    });
                });
                
                // 点击弹窗外部关闭弹窗
                window.addEventListener('click', function(event) {
                    if (event.target === homeModal) {
                        homeModal.style.display = 'none';
                    }
                    if (event.target === contactModal) {
                        contactModal.style.display = 'none';
                    }
                    if (event.target === devModal) {
                        devModal.style.display = 'none';
                    }
                });
            }
            
            // 初始化页面
            document.addEventListener('DOMContentLoaded', function() {
                updatePageData();
                setupModalControls();
                
                // 每3秒更新一次数据
                setInterval(updatePageData, 3000);
            });
        </script>
    </body>
    </html>    
        )rawliteral";
        request->send(200, "text/html", html);
    });
    // 处理未知请求
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not Found");
    });
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){
      JsonDocument doc;
      doc["station"] = station_name;
      doc["station_ch"] = station_ch;
      doc["passengers_total"] = passenger_num;
      doc["ip"] = WiFi.softAPIP().toString();
      doc["clients"] = WiFi.softAPgetStationNum();
      doc["ssid"] = String(ssid);

      // 乘客数量数组
      JsonArray passenger_arr = doc["passenger_list"].to<JsonArray>();
      for (size_t i = 0; i < (sizeof(passenger)/sizeof(passenger[0])); ++i) {
          passenger_arr.add(passenger[i]);
      }

      // 路线名称映射
      const char* route_names[] = {"路线1", "路线2", "路线3", "环线"};
      JsonArray route_names_arr = doc["route_names"].to<JsonArray>();
      for (size_t i = 0; i < (sizeof(route_names)/sizeof(route_names[0])); ++i) {
          route_names_arr.add(route_names[i]);
      }

      // 车辆信息数组 - 只包含非离站状态的车辆
      JsonArray vehicles_arr = doc["vehicles"].to<JsonArray>();
      for (int i = 0; i < vehicleCount; i++) {
          // 只添加候车中和即将进站的车辆
          if (vehicles[i].status != VehicleStatus::LEAVING) {
              JsonObject vehicle_obj = vehicles_arr.add<JsonObject>();
              vehicle_obj["route"] = static_cast<int>(vehicles[i].route);
              vehicle_obj["plate"] = vehicles[i].plate;
              
              // 状态映射
              String statusStr;
              switch (vehicles[i].status) {
                  case VehicleStatus::WAITING:
                      statusStr = "waiting";
                      break;
                  case VehicleStatus::ARRIVING:
                      statusStr = "arriving";
                      break;
                  case VehicleStatus::LEAVING:
                      statusStr = "leaving"; // 理论上不会执行到这里
                      break;
              }
              vehicle_obj["status"] = statusStr;
              vehicle_obj["timestamp"] = vehicles[i].timestamp;
          }
      }

      String out;
      serializeJson(doc, out);
      request->send(200, "application/json", out);
  });

  server.on("/api/clear",HTTP_GET,[](AsyncWebServerRequest *request){
    Rounter rounter;
    if (!request->hasParam("rounter"))
    {
      request->send(400,"text/plain","invalid input");
      return;
    }
    ACK_Queue_t ack_queue_t;
    String s = request->getParam("rounter")->value();
    const char* str = s.c_str();
    char* endptr = nullptr;
    int32_t v = strtol(str,&endptr,10);
    if(endptr == str || *endptr != '\0')
    {
      request->send(400,"text/plain","invalid rounter num");
      return;
    }
    if (v < 0 || v > static_cast<int>(Rounter::Ring_road))
    {
      request->send(400,"text/plain","invalid rounter");
      return;
    }
    rounter = static_cast<Rounter>(v);
    passenger[v] = 0;
    ack_queue_t.type = CmdType::CLEAR;
    ack_queue_t.value.passenger.router = rounter;
    ack_queue_t.value.passenger.passenger_num = 0;
    xQueueSend(xCommandQueue,&ack_queue_t,0);
    request->send(200,"text/plain","success");
  });
  server.on("/api/vehicle_report", HTTP_POST, [](AsyncWebServerRequest *request){
      // 检查参数
      ACK_Queue_t ack_queue_t;
      if (!request->hasParam("route", true) || 
          !request->hasParam("plate", true) || 
          !request->hasParam("status", true)) {
          request->send(400, "text/plain", "Missing parameters");
          return;
      }

      // 解析路线
      String routeStr = request->getParam("route", true)->value();
      int routeInt = routeStr.toInt();
      if (routeInt < 0 || routeInt > static_cast<int>(Rounter::Ring_road)) {
          request->send(400, "text/plain", "Invalid route");
          return;
      }
      Rounter route = static_cast<Rounter>(routeInt);

      // 解析车牌号
      String plate = request->getParam("plate", true)->value();
      if (plate.length() == 0 || plate.length() > 20) {
          request->send(400, "text/plain", "Invalid plate number");
          return;
      }

      // 解析状态
      String statusStr = request->getParam("status", true)->value();
      VehicleStatus status;
      if (statusStr == "waiting") {
          status = VehicleStatus::WAITING;
      } else if (statusStr == "arriving") {
          status = VehicleStatus::ARRIVING;
      } else if (statusStr == "leaving") {
          status = VehicleStatus::LEAVING;
      } else {
          request->send(400, "text/plain", "Invalid status");
          return;
      }

      // 处理离站状态
    if (status == VehicleStatus::LEAVING) {
          // 从车辆数组中移除该车辆
          bool found = false;
          for (int i = 0; i < vehicleCount; i++) {
              if (vehicles[i].plate == plate) {
                  // 找到车辆，检查先前状态
                  VehicleStatus prev = vehicles[i].status;
                  Serial.println("车辆离站: " + plate + ", 路线: " + String(static_cast<int>(vehicles[i].route)) + ", 先前状态: " + String(static_cast<int>(prev)));
                  
                  // 仅当先前为 WAITING 时，才清除对应路线的人数并发送 CLEAR 命令
                  if (prev == VehicleStatus::WAITING) {
                      ack_queue_t.type = CmdType::CLEAR;
                      ack_queue_t.value.passenger.router = vehicles[i].route;
                      ack_queue_t.value.passenger.passenger_num = 0;
                      xQueueSend(xCommandQueue, &ack_queue_t, 0);
                      passenger[static_cast<uint8_t>(vehicles[i].route)] = 0;
                      Serial.println("已清除该路线乘客数");
                  } else {
                      Serial.println("先前状态非 WAITING，保留乘客数，不发送 CLEAR");
                  }

                  // 将后面的元素前移并移除该车辆记录
                  for (int j = i; j < vehicleCount - 1; j++) {
                      vehicles[j] = vehicles[j + 1];
                  }
                  vehicleCount--;
                  found = true;
                  break;
              }
          }
          
          if (found) {
              request->send(200, "text/plain", "success");
          } else {
              request->send(404, "text/plain", "Vehicle not found");
          }
          return;
      }

      // 处理候车中和即将进站状态（原有逻辑）
      bool found = false;
      for (int i = 0; i < vehicleCount; i++) {
          if (vehicles[i].plate == plate) {
              // 更新现有车辆信息
              vehicles[i].route = route;
              vehicles[i].status = status;
              vehicles[i].timestamp = millis();
              found = true;
              Serial.println("更新车辆: " + plate + ", 路线: " + String(routeInt) + ", 状态: " + statusStr);
              break;
          }
      }

      // 如果没找到，添加新车辆
      if (!found) {
          if (vehicleCount < MAX_VEHICLES) {
              vehicles[vehicleCount].route = route;
              vehicles[vehicleCount].plate = plate;
              vehicles[vehicleCount].status = status;
              vehicles[vehicleCount].timestamp = millis();
              vehicleCount++;
              Serial.println("新增车辆: " + plate + ", 路线: " + String(routeInt) + ", 状态: " + statusStr);
          } else {
              // 数组已满，替换最旧的条目
              uint32_t oldestTime = UINT32_MAX;
              int oldestIndex = 0;
              for (int i = 0; i < MAX_VEHICLES; i++) {
                  if (vehicles[i].timestamp < oldestTime) {
                      oldestTime = vehicles[i].timestamp;
                      oldestIndex = i;
                  }
              }
              vehicles[oldestIndex].route = route;
              vehicles[oldestIndex].plate = plate;
              vehicles[oldestIndex].status = status;
              vehicles[oldestIndex].timestamp = millis();
              Serial.println("替换车辆: " + plate + ", 路线: " + String(routeInt) + ", 状态: " + statusStr);
          }
      }

      request->send(200, "text/plain", "success");
  });

  // 启动服务器
  server.begin();
  Serial.println("HTTP 服务器已启动");
  xPassengerUpdateQueue = xQueueCreate(10,sizeof(uint8_t));
  xTaskCreate(AP_Task,"ap_task",1024,NULL,1,NULL);
  // xTaskCreate(PassengerTask,"passengertask",1024,NULL,1,NULL); 
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
  xTaskCreate(STA_Task,"sta_task",3072,NULL,1,NULL);
  #endif
  xUartRxQueue = xQueueCreate(32,sizeof(uint8_t)); 
  xCommandQueue = xQueueCreate(32,sizeof(ACK_Queue_t));
  evt = xEventGroupCreate();
  xTaskCreate(AP_Task,"ap_task",1024,NULL,1,NULL);
  xTaskCreate(Rx_Task,"rx_task",2048,NULL,3,NULL);
  xTaskCreate(Serial_Task,"serial_task",1024,NULL,2,NULL);
  xTaskCreate(ACK_Task,"ack_task",2048,NULL,2,NULL); 
  xTaskCreate(VehicleCleanupTask, "vehicle_cleanup", 2048, NULL, 1, NULL);
}

void loop() 
{
  digitalWrite(LED_Pin, HIGH);
  delay(500);
  digitalWrite(LED_Pin, LOW);
  delay(500);
  
  static uint32_t counter = 0;
  if (++counter % 20 == 0) {
    // uart_protocol.Send_Uart_Frame((int32_t)counter);
    Serial.printf("🔁 主循环 - 计数: %lu, 内存: %d\n", 
                  counter, ESP.getFreeHeap());
  }
}