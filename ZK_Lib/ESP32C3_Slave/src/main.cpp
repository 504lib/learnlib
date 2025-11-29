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
        case CmdType::VEHICLE_STATUS:
          uart_protocol.Set_Vehicle_Status(static_cast<VehicleStatus>(ack_queue_t.value.int_value));
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


void Bus_scheduler_Task(void* pvParameters)
{
    Station_t station1("normal_univercity","师范学院",target_ssid,target_password,target_station_server);
    station_repo.Add_Station_to_Tail(station1);
    station_repo.Add_Station_to_Tail(Station_t("central_hospital","中心医院","test","",target_station_server));   
    for(;;)
    {
        router_scheduler.RouterScheduler_Executer();
        vTaskDelay(100 / portTICK_PERIOD_MS);
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
        Serial.printf("Received Vehicle Status Change Request: %d\n", static_cast<uint8_t>(status));
      if (status != VehicleStatus::STATUS_ARRIVING && status != VehicleStatus::STATUS_LEAVING && status != VehicleStatus::STATUS_WAITING)
      {
        Serial.printf("No permission to change status !!!\n");
        return;
      }
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
        <html lang="zh-CN">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>监控系统</title>
            <style>
                :root {
                    --primary-color: #3498db;
                    --primary-dark: #2980b9;
                    --light-gray: #f5f7fa;
                    --medium-gray: #e4e7ed;
                    --dark-gray: #7f8c8d;
                    --white: #ffffff;
                    --success-color: #2ecc71;
                    --warning-color: #f39c12;
                    --danger-color: #e74c3c;
                }
                
                * {
                    margin: 0;
                    padding: 0;
                    box-sizing: border-box;
                }
                
                body {
                    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                    background: linear-gradient(135deg, var(--light-gray) 0%, var(--white) 100%);
                    color: #333;
                    line-height: 1.6;
                    min-height: 100vh;
                    padding: 20px;
                }
                
                .container {
                    max-width: 1200px;
                    margin: 0 auto;
                }
                
                .dashboard-header {
                    display: grid;
                    grid-template-columns: 2fr 1fr 1fr 1fr;
                    gap: 20px;
                    margin-bottom: 30px;
                }
                
                .card {
                    background: var(--white);
                    border-radius: 10px;
                    padding: 25px;
                    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.08);
                    transition: transform 0.3s ease, box-shadow 0.3s ease;
                    animation: fadeInUp 0.8s ease-out;
                }
                
                .card:hover {
                    transform: translateY(-5px);
                    box-shadow: 0 6px 20px rgba(0, 0, 0, 0.12);
                }
                
                .vehicle-card {
                    display: flex;
                    flex-direction: column;
                    justify-content: space-between;
                }
                
                .vehicle-plate {
                    font-size: 2.5rem;
                    font-weight: bold;
                    color: var(--primary-color);
                    margin-bottom: 10px;
                    text-shadow: 1px 1px 3px rgba(0, 0, 0, 0.1);
                }
                
                .vehicle-details {
                    display: flex;
                    justify-content: space-between;
                    align-items: flex-end;
                }
                
                .vehicle-route {
                    font-size: 1.2rem;
                    color: var(--dark-gray);
                }
                
                .vehicle-status {
                    font-size: 1rem;
                    color: var(--success-color);
                    font-weight: bold;
                }
                
                .status-card {
                    display: flex;
                    flex-direction: column;
                    text-align: center;
                    justify-content: center;
                }
                
                .status-card h3 {
                    color: var(--dark-gray);
                    margin-bottom: 15px;
                    font-size: 1rem;
                }
                
                .status-value {
                    font-size: 2rem;
                    font-weight: bold;
                }
                
                .target-station {
                    color: var(--primary-color);
                }
                
                .current-status {
                    color: var(--success-color);
                }
                
                .station-count {
                    color: var(--warning-color);
                }
                
                .table-container {
                    background: var(--white);
                    border-radius: 10px;
                    overflow: hidden;
                    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
                    animation: fadeIn 1s ease-out;
                }
                
                table {
                    width: 100%;
                    border-collapse: collapse;
                }
                
                thead {
                    background: linear-gradient(135deg, var(--primary-color) 0%, var(--primary-dark) 100%);
                    color: var(--white);
                }
                
                th {
                    padding: 15px;
                    text-align: left;
                    font-weight: 600;
                    letter-spacing: 0.5px;
                }
                
                tbody tr {
                    border-bottom: 1px solid var(--medium-gray);
                    transition: background-color 0.3s ease;
                }
                
                tbody tr:hover {
                    background-color: rgba(52, 152, 219, 0.05);
                }
                
                tbody tr:nth-child(even) {
                    background-color: var(--light-gray);
                }
                
                tbody tr:nth-child(even):hover {
                    background-color: rgba(52, 152, 219, 0.08);
                }
                
                td {
                    padding: 12px 15px;
                }
                
                .status-indicator {
                    display: inline-block;
                    width: 10px;
                    height: 10px;
                    border-radius: 50%;
                    margin-right: 8px;
                }
                
                .status-true {
                    color: var(--success-color);
                }
                
                .status-false {
                    color: var(--danger-color);
                }
                
                .status-indicator.true {
                    background-color: var(--success-color);
                }
                
                .status-indicator.false {
                    background-color: var(--danger-color);
                }
                
                .refresh-info {
                    text-align: center;
                    margin-top: 20px;
                    color: var(--dark-gray);
                    font-size: 0.9rem;
                }
                
                .pulse {
                    display: inline-block;
                    width: 10px;
                    height: 10px;
                    border-radius: 50%;
                    background: var(--success-color);
                    margin-right: 5px;
                    animation: pulse 2s infinite;
                }
                
                @keyframes fadeIn {
                    from { opacity: 0; }
                    to { opacity: 1; }
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
                        transform: scale(0.95);
                        box-shadow: 0 0 0 0 rgba(46, 204, 113, 0.7);
                    }
                    
                    70% {
                        transform: scale(1);
                        box-shadow: 0 0 0 10px rgba(46, 204, 113, 0);
                    }
                    
                    100% {
                        transform: scale(0.95);
                        box-shadow: 0 0 0 0 rgba(46, 204, 113, 0);
                    }
                }
                
                @media (max-width: 1024px) {
                    .dashboard-header {
                        grid-template-columns: 1fr 1fr;
                        grid-template-rows: auto auto;
                    }
                    
                    .vehicle-card {
                        grid-column: 1 / span 2;
                    }
                }
                
                @media (max-width: 768px) {
                    .dashboard-header {
                        grid-template-columns: 1fr;
                        grid-template-rows: auto auto auto auto;
                    }
                    
                    .vehicle-card {
                        grid-column: 1;
                    }
                    
                    table {
                        display: block;
                        overflow-x: auto;
                    }
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="dashboard-header">
                    <div class="card vehicle-card">
                        <div class="vehicle-plate" id="vehiclePlate">京A12345</div>
                        <div class="vehicle-details">
                            <div class="vehicle-route">路线: <span id="vehicleRoute">1</span></div>
                            <div class="vehicle-status" id="vehicleStatus">运行中</div>
                        </div>
                    </div>
                    
                    <div class="card status-card">
                        <h3>当前目标站点</h3>
                        <div id="targetStation" class="status-value target-station">Station A</div>
                    </div>
                    
                    <div class="card status-card">
                        <h3>站点状态</h3>
                        <div id="currentStatus" class="status-value current-status">正常</div>
                    </div>
                    
                    <div class="card status-card">
                        <h3>站点总数</h3>
                        <div id="stationCount" class="status-value station-count">5</div>
                    </div>
                </div>
                
                <div class="table-container">
                    <table>
                        <thead>
                            <tr>
                                <th>站点名称</th>
                                <th>中文名称</th>
                                <th>SSID</th>
                                <th>IP地址</th>
                                <th>处理状态</th>
                                <th>连接状态</th>
                            </tr>
                        </thead>
                        <tbody id="stationTable">
                            <tr>
                                <td>Station A</td>
                                <td>站点A</td>
                                <td>AP_Station_A</td>
                                <td>192.168.1.10</td>
                                <td class="status-true">
                                    <span class="status-indicator true"></span>
                                    已处理
                                </td>
                                <td class="status-true">
                                    <span class="status-indicator true"></span>
                                    已连接
                                </td>
                            </tr>
                            <tr>
                                <td>Station B</td>
                                <td>站点B</td>
                                <td>AP_Station_B</td>
                                <td>192.168.1.11</td>
                                <td class="status-true">
                                    <span class="status-indicator true"></span>
                                    已处理
                                </td>
                                <td class="status-false">
                                    <span class="status-indicator false"></span>
                                    未连接
                                </td>
                            </tr>
                            <tr>
                                <td>Station C</td>
                                <td>站点C</td>
                                <td>AP_Station_C</td>
                                <td>192.168.1.12</td>
                                <td class="status-false">
                                    <span class="status-indicator false"></span>
                                    未处理
                                </td>
                                <td class="status-true">
                                    <span class="status-indicator true"></span>
                                    已连接
                                </td>
                            </tr>
                            <tr>
                                <td>Station D</td>
                                <td>站点D</td>
                                <td>AP_Station_D</td>
                                <td>192.168.1.13</td>
                                <td class="status-false">
                                    <span class="status-indicator false"></span>
                                    未处理
                                </td>
                                <td class="status-false">
                                    <span class="status-indicator false"></span>
                                    未连接
                                </td>
                            </tr>
                            <tr>
                                <td>Station E</td>
                                <td>站点E</td>
                                <td>AP_Station_E</td>
                                <td>192.168.1.14</td>
                                <td class="status-true">
                                    <span class="status-indicator true"></span>
                                    已处理
                                </td>
                                <td class="status-true">
                                    <span class="status-indicator true"></span>
                                    已连接
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
                
                <div class="refresh-info">
                    <span class="pulse"></span>数据每2秒自动更新
                </div>
            </div>

            <script>

                // 更新页面数据
                function updateStationData(data) {
                    // 更新车辆信息
                    const vehicleInfo = data.vehicle_info;
                    document.getElementById('vehiclePlate').textContent = vehicleInfo.Plate;
                    document.getElementById('vehicleRoute').textContent = vehicleInfo.Router;
                    document.getElementById('vehicleStatus').textContent = vehicleInfo.Status;
                    
                    // 更新站点状态卡片
                    const stationRepo = data.station_repo;
                    document.getElementById('targetStation').textContent = stationRepo.target_station;
                    document.getElementById('currentStatus').textContent = 
                        stationRepo.current_sta_status === "true" ? "已前往" : "正在前往";
                    document.getElementById('stationCount').textContent = stationRepo.station_list.length;
                    
                    // 更新站点表格
                    const table = document.getElementById('stationTable');
                    table.innerHTML = ''; // 清空现有表格行
                    
                    // 填充表格数据
                    stationRepo.station_list.forEach(station => {
                        const row = document.createElement('tr');
                        row.innerHTML = `
                            <td>${station.name}</td>
                            <td>${station.name_ch}</td>
                            <td>${station.ssid}</td>
                            <td>${station.ip}</td>
                            <td class="${station.isProcessed === 'true' ? 'status-true' : 'status-false'}">
                                <span class="status-indicator ${station.isProcessed === 'true' ? 'true' : 'false'}"></span>
                                ${station.isProcessed === 'true' ? '已处理' : '未处理'}
                            </td>
                            <td class="${station.isConnected === 'true' ? 'status-true' : 'status-false'}">
                                <span class="status-indicator ${station.isConnected === 'true' ? 'true' : 'false'}"></span>
                                ${station.isConnected === 'true' ? '已连接' : '未连接'}
                            </td>
                        `;
                        table.appendChild(row);
                    });
                }

                // 模拟从API获取数据
                async function fetchStationData() {
                    try {
                        // 在实际应用中，这里应该是：
                        const response = await fetch('/api/info');
                        const data = await response.json();
                        
                        // 模拟API调用延迟
                        // await new Promise(resolve => setTimeout(resolve, 500));
                        
                        // 使用模拟数据
                        updateStationData(data);
                    } catch (error) {
                        console.error('获取站点数据时出错:', error);
                    }
                }

                // 每2秒获取一次数据
                setInterval(fetchStationData, 2000);

                // 初始加载
                fetchStationData();
            </script>
        </body>
        </html>


        )rawliteral";
        request->send(200, "text/html", html);
    });
    network_client.addWebRoute("/api/info",[&](AsyncWebServerRequest *request){
        request->send(200, "application/json", router_scheduler.Get_RouterInfo_JSON());
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