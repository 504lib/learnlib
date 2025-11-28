#include <Arduino.h>
#include "./protocol/protocol.hpp"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "vehicle/Vehicle.hpp"
#include "StationRepo/StationRepo.hpp"
#include "NetworkClient/NetworkClient.hpp"
#include "RouterScheduler/RouterScheduler.hpp"

const char* ssid            = "黑A123456"; // AP 鍚嶇О
const char* password        = "12345678"; // AP 瀵嗙爜
const char* station_server  = "http://192.168.4.1";

String target_ssid                  = "ESP32-Access-Point"; // AP 鍚嶇О
String target_password              = "12345678"; // AP 瀵嗙爜
String target_station_server        = "http://192.168.4.1";

QueueHandle_t xUartRxQueue;
QueueHandle_t xPassengerUpdateQueue;
QueueHandle_t xCommandQueue;

EventGroupHandle_t evt;

StationRepo station_repo;
NetworkClient network_client;
Vehicle_t vehicle_data("黑A123456", String(ssid), String(password), String(station_server), Rounter::Route_1, VehicleStatus::STATUS_SCANNING);
Vehicle_Info vehicle(vehicle_data);
RouterScheduler router_scheduler(station_repo,network_client,vehicle);


#define UART_ACK_REQUIRED (1 << 0u)

#define LED_Pin LED_BUILTIN

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

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);

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


// #define N   50
// class Router_List
// {
// private:
//     uint8_t SSID_Num;
//     uint8_t current_index = 0;
//     uint8_t used_num = 0;
//     const Rounter router = Rounter::Route_1;
//     String ssid            = "黑A123456"; // AP 鍚嶇О
//     String password        = ""; // AP 瀵嗙爜
//     String station_server  = "http://192.168.4.1";
//     struct Station_t  // 移除 typedef，C++中不需要
//     {
//         String name;
//         String name_ch;
//         bool isProcessed;
//         String ssid;
//         String password;
//         String ip;
//         int8_t lastRSSI;
//         uint32_t lastVisitTime;
//         int visitCount;
//         bool isConnnectd;
//     };
//     enum class Bus_status 
//     { 
//         STATUS_SCANNING,
//         STATUS_IDLE,
//         STATUS_GROPE,
//         STATUS_CONNECTING,
//         STATUS_CONNECTED,
//         STAUS_DISCONNECTED,
//         STATUS_WAITING,
//         STATUS_ARRIVING,
//         STATUS_LEAVING
//     };
//     Bus_status bus_status;
//     Bus_status last_bus_status;
//     Station_t station_list[N];
    
//     int8_t find_Station(String target_name)
//     {
//         for (uint8_t i = 0; i < this->used_num; i++)  // 修复：应该是 i < used_num，不是 used_num-1
//         {
//             if (strcmp(this->station_list[i].name.c_str(), target_name.c_str()) == 0)  // 修复：添加 == 0 比较
//             {
//                 return i;
//             }
//         }
//         return -1;
//     }
    
//     int8_t find_Station_Chinese(String target_name_ch)  // 修复：拼写错误
//     {
//         for (uint8_t i = 0; i < this->used_num; i++)  // 修复：应该是 i < used_num
//         {
//             if (strcmp(this->station_list[i].name_ch.c_str(), target_name_ch.c_str()) == 0)  // 修复：添加 == 0 比较
//             {
//                 return i;
//             }
//         }
//         return -1;
//     }
//     int8_t RSSI_intesify(String ssid)
//     {
//         if(SSID_Num <= 0) return -1;  // 修复：返回-1表示无效
        
//         int8_t bestRSSI = -100;
//         bool found = false;
        
//         for (uint8_t i = 0; i < SSID_Num && i < WiFi.scanComplete(); i++)  // 添加边界检查
//         {
//             if (WiFi.SSID(i) == ssid)
//             {
//                 int8_t rssi = WiFi.RSSI(i);
//                 if (rssi > bestRSSI) 
//                 {
//                     bestRSSI = rssi;
//                     found = true;
//                 }
//             }
//         }
//         return found ? bestRSSI : -1;
//     }
//     float CalculateStationScore(uint8_t index) 
//     {
//         Station_t& station = station_list[index];
//         float score = 0.0f;
        
