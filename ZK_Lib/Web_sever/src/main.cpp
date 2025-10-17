// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <ArduinoJson.h>
// #include <map>
// #include <queue>

// const char* ssid = "MedicineTest";
// const char* password = "12345678";

// AsyncWebServer server(80);

// // 模拟系统状态
// struct SystemStatus {
//     int32_t current_weight = 0;
//     int32_t target_weight = 100;
//     uint8_t system_state = 0; // 0:等待, 1:运行, 2:完成
//     uint8_t medicine_type = 0;
// };

// SystemStatus g_status;

// // 队列系统
// class UserQueue {
// private:
//     std::queue<String> userQueue;
//     std::map<String, bool> userInQueue;
//     String currentUser;
    
// public:
//     bool addUser(const String& userId) {
//         if (userInQueue.find(userId) != userInQueue.end()) {
//             return false; // 用户已在队列中
//         }
        
//         userQueue.push(userId);
//         userInQueue[userId] = true;
//         Serial.printf("用户 %s 加入队列，位置: %d\n", userId.c_str(), userQueue.size());
//         return true;
//     }
    
//     bool removeUser(const String& userId) {
//         if (userInQueue.find(userId) == userInQueue.end()) {
//             return false;
//         }
        
//         // 重建队列（排除要删除的用户）
//         std::queue<String> newQueue;
//         while (!userQueue.empty()) {
//             String user = userQueue.front();
//             userQueue.pop();
//             if (user != userId) {
//                 newQueue.push(user);
//             }
//         }
//         userQueue = newQueue;
//         userInQueue.erase(userId);
        
//         Serial.printf("用户 %s 离开队列\n", userId.c_str());
//         return true;
//     }
    
//     String getNextUser() {
//         if (userQueue.empty()) {
//             currentUser = "";
//             return "";
//         }
        
//         currentUser = userQueue.front();
//         userQueue.pop();
//         userInQueue.erase(currentUser);
        
//         Serial.printf("开始服务用户: %s\n", currentUser.c_str());
//         return currentUser;
//     }
    
//     String getCurrentUser() { return currentUser; }
//     int getQueueCount() { return userQueue.size(); }
    
//     // 获取队列列表（用于显示）
//     std::vector<String> getQueueList() {
//         std::vector<String> list;
//         std::queue<String> temp = userQueue;
        
//         while (!temp.empty()) {
//             list.push_back(temp.front());
//             temp.pop();
//         }
//         return list;
//     }
// };

// UserQueue medicineQueue;

// void Setup_Web_Server() {
//     // 主页面
//     server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//         String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//     <meta charset="UTF-8">
//     <title>中药取药系统 - 最小测试</title>
//     <style>
//         body { font-family: Arial; margin: 20px; background: #f0f8ff; }
//         .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
//         .card { background: #f8f9fa; padding: 15px; margin: 10px 0; border-radius: 8px; border-left: 4px solid #4CAF50; }
//         .btn { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; background: #4CAF50; color: white; }
//         .btn-danger { background: #f44336; }
//         .status { padding: 8px; border-radius: 4px; font-weight: bold; }
//         .status-waiting { background: #fff3cd; color: #856404; }
//         .status-running { background: #d4edda; color: #155724; }
//         .status-complete { background: #d1ecf1; color: #0c5460; }
//         .queue-item { display: flex; justify-content: space-between; padding: 8px; margin: 5px 0; background: white; border-radius: 4px; }
//         .current-user { background: #e8f5e8; font-weight: bold; }
//     </style>
// </head>
// <body>
//     <div class="container">
//         <h1>🧪 中药取药系统 - 最小测试模式</h1>
//         <p><strong>ESP32独立测试 - 不连接STM32</strong></p>
        
//         <div class="card">
//             <h3>📊 系统状态</h3>
//             <div id="systemStatus" class="status status-waiting">等待中...</div>
//             <p>当前重量: <span id="currentWeight">0</span>g / 目标重量: <span id="targetWeight">100</span>g</p>
//             <p>当前用户: <span id="currentUser">无</span></p>
//         </div>
        
//         <div class="card">
//             <h3>📋 取药队列 (<span id="queueCount">0</span>)</h3>
//             <div id="queueList">
//                 <div style="text-align: center; color: #666; padding: 10px;">队列为空</div>
//             </div>
//         </div>
        
//         <div class="card">
//             <h3>🎮 测试控制</h3>
//             <button class="btn" onclick="joinQueue()">加入队列</button>
//             <button class="btn btn-danger" onclick="leaveQueue()">离开队列</button>
//             <button class="btn" onclick="startTaking()">开始取药</button>
//             <button class="btn" onclick="nextUser()">下一个用户</button>
//         </div>
        
