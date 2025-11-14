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

#define LED_Pin GPIO_NUM_12
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
        <div class="value" id="passengers_total">--</div>
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
                    document.getElementById('passengers_total').textContent = data.passengers_total;
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

    JsonDocument doc;
    doc["station"] = station_name;
    doc["station_ch"] = station_ch;
    doc["passengers_total"] = passenger_num;
    doc["ip"] = WiFi.softAPIP().toString();
    doc["clients"] = WiFi.softAPgetStationNum();
    doc["ssid"] = String(ssid);

    // 创建数组并填充
    JsonArray arr = doc["passenger_list"].to<JsonArray>();
    for (size_t i = 0; i < (sizeof(passenger)/sizeof(passenger[0])); ++i) {
        arr.add(passenger[i]);
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