//         // 规则1: 下一目标站点最高优先级
//         uint8_t nextTarget = (current_index + 1) % used_num;
//         if (index == nextTarget) {
//             score += 50.0f;
//         }
        
//         // 规则2: 信号强度权重
//         int8_t currentRSSI = RSSI_intesify(station.ssid);
//         if (currentRSSI > -50) score += 30.0f;
//         else if (currentRSSI > -70) score += 20.0f;
//         else if (currentRSSI > -80) score += 10.0f;
//         else score += 5.0f;
        
//         // 规则3: 未访问过的站点加分
//         if (!station.isProcessed) 
//         {
//             score += 15.0f;
//         }
        
//         // 规则4: 长时间未访问的站点加分
//         unsigned long timeSinceLastVisit = millis() - station.lastVisitTime;
//         if (timeSinceLastVisit > 300000)  // 5分钟
//         {
//             score += 10.0f;
//         }
        
//         // 规则5: 访问次数少的站点加分（避免总是连接同一个）
//         score += (10.0f - min(station.visitCount, 10));
        
//         return score;
//     }
//     int FindBestStation() 
//     {
//         if (used_num == 0 || SSID_Num == 0) return -1;
        
//         int bestIndex = -1;
//         float bestScore = -1000.0f;
        
//         for (uint8_t i = 0; i < used_num; i++) 
//         {
//             float score = CalculateStationScore(i);
            
//             if (score > bestScore) 
//             {
//                 bestScore = score;
//                 bestIndex = i;
//             }
//         }
//         if (bestIndex != -1) 
//         {
//             Serial.printf("🏆 最佳站点: %s (得分: %.2f)\n", 
//                          station_list[bestIndex].name_ch.c_str(), bestScore);
//         }
        
//         return bestIndex;
//     }
//     bool findBestWiFi()
//     {
//         static uint32_t lastTick = millis();
//         int8_t netNum = WiFi.scanNetworks();
//         switch (netNum)
//         {
//         case WIFI_SCAN_RUNNING:
//             if (millis() - lastTick > 3000)
//             {
//                 Serial.printf("正在寻找ssid\n");
//                 lastTick = millis();
//             }
//             return false;
//         case WIFI_SCAN_FAILED:
//             if (millis() - lastTick > 3000)
//             {
//                 Serial.printf("寻找ssid失败\n");
//                 lastTick = millis();
//             }
//             return false;
//         default:
//             Serial.printf("周围目前有%d个WiFi\n",netNum);
//             SSID_Num = netNum;
//             return true;
//         }
//     }
//     String Get_Status_Str(Bus_status bus_status)
//     {
//         switch (bus_status)
//         {
//         case Bus_status::STATUS_SCANNING:
//             return "SCANNING";
//         case Bus_status::STATUS_IDLE:
//             return "IDLE";
//         case Bus_status::STATUS_GROPE:
//             return "GROPE";
//         case Bus_status::STATUS_CONNECTING:
//             return "CONNECTING";
//         case Bus_status::STATUS_CONNECTED:
//             return "CONNECTED";
//         case Bus_status::STAUS_DISCONNECTED:
//             return "DISCONNECTED";
//         case Bus_status::STATUS_WAITING:
//             return "waiting";
//         case Bus_status::STATUS_ARRIVING:
//             return "arriving";
//         case Bus_status::STATUS_LEAVING:
//             return "leaving";
//         default:
//             return "UNKNOWN";
//         }
//     }
//     void CheckArrivingAndMaybeLeave()
//     {
//         // 确保有当前站点并且已连接 WiFi
//         if (used_num == 0 || current_index >= used_num)
//         {
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             return;
//         }
//         if (WiFi.status() != WL_CONNECTED)
//         {
//             Serial.println("CheckArriving: WiFi 未连接，跳过检查");
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             return;
//         }