//         <div class="card">
//             <h3>📝 测试日志</h3>
//             <div id="log" style="height: 100px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; background: #fafafa; font-family: monospace; font-size: 12px;"></div>
//         </div>
//     </div>

//     <script>
//         let currentUserId = 'TEST_' + Math.random().toString(36).substr(2, 5).toUpperCase();
        
//         function updateStatus(data) {
//             // 更新系统状态
//             document.getElementById('currentWeight').textContent = data.system.current_weight;
//             document.getElementById('targetWeight').textContent = data.system.target_weight;
//             document.getElementById('currentUser').textContent = data.queue.current_user || '无';
            
//             const statusElem = document.getElementById('systemStatus');
//             const states = {0: ['等待中...', 'status-waiting'], 1: ['取药中...', 'status-running'], 2: ['完成!', 'status-complete']};
//             const [text, cls] = states[data.system.system_state] || ['未知', 'status-waiting'];
//             statusElem.textContent = text;
//             statusElem.className = 'status ' + cls;
            
//             // 更新队列
//             document.getElementById('queueCount').textContent = data.queue.queue_count;
//             const queueList = document.getElementById('queueList');
            
//             if (data.queue.queue_count > 0) {
//                 let html = '';
//                 data.queue.queue_list.forEach((user, index) => {
//                     const isCurrent = user === data.queue.current_user;
//                     html += `<div class="queue-item ${isCurrent ? 'current-user' : ''}">
//                         <span>${index + 1}. ${user} ${user === currentUserId ? '(我)' : ''}</span>
//                         <span>${isCurrent ? '正在取药' : '等待中'}</span>
//                     </div>`;
//                 });
//                 queueList.innerHTML = html;
//             } else {
//                 queueList.innerHTML = '<div style="text-align: center; color: #666; padding: 10px;">队列为空</div>';
//             }
//         }
        
//         function joinQueue() {
//             fetch('/api/join_queue?user_id=' + currentUserId)
//                 .then(r => r.json())
//                 .then(data => {
//                     if (data.success) {
//                         addLog('加入队列成功，位置: ' + data.position);
//                     } else {
//                         addLog('加入队列失败: ' + data.message);
//                     }
//                 });
//         }
        
//         function leaveQueue() {
//             fetch('/api/leave_queue?user_id=' + currentUserId)
//                 .then(r => r.json())
//                 .then(data => {
//                     if (data.success) {
//                         addLog('已离开队列');
//                     }
//                 });
//         }
        
//         function startTaking() {
//             fetch('/api/start_taking')
//                 .then(() => addLog('开始取药'));
//         }
        
//         function nextUser() {
//             fetch('/api/next_user')
//                 .then(() => addLog('切换到下一个用户'));
//         }
        
//         function addLog(message) {
//             const log = document.getElementById('log');
//             const time = new Date().toLocaleTimeString();
//             log.innerHTML = '[' + time + '] ' + message + '<br>' + log.innerHTML;
//         }
        
//         // 实时更新
//         setInterval(() => {
//             fetch('/api/status')
//                 .then(r => r.json())
//                 .then(updateStatus);
//         }, 500);
        
//         // 初始化
//         addLog('测试模式启动 - 用户ID: ' + currentUserId);
//         addLog('打开浏览器开发者工具查看网络请求');
//     </script>
// </body>
// </html>
//         )rawliteral";
//         request->send(200, "text/html", html);
//     });

//     // API接口
// // 在API路由中修复JSON创建
// server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
//     JsonDocument doc;
    
//     // 系统状态
//     doc["system"]["current_weight"] = g_status.current_weight;
//     doc["system"]["target_weight"] = g_status.target_weight;
//     doc["system"]["system_state"] = g_status.system_state;
//     doc["system"]["medicine_type"] = g_status.medicine_type;
    
//     // 队列状态
//     doc["queue"]["queue_count"] = medicineQueue.getQueueCount();
//     doc["queue"]["current_user"] = medicineQueue.getCurrentUser();
    
//     // 修复：使用新API创建数组
//     JsonArray queueArray = doc["queue"]["queue_list"].to<JsonArray>();
//     auto queueList = medicineQueue.getQueueList();
//     for (const auto& user : queueList) {
//         queueArray.add(user);
//     }
    
//     String json;
//     serializeJson(doc, json);
//     request->send(200, "application/json", json);
// });
    