//         // 构建 /api/info URL
//         String url = station_list[current_index].ip;
//         if (!url.startsWith("http://") && !url.startsWith("https://")) url = String("http://") + url;
//         if (!url.endsWith("/")) url += "/";
//         url += "api/info";

//         Serial.printf("CheckArriving GET: %s\n", url.c_str());

//         HTTPClient http;
//         if (!http.begin(url))
//         {
//             Serial.println("CheckArriving: http.begin 失败");
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             return;
//         }

//         int httpCode = http.GET();
//         if (httpCode != 200)
//         {
//             Serial.printf("CheckArriving: GET 失败，code=%d\n", httpCode);
//             http.end();
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             return;
//         }

//         String payload = http.getString();
//         http.end();

//         // 解析 JSON（根据你的 /api/info 输出，关注 passenger_list）
//         JsonDocument doc;
//         auto err = deserializeJson(doc, payload);
//         if (err)
//         {
//             Serial.printf("CheckArriving: JSON 解析失败: %s\n", err.c_str());
//             Change_Status_Tip(Bus_status::STATUS_SCANNING) ;
//             return;
//         }

//         if (!doc["passenger_list"].is<JsonArray>())
//         {
//             Serial.println("CheckArriving: JSON 中无 passenger_list 字段");
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             return;
//         }

//         JsonArray passenger_arr = doc["passenger_list"].as<JsonArray>();
//         int routeIndex = static_cast<int>(router);
//         if (routeIndex < 0 || routeIndex >= (int)passenger_arr.size())
//         {
//             Serial.printf("CheckArriving: routeIndex(%d) 超出 passenger_list 大小(%u)\n", routeIndex, (unsigned)passenger_arr.size());
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             return;
//         }

//         int pnum = passenger_arr[routeIndex];
//         Serial.printf("CheckArriving: route %d passenger_num = %d\n", routeIndex, pnum);

//         if (pnum == 0)
//         {
//             Serial.printf("CheckArriving: route %d 无乘客，切换到 LEAVING\n", routeIndex);
//             Change_Status_Tip(Bus_status::STATUS_LEAVING);
//             return;
//         }
//         Change_Status_Tip(Bus_status::STATUS_ARRIVING);
//     }
// public:
//      Router_List(Rounter rounter,const String& ssid,const String& pwd,const String& station_server)
//         : router(rounter), ssid(ssid), password(pwd), station_server(station_server)
//     {
//         SSID_Num = 0;
//         used_num = 0;
//         current_index = 0;
//         bus_status = Bus_status::STATUS_SCANNING;
//     }
//     void Change_Status_Tip(Bus_status bus_status)
//     {
//         last_bus_status = this->bus_status;
//         this->bus_status = bus_status;
//         if(last_bus_status == bus_status) return;
//         Serial.printf("状态变更: %s -> %s\n", Get_Status_Str(last_bus_status), Get_Status_Str(bus_status));
//     }
//     void Change_Status_Tip(VehicleStatus status)
//     {
//         if (status == VehicleStatus::STATUS_ARRIVING)
//         {
//             Change_Status_Tip(Bus_status::STATUS_ARRIVING);
//         }
//         else if (status == VehicleStatus::STATUS_LEAVING)
//         {
//             Change_Status_Tip(Bus_status::STATUS_LEAVING);
//         }
//         else if (status == VehicleStatus::STATUS_WAITING)
//         {
//             Change_Status_Tip(Bus_status::STATUS_WAITING);
//         }
        
//     }   
//     void Scheduler()
//     {
//         static int bestIndex = -1;
//         switch (bus_status)
//         {
//         case Bus_status::STATUS_SCANNING:
//         {
//             bool isFind = findBestWiFi();
//             if(isFind) Change_Status_Tip(Bus_status::STATUS_GROPE);
//             break;
//         }
//         case Bus_status::STATUS_GROPE:
//         {
            