//     server.on("/api/join_queue", HTTP_GET, [](AsyncWebServerRequest *request){
//         if (request->hasParam("user_id")) {
//             String userId = request->getParam("user_id")->value();
            
//             JsonDocument doc;
//             if (medicineQueue.addUser(userId)) {
//                 doc["success"] = true;
//                 doc["position"] = medicineQueue.getQueueCount();
//                 doc["message"] = "加入队列成功";
//             } else {
//                 doc["success"] = false;
//                 doc["message"] = "用户已在队列中";
//             }
            
//             String json;
//             serializeJson(doc, json);
//             request->send(200, "application/json", json);
//         }
//     });
    
//     server.on("/api/leave_queue", HTTP_GET, [](AsyncWebServerRequest *request){
//         if (request->hasParam("user_id")) {
//             String userId = request->getParam("user_id")->value();
            
//             JsonDocument doc;
//             doc["success"] = medicineQueue.removeUser(userId);
//             doc["message"] = doc["success"] ? "已离开队列" : "用户不在队列中";
            
//             String json;
//             serializeJson(doc, json);
//             request->send(200, "application/json", json);
//         }
//     });
    
//     server.on("/api/start_taking", HTTP_GET, [](AsyncWebServerRequest *request){
//         if (medicineQueue.getQueueCount() > 0 && medicineQueue.getCurrentUser().isEmpty()) {
//             medicineQueue.getNextUser();
//             g_status.system_state = 1;
//             g_status.current_weight = 0;
//             g_status.target_weight = 80 + random(0, 5) * 10;
//         }
//         request->send(200, "text/plain", "OK");
//     });
    
//     server.on("/api/next_user", HTTP_GET, [](AsyncWebServerRequest *request){
//         if (!medicineQueue.getNextUser().isEmpty()) {
//             g_status.system_state = 1;
//             g_status.current_weight = 0;
//             g_status.target_weight = 80 + random(0, 5) * 10;
//         }
//         request->send(200, "text/plain", "OK");
//     });

//     server.begin();
//     Serial.println("HTTP服务器已启动");
// }
// // 测试数据模拟器
// void Setup_Test_Data_Simulator() {
//     // 初始化一些测试用户
//     medicineQueue.addUser("TEST_USER_001");
//     medicineQueue.addUser("TEST_USER_002");
//     medicineQueue.addUser("TEST_USER_003");
// }

// void Update_Test_Data() {
//     static unsigned long lastUpdate = 0;
//     static bool increasing = true;
    
//     if (millis() - lastUpdate > 200) { // 每200ms更新一次
//         lastUpdate = millis();
        
//         // 模拟重量变化
//         if (g_status.system_state == 1) { // 运行状态
//             if (increasing) {
//                 g_status.current_weight += 2;
//                 if (g_status.current_weight >= g_status.target_weight) {
//                     g_status.current_weight = g_status.target_weight;
//                     g_status.system_state = 2; // 完成
//                     increasing = false;
                    
//                     // 自动切换到下一个用户
//                     if (!medicineQueue.getNextUser().isEmpty()) {
//                         g_status.system_state = 1;
//                         g_status.current_weight = 0;
//                         g_status.target_weight = 80 + random(0, 5) * 10; // 随机目标重量
//                     }
//                 }
//             }
//         } else if (g_status.system_state == 2) { // 完成状态
//             // 3秒后重置
//             static unsigned long completeTime = 0;
//             if (completeTime == 0) completeTime = millis();
            
//             if (millis() - completeTime > 3000) {
//                 g_status.system_state = 0;
//                 g_status.current_weight = 0;
//                 completeTime = 0;
//                 increasing = true;
//             }
//         }
//     }
// }


// void setup() {
//     Serial.begin(115200);
//     // 启动AP
//     WiFi.softAP(ssid, password);
//     Serial.println("ESP32最小测试模式启动");
//     Serial.print("AP IP: ");
//     Serial.println(WiFi.softAPIP());
//     Setup_Web_Server();
//     Setup_Test_Data_Simulator();
    
//     Serial.println("最小测试准备就绪，打开浏览器访问上述IP");
// }

// void loop() {
//     // 模拟数据更新
//     Update_Test_Data();
//     delay(100);
// }



// // Web服务器设置

#include <Arduino.h>

void setup() {
  // 延迟2秒，让串口稳定
  delay(2000);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  Serial.begin(115200);
}

void loop() {
  digitalWrite(4, !digitalRead(4));
  Serial.println("💡 LED闪烁 - 系统正常");
  delay(1000);
}