//             bestIndex = FindBestStation();
//             if(bestIndex != -1) bus_status = Bus_status::STATUS_CONNECTING;
//             else Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             break;
//         }
//         case Bus_status::STATUS_CONNECTING:
//         {
//             bool isConnected = Connect_To_Station(bestIndex);
//             if(isConnected) Change_Status_Tip(Bus_status::STATUS_CONNECTED);
//             else Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             break;
//         }
//         case Bus_status::STATUS_CONNECTED:
//         {
//             if(WiFi.status() != WL_CONNECTED)
//             {
//                 Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//                 break;
//             }
//             if (last_bus_status == bus_status)
//             {
//                 break;
//             }
//             bool success = false;
//             for (uint8_t i = 0; i < 5 && !success; i++)
//             {
//                 Serial.printf("正在发送等待状态报告，尝试次数 %d\n", i + 1);
//                 success = sendSinglePost(current_index,router,ssid,Bus_status::STATUS_ARRIVING);
//                 delay(1000);
//             }
//             if(!success) Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//             else Change_Status_Tip(Bus_status::STATUS_ARRIVING);
//             break;
//         }
//         case Bus_status::STAUS_DISCONNECTED:
//         {
//             WiFi.disconnect();
//             Change_Status_Tip(Bus_status::STATUS_SCANNING);
//             break;
//         }
//         case Bus_status::STATUS_IDLE:
//             // 空闲状态下的逻辑
//         {

//             break;
//         }
//         case Bus_status::STATUS_WAITING:
//         {
//             if(WiFi.status() != WL_CONNECTED)
//             {
//                 Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//                 break;
//             }
//             if (last_bus_status == bus_status)
//             {
//                 break;
//             }
//             bool success = false;
//             for (uint8_t i = 0; i < 5 && !success; i++)
//             {
//                 Serial.printf("正在发送等待状态报告，尝试次数 %d\n", i + 1);
//                 success = sendSinglePost(current_index,router,ssid,Bus_status::STATUS_WAITING);
//                 if(success)
//                 {
//                     Serial.printf("发送等待状态报告成功\n");
//                 } 
//                 delay(100);
//             }
//             if(!success) Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//             else Change_Status_Tip(Bus_status::STATUS_WAITING);
//             break;
//         }
//         case Bus_status::STATUS_ARRIVING:
//         {
//             if(WiFi.status() != WL_CONNECTED)
//             {
//                 Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//                 break;
//             }
//             if (last_bus_status == bus_status)
//             {
//                 break;
//             }
//             if (WiFi.status() != WL_CONNECTED)
//             {
//                 Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//                 break;
//             }
//             Serial.printf("等待指令中...\n");
//             CheckArrivingAndMaybeLeave();
//             break;
//         }
//         case Bus_status::STATUS_LEAVING:
//         {
//             if(WiFi.status() != WL_CONNECTED)
//             {
//                 Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//                 break;
//             }
//             if (last_bus_status == bus_status)
//             {
//                 break;
//             }
//             bool success = false;
//             for (uint8_t i = 0; i < 5 && !success; i++)
//             {
//                 Serial.printf("正在发送等待状态Leaving报告，尝试次数 %d\n", i + 1);
//                 success = sendSinglePost(current_index,router,ssid,Bus_status::STATUS_LEAVING);
//                 if(success)
//                 {
//                     Serial.printf("发送离开状态报告成功\n");
//                     break;
//                 } 
//                 delay(100);
//             }
//             if(success)
//             {
//                 station_list[current_index].isProcessed = true;
//                 Move_To_Next_Station();
//             }
//             Change_Status_Tip(Bus_status::STAUS_DISCONNECTED);
//             break;
//         }
//         default:
//             break;
//         }
//     }
//     bool Connect_To_Station(uint8_t index)
//     {
//         if(index >= used_num)   return false;
//         Serial.printf("正在连接:%s (%s)\n",station_list[index].name_ch.c_str(),station_list[index].ssid.c_str());
//         WiFi.disconnect();
//         delay(100);
//         WiFi.begin(station_list[index].ssid,station_list[index].password);
//         uint32_t startTick = millis();
//         while (WiFi.status() != WL_CONNECTED && millis() - startTick < 10000)
//         {
//             Serial.print(".");
//             delay(500);
//         }
//         if (WiFi.status() == WL_CONNECTED)
//         {
//             current_index = index;
//             station_list[index].isConnnectd = true;
//             station_list[index].lastVisitTime = millis();
//             station_list[index].lastRSSI = WiFi.RSSI();
//             station_list[index].visitCount++;
//             Serial.printf("成功连接到站点\nssid:%s,RSSI:%d",station_list[index].ssid.c_str(),station_list[index].lastRSSI);
//             return true;
//         }
//         else
//         {
//             Serial.printf("连接失败\nssid:%s,RSSI:%d",station_list[index].ssid.c_str(),station_list[index].lastRSSI);
//             return false;
//         }
//     }
//     bool sendSinglePost(uint8_t index,Rounter rounter,const String& plate, Bus_status bus_status) 
//     {
//         if (WiFi.status() != WL_CONNECTED) 
//         {
//             Serial.printf("WiFi 未连接，无法发送数据\n");
//             return false;
//         }
        
//         HTTPClient http;
//         String url = station_list[index].ip + "/api/vehicle_report";
//         http.begin(url);
//         http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
//         String postData = "route=" + String(static_cast<uint8_t>(rounter)) + 
//                          "&plate=" +  plate + 
//                          "&status=" + Get_Status_Str(bus_status);
        
//         int httpCode = http.POST(postData);
//         bool success = (httpCode == 200);
//         // Serial.printf("httpCode:%d",httpCode);
//         http.end();
//         return success;
//     }
//     void Disconnect_CurWiFi()
//     {
//         if (used_num > 0 && current_index < used_num)
//         {
//             station_list[current_index].isConnnectd = false;
//         }
//         WiFi.disconnect();
//     }
//     void Add_Station_to_Tail(String name, String name_ch,String ssid,String pwd,String ip)  // 修复：拼写 Tail
//     {
//         if (this->used_num >= N)
//         {
//             Serial.printf("The station Number is overflow\n");
//             return;
//         }
//         if (this->find_Station(name) != -1 || this->find_Station_Chinese(name_ch) != -1)
//         {
//             Serial.printf("Input Station Name is same as station of the list, Please check\n");
//             return;
//         }
//         this->station_list[this->used_num].name = name;
//         this->station_list[this->used_num].name_ch = name_ch;
//         this->station_list[this->used_num].ssid = ssid;
//         this->station_list[this->used_num].password = pwd;
//         this->station_list[this->used_num].ip = ip;
//         this->station_list[this->used_num].isProcessed = false;
//         this->used_num++;
//     }
    
//     void Add_Station_ByIndex(String name, String name_ch,String ssid,String pwd,String ip, uint8_t index)
//     {
//         if (index >= N || this->used_num >= N)  // 修复：index是uint8_t，不会<0
//         {
//             return;
//         }
        
//         // 如果index大于当前数量，则添加到末尾
//         if (index > this->used_num) {
//             Add_Station_to_Tail(name, name_ch, ssid, pwd, ip);
//             return;
//         }
        
//         // 移动元素腾出空间
//         for (uint8_t i = this->used_num; i > index; i--)
//         {
//             this->station_list[i] = this->station_list[i-1];
//         }
        
//         // 插入新元素
//         this->station_list[index].name = name;
//         this->station_list[index].name_ch = name_ch;
//         this->station_list[index].isProcessed = false;
//         this->used_num++;
//     }
    
//     void Add_StationToLater_ByString(String name, String name_ch,String ssid,String pwd,String ip, String target_name, String target_name_ch)
//     {
//         if (this->used_num >= N)  // 修复：移除index检查，因为index还没定义
//         {
//             return;
//         }
        
//         int8_t index = this->find_Station(target_name);
//         int8_t index_ch = this->find_Station_Chinese(target_name_ch);  // 修复：使用target_name_ch
        
//         if (index != -1 && index_ch != -1 && index == index_ch)
//         {
//             this->Add_Station_ByIndex(name, name_ch,ssid,pwd,ip, index + 1);  // 修复：添加右括号，插入到目标后面
//         }
//     }
    
//     void Remove_Station_Head()
//     {
//         this->Remove_Station_ByIndex(0);
//     }
    
//     void Remove_Station_Tail()  // 修复：拼写Tail
//     {
//         if (!this->used_num) return;
//         this->used_num--; 
//     }
    
//     void Remove_Station_ByIndex(uint8_t index)
//     {
//         if(!this->used_num || index >= this->used_num) return;
        
//         for (uint8_t i = index; i < this->used_num - 1; i++)  // 修复：边界条件
//         {
//             this->station_list[i] = this->station_list[i + 1];
//         }
//         this->used_num--;
//     }
    
//     void Remove_Station_ByString(String name, String name_ch)
//     {
//         int8_t temp = this->find_Station(name);
//         if (temp == -1)
//         {
//             temp = this->find_Station_Chinese(name_ch);
//         }
        
//         if (temp == -1)
//         {
//             Serial.printf("Remove fail");
//             return;
//         }
//         else
//         {
//             this->Remove_Station_ByIndex(temp);
//             return;
//         }
//     }
    
//     String Get_Current_Station()
//     {
//         if (this->used_num == 0) return String("");  // 修复：使用current_index而不是未定义的i
//         return this->station_list[this->current_index].name;
//     }
    
//     // 添加一些有用的方法
//     String Get_Current_Station_Chinese()
//     {
//         if (this->used_num == 0) return String("");
//         return this->station_list[this->current_index].name_ch;
//     }
    
//     void Move_To_Next_Station()
//     {
//         if (this->used_num == 0) return;
//         this->current_index = (this->current_index + 1) % this->used_num;
//     }
    
//     void Move_To_Previous_Station()
//     {
//         if (this->used_num == 0) return;
//         this->current_index = (this->current_index == 0) ? this->used_num - 1 : this->current_index - 1;
//     }
    
//     uint8_t Get_Station_Count()
//     {
//         return this->used_num;
//     }
    
//     bool Is_Empty()
//     {
//         return this->used_num == 0;
//     }
    
//     void Mark_Current_Processed()
//     {
//         if (this->used_num > 0)
//         {
//             this->station_list[this->current_index].isProcessed = true;
//         }
//     }
    
//     bool Is_Current_Processed()
//     {
//         if (this->used_num == 0) return false;
//         return this->station_list[this->current_index].isProcessed;
//     }
//     void Change_Station_Password(uint8_t index, String new_password)
//     {
//         if (index >= this->used_num) return;
//         this->station_list[index].password = new_password;
//     }
//     void Print_Station_List()
//     {
//         Serial.println("当前站点列表:");
//         for (uint8_t i = 0; i < this->used_num; i++)
//         {
//             Serial.printf("%d: %s (%s) - 已处理: %s\n", 
//                           i, 
//                           this->station_list[i].name_ch.c_str(), 
//                           this->station_list[i].ssid.c_str(),
//                           this->station_list[i].isProcessed ? "是" : "否");
//         }
//     }
//     void Clear_Station_List()
//     {
//         this->used_num = 0;
//         this->current_index = 0;
//     }
//     void Reset_Processing_Status()
//     {
//         for (uint8_t i = 0; i < this->used_num; i++)
//         {
//             this->station_list[i].isProcessed = false;
//         }
//     }
// };


// Router_List router_list(Rounter::Route_1,String(ssid),String(password),String(station_server));
void Bus_scheduler_Task(void* pvParameters)
{
    Station_t station1("normal_univercity","师范学院",target_ssid,target_password,target_station_server);
    station_repo.Add_Station_to_Tail(station1);
    for(;;)
    {
        router_scheduler.RouterScheduler_Executer();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void setup() 
{
    
    evt = xEventGroupCreate();
    uart_protocol.setAckCallback([](){
        if (evt == nullptr) return;
        xEventGroupSetBits(evt,UART_ACK_REQUIRED);
    });
    uart_protocol.setVehicleStatusCallback([](VehicleStatus status){
        vehicle.Update_Vehicle_Status(status);
    });
    xUartRxQueue = xQueueCreate(32,sizeof(uint8_t)); 
    xCommandQueue = xQueueCreate(32,sizeof(ACK_Queue_t));
    xTaskCreate(Rx_Task,"rx_task",2048,NULL,3,NULL);
    xTaskCreate(Serial_Task,"serial_task",1024,NULL,2,NULL);
    xTaskCreate(ACK_Task,"ack_task",2048,NULL,2,NULL); 
    xTaskCreate(Bus_scheduler_Task,"bus_schedulet_task",8192,NULL,2,NULL); 

    network_client.startWiFiAP(String(ssid),String(password));
    network_client.addWebRoute("/",[](AsyncWebServerRequest *request){
      String html = R"rawliteral(
          <!DOCTYPE html>
          <html>
          <head>
              <title>Station List</title>
              <style>
                  body {
                      font-family: Arial, sans-serif;
                      text-align: center;
                      margin: 0;
                      padding: 0;
                      background-color: #f9f9f9;
                  }
                  h1 {
                      margin: 20px 0;
                  }
                  table {
                      margin: 20px auto;
                      border-collapse: collapse;
                      width: 90%;
                      max-width: 1200px;
                      background-color: #fff;
                      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
                  }
                  th, td {
                      border: 1px solid #ddd;
                      padding: 10px;
                      text-align: center;
                  }
                  th {
                      background-color: #f2f2f2;
                      font-weight: bold;
                  }
                  tr:nth-child(even) {
                      background-color: #f9f9f9;
                  }
                  tr:hover {
                      background-color: #f1f1f1;
                  }
                  .status-true {
                      color: green;
                      font-weight: bold;
                  }
                  .status-false {
                      color: red;
                      font-weight: bold;
                  }
              </style>
          </head>
          <body>
              <h1>Station List</h1>
              <table>
                  <thead>
                      <tr>
                          <th>Name</th>
                          <th>Chinese Name</th>
                          <th>SSID</th>
                          <th>IP</th>
                          <th>Processed</th>
                          <th>Connected</th>
                      </tr>
                  </thead>
                  <tbody id="stationTable">
                      <!-- Dynamic content will be inserted here -->
                  </tbody>
              </table>
              <script>
                  // Function to fetch data from /api/info and update the table
                  async function fetchStationData() {
                      try {
                          const response = await fetch('/api/info');
                          const data = await response.json();
                          const table = document.getElementById('stationTable');
                          table.innerHTML = ''; // Clear existing table rows

                          // Populate table with station data
                          data.station_list.forEach(station => {
                              const row = document.createElement('tr');
                              row.innerHTML = `
                                  <td>${station.name}</td>
                                  <td>${station.name_ch}</td>
                                  <td>${station.ssid}</td>
                                  <td>${station.ip}</td>
                                  <td class="${station.isProcessed === 'true' ? 'status-true' : 'status-false'}">
                                      ${station.isProcessed === 'true' ? 'Yes' : 'No'}
                                  </td>
                                  <td class="${station.isConnected === 'true' ? 'status-true' : 'status-false'}">
                                      ${station.isConnected === 'true' ? 'Yes' : 'No'}
                                  </td>
                              `;
                              table.appendChild(row);
                          });
                      } catch (error) {
                          console.error('Error fetching station data:', error);
                      }
                  }

                  // Fetch data every 2 seconds
                  setInterval(fetchStationData, 2000);

                  // Initial fetch
                  fetchStationData();
              </script>
          </body>
          </html>
        )rawliteral";
        request->send(200, "text/plain", html);
    });
    network_client.addWebRoute("/api/info",[&](AsyncWebServerRequest *request){
        request->send(200, "application/json", station_repo.Get_StationList_JSON());
    });
    network_client.beginWebServer();
}

void loop() 
{
    static uint32_t lastTestTick = 0;
    static bool LED_State = true;
    if (millis() - lastTestTick > 200) // 每60秒运行一次测试
    {
        digitalWrite(LED_Pin,LED_State); // 打开LED表示测试开始
        LED_State = !LED_State;
        lastTestTick = millis();
    }        
}