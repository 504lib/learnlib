
// 测试数据模拟器
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <queue>
#include <map>
#include "./protocol/protocol.hpp"

const char* ssid = "MedicineTest";
const char* password = "12345678";

AsyncWebServer server(80);

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);

// ArduinoJson兼容性宏
#if ARDUINOJSON_VERSION_MAJOR >= 7
    #define CREATE_JSON_ARRAY(parent, key) parent[key].to<JsonArray>()
#else
    #define CREATE_JSON_ARRAY(parent, key) parent.createNestedArray(key)
#endif

// 系统状态
struct SystemStatus {
    float current_weight = 0;
    int32_t target_weight = 0;  // 初始为0，必须设置后才能加入队列
    uint8_t system_state = 0;   // 0:等待, 1:运行, 2:完成
    Medicine medicine_type;
    bool motor_ready = true;    // 电机就绪状态，收到MOTOR_READY时为true
    bool queue_paused = false;  // 队列是否暂停（收到MOTOR_STOP时暂停）
};

SystemStatus g_status;

// 用户信息结构
struct UserInfo {
    String id;
    float target_weight;
};

// 队列系统
class UserQueue {
private:
    std::queue<UserInfo> userQueue;
    std::map<String, bool> userInQueue;
    UserInfo currentUser;
    
public:
    bool addUser(const String& userId, int32_t weight) {
        if (userInQueue.find(userId) != userInQueue.end()) {
            return false; // 用户已在队列中
        }
        
        if (weight <= 0) {
            return false; // 重量必须大于0
        }
        
        UserInfo user;
        user.id = userId;
        user.target_weight = weight;
        
        userQueue.push(user);
        userInQueue[userId] = true;
        Serial.printf("用户 %s 加入队列，目标重量: %dg，位置: %d\n", 
                     userId.c_str(), weight, userQueue.size());
        return true;
    }
    
    bool removeUser(const String& userId) {
        if (userInQueue.find(userId) == userInQueue.end()) {
            return false;
        }
        
        // 重建队列（排除要删除的用户）
        std::queue<UserInfo> newQueue;
        while (!userQueue.empty()) {
            UserInfo user = userQueue.front();
            userQueue.pop();
            if (user.id != userId) {
                newQueue.push(user);
            }
        }
        userQueue = newQueue;
        userInQueue.erase(userId);
        
        Serial.printf("用户 %s 离开队列\n", userId.c_str());
        return true;
    }
    
    UserInfo getNextUser() {
        if (userQueue.empty()) {
            currentUser = UserInfo(); // 清空
            return currentUser;
        }
        
        currentUser = userQueue.front();
        userQueue.pop();
        userInQueue.erase(currentUser.id);
        
        Serial.printf("开始服务用户: %s, 目标重量: %dg\n", 
                     currentUser.id.c_str(), currentUser.target_weight);
        return currentUser;
    }
    
    String getCurrentUserId() { return currentUser.id; }
    int32_t getCurrentUserTargetWeight() { return currentUser.target_weight; }
    int getQueueCount() { return userQueue.size(); }
    
    // 清空当前用户
    void clearCurrentUser() {
        currentUser = UserInfo();
    }
    
    // 获取队列列表（用于显示）
    std::vector<String> getQueueList() {
        std::vector<String> list;
        std::queue<UserInfo> temp = userQueue;
        
        while (!temp.empty()) {
            UserInfo user = temp.front();
            list.push_back(user.id + " (" + String(user.target_weight) + "g)");
            temp.pop();
        }
        return list;
    }
};

UserQueue medicineQueue;

// 自动队列处理函数
void ProcessAutoQueue() {
    static unsigned long lastQueueCheck = 0;
    
    if (millis() - lastQueueCheck > 1000) { // 每秒检查一次
        lastQueueCheck = millis();
        
        // 如果系统就绪且没有暂停，且有用户在队列中，自动开始下一个
        if (g_status.motor_ready && !g_status.queue_paused && 
            g_status.system_state == 0 && medicineQueue.getQueueCount() > 0) {
            
            UserInfo nextUser = medicineQueue.getNextUser();
            if (!nextUser.id.isEmpty()) {
                g_status.system_state = 1;
                g_status.current_weight = 0;
                g_status.target_weight = nextUser.target_weight;
                
                // 发送协议命令
                // uart_protocol.Send_Uart_Frame_TARGET_WEIGHT(g_status.target_weight);
                
                Serial.printf("🔄 自动开始用户: %s, 目标重量: %dg\n", 
                             nextUser.id.c_str(), g_status.target_weight);
            }
        }
    }
}

void Setup_Web_Server() {
    // 主页面
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>中药取药系统</title>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f8ff; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .card { background: #f8f9fa; padding: 15px; margin: 10px 0; border-radius: 8px; border-left: 4px solid #4CAF50; }
        .btn { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; background: #4CAF50; color: white; }
        .btn:disabled { background: #cccccc; cursor: not-allowed; }
        .btn-warning { background: #ff9800; }
        .status { padding: 8px; border-radius: 4px; font-weight: bold; }
        .status-waiting { background: #fff3cd; color: #856404; }
        .status-running { background: #d4edda; color: #155724; }
        .status-paused { background: #f8d7da; color: #721c24; }
        .status-complete { background: #d1ecf1; color: #0c5460; }
        .queue-item { display: flex; justify-content: space-between; padding: 8px; margin: 5px 0; background: white; border-radius: 4px; }
        .current-user { background: #e8f5e8; font-weight: bold; }
        .progress-bar { width: 100%; height: 20px; background: #e0e0e0; border-radius: 10px; overflow: hidden; margin: 10px 0; }
        .progress-fill { height: 100%; background: #4CAF50; transition: width 0.3s; }
        .motor-status { display: inline-block; padding: 4px 8px; border-radius: 4px; margin-left: 10px; }
        .motor-ready { background: #d4edda; color: #155724; }
        .motor-stopped { background: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🧪 中药取药系统</h1>
        
        <div class="card">
            <h3>📊 系统状态</h3>
            <div id="systemStatus" class="status status-waiting">等待中...</div>
            <p>当前重量: <span id="currentWeight">0</span>g / 目标重量: <span id="targetWeight">0</span>g</p>
            <div class="progress-bar">
                <div id="progressFill" class="progress-fill" style="width: 0%"></div>
            </div>
            <p>当前用户: <span id="currentUser">无</span></p>
            <p>电机状态: 
                <span id="motorStatus" class="motor-status motor-ready">就绪</span>
                <span id="queueStatus" style="margin-left: 10px;"></span>
            </p>
        </div>
        
        <div class="card">
            <h3>📋 取药队列 (<span id="queueCount">0</span>)</h3>
            <div id="queueList">
                <div style="text-align: center; color: #666; padding: 10px;">队列为空</div>
            </div>
        </div>
        
        <div class="card">
            <h3>👤 加入队列</h3>
            <div>
                <label>目标重量 (g): </label>
                <input type="number" id="weightInput" min="1" max="500" value="100" style="padding: 5px; margin: 5px;">
                <button class="btn" onclick="joinQueue()" id="joinBtn">加入队列</button>
                <button class="btn btn-warning" onclick="leaveQueue()" id="leaveBtn" style="display: none;">离开队列</button>
            </div>
            <div style="margin-top: 10px; color: #666; font-size: 14px;">
                提示: 目标重量必须大于0才能加入队列
            </div>
        </div>
        
        <div class="card">
            <h3>📝 系统日志</h3>
            <div id="log" style="height: 100px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; background: #fafafa; font-family: monospace; font-size: 12px;"></div>
        </div>
    </div>

    <script>
        let currentUserId = 'USER_' + Math.random().toString(36).substr(2, 5).toUpperCase();
        let userInQueue = false;
        
        function updateStatus(data) {
            // 更新系统状态
            document.getElementById('currentWeight').textContent = data.system.current_weight;
            document.getElementById('targetWeight').textContent = data.system.target_weight;
            document.getElementById('currentUser').textContent = data.queue.current_user || '无';
            
            // 更新进度条
            const progress = data.system.target_weight > 0 ? 
                (data.system.current_weight / data.system.target_weight * 100) : 0;
            document.getElementById('progressFill').style.width = Math.min(progress, 100) + '%';
            
            // 更新状态显示
            const statusElem = document.getElementById('systemStatus');
            const motorStatusElem = document.getElementById('motorStatus');
            const queueStatusElem = document.getElementById('queueStatus');
            
            if (data.system.queue_paused) {
                statusElem.textContent = '暂停中 - 请取走药物';
                statusElem.className = 'status status-paused';
                motorStatusElem.textContent = '暂停';
                motorStatusElem.className = 'motor-status motor-stopped';
                queueStatusElem.textContent = '⏸️ 队列已暂停';
            } else if (data.system.system_state === 1) {
                statusElem.textContent = '取药中...';
                statusElem.className = 'status status-running';
                motorStatusElem.textContent = '运行中';
                motorStatusElem.className = 'motor-status motor-ready';
                queueStatusElem.textContent = '▶️ 队列运行中';
            } else if (data.system.system_state === 2) {
                statusElem.textContent = '完成!';
                statusElem.className = 'status status-complete';
                motorStatusElem.textContent = '完成';
                motorStatusElem.className = 'motor-status motor-ready';
                queueStatusElem.textContent = '✅ 任务完成';
            } else {
                statusElem.textContent = '等待中...';
                statusElem.className = 'status status-waiting';
                motorStatusElem.textContent = '就绪';
                motorStatusElem.className = 'motor-status motor-ready';
                queueStatusElem.textContent = data.system.motor_ready ? '🔄 等待队列' : '⏳ 等待电机就绪';
            }
            
            // 更新队列
            document.getElementById('queueCount').textContent = data.queue.queue_count;
            const queueList = document.getElementById('queueList');
            
            if (data.queue.queue_count > 0) {
                let html = '';
                data.queue.queue_list.forEach((user, index) => {
                    const isCurrent = user.includes(data.queue.current_user);
                    html += `<div class="queue-item ${isCurrent ? 'current-user' : ''}">
                        <span>${index + 1}. ${user}</span>
                        <span>${isCurrent ? '正在取药' : '等待中'}</span>
                    </div>`;
                });
                queueList.innerHTML = html;
            } else {
                queueList.innerHTML = '<div style="text-align: center; color: #666; padding: 10px;">队列为空</div>';
            }
            
            // 更新按钮状态
            const joinBtn = document.getElementById('joinBtn');
            const leaveBtn = document.getElementById('leaveBtn');
            const weightInput = document.getElementById('weightInput');
            
            // 检查重量是否有效
            const weight = parseInt(weightInput.value);
            joinBtn.disabled = weight <= 0 || userInQueue;
            
            // 显示/隐藏离开队列按钮
            leaveBtn.style.display = userInQueue ? 'inline-block' : 'none';
        }
        
        function joinQueue() {
            const weight = document.getElementById('weightInput').value;
            if (weight <= 0) {
                addLog('错误: 目标重量必须大于0');
                return;
            }
            
            fetch('/api/join_queue?user_id=' + currentUserId + '&weight=' + weight)
                .then(r => r.json())
                .then(data => {
                    if (data.success) {
                        userInQueue = true;
                        addLog('加入队列成功，目标重量: ' + weight + 'g，位置: ' + data.position);
                    } else {
                        addLog('加入队列失败: ' + data.message);
                    }
                });
        }
        
        function leaveQueue() {
            fetch('/api/leave_queue?user_id=' + currentUserId)
                .then(r => r.json())
                .then(data => {
                    if (data.success) {
                        userInQueue = false;
                        addLog('已离开队列');
                    }
                });
        }
        
        function addLog(message) {
            const log = document.getElementById('log');
            const time = new Date().toLocaleTimeString();
            log.innerHTML = '[' + time + '] ' + message + '<br>' + log.innerHTML;
        }
        
        // 实时更新
        setInterval(() => {
            fetch('/api/status')
                .then(r => r.json())
                .then(updateStatus);
        }, 500);
        
        // 初始化
        addLog('系统启动 - 用户ID: ' + currentUserId);
        addLog('提示: 设置目标重量大于0后点击"加入队列"');
    </script>
</body>
</html>
        )rawliteral";
        request->send(200, "text/html", html);
    });

    // API接口
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        
        // 系统状态
        doc["system"]["current_weight"] = g_status.current_weight;
        doc["system"]["target_weight"] = g_status.target_weight;
        doc["system"]["system_state"] = g_status.system_state;
        doc["system"]["medicine_type"] = static_cast<uint8_t>(g_status.medicine_type);
        doc["system"]["motor_ready"] = g_status.motor_ready;
        doc["system"]["queue_paused"] = g_status.queue_paused;

        // 队列状态
        doc["queue"]["queue_count"] = medicineQueue.getQueueCount();
        doc["queue"]["current_user"] = medicineQueue.getCurrentUserId();
        
        JsonArray queueArray = CREATE_JSON_ARRAY(doc["queue"], "queue_list");
        auto queueList = medicineQueue.getQueueList();
        for (const auto& user : queueList) {
            queueArray.add(user);
        }
        
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    
    server.on("/api/join_queue", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("user_id") && request->hasParam("weight")) {
            String userId = request->getParam("user_id")->value();
            int32_t weight = request->getParam("weight")->value().toInt();
            
            JsonDocument doc;
            if (weight <= 0) {
                doc["success"] = false;
                doc["message"] = "目标重量必须大于0";
            } else if (medicineQueue.addUser(userId, weight)) {
                doc["success"] = true;
                doc["position"] = medicineQueue.getQueueCount();
                doc["message"] = "加入队列成功";
            } else {
                doc["success"] = false;
                doc["message"] = "用户已在队列中";
            }
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        }
    });
    
    server.on("/api/leave_queue", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("user_id")) {
            String userId = request->getParam("user_id")->value();
            
            JsonDocument doc;
            doc["success"] = medicineQueue.removeUser(userId);
            doc["message"] = doc["success"] ? "已离开队列" : "用户不在队列中";
            
            String json;
            serializeJson(doc, json);
            request->send(200, "application/json", json);
        }
    });

    server.begin();
    Serial.println("HTTP服务器已启动");
}

// 重量回调函数
void weight_callback(float weight)
{
    if (weight < 0.0f) {
        weight = 0;
    }
    
    g_status.current_weight = weight;
    
    // 重量达到目标时自动完成
    if (g_status.system_state == 1 && weight >= g_status.target_weight) {
        g_status.system_state = 2;
        Serial.println("✅ 取药完成！等待取走药物...");
    }
}

// 电机停止回调（来自STM32）
void motor_stop_callback()
{
    g_status.queue_paused = true;
    Serial.println("⏸️ 收到MOTOR_STOP: 队列暂停，请取走药物");

}

// 电机就绪回调（来自STM32）  
void motor_ready_callback()
{
    g_status.motor_ready = true;
    g_status.queue_paused = false;

    
    // 如果之前是完成状态，现在重置为等待状态
    if (g_status.system_state == 2) {
        g_status.system_state = 0;
        g_status.current_weight = 0;
        medicineQueue.clearCurrentUser();
    }
    
    Serial.println("▶️ 收到MOTOR_READY: 电机就绪，队列继续");
}

void setup() {
    Serial.begin(115200);
    
    // 设置回调函数
    uart_protocol.setWeightCallback(weight_callback);
    uart_protocol.setMotorStopCallback(motor_stop_callback);
    uart_protocol.setMotorReadyCallback(motor_ready_callback);
    
    // 启动AP
    WiFi.softAP(ssid, password);
    Serial.println("ESP32中药取药系统启动");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    Setup_Web_Server();
    
    Serial.println("系统准备就绪，打开浏览器访问上述IP");
    Serial.println("工作流程: 设置目标重量 → 加入队列 → 自动称重 → 取走药物 → 继续下一个");
}

void loop() {
    static String lastUser = "";
    String currentUser = medicineQueue.getCurrentUserId();
    
    // 处理自动队列
    ProcessAutoQueue();
    
    // 处理串口通信
    if (Serial1.available()) {
        uart_protocol.Receive_Uart_Frame(Serial1.read());
    }

    // 用户变化时发送用户信息
    if (currentUser != lastUser) {
    float targetWeight = medicineQueue.getCurrentUserTargetWeight();
    if (targetWeight > 0) {
        uart_protocol.Send_Uart_Frame_TARGET_WEIGHT(targetWeight);
    }
    delay(100);
    uart_protocol.Send_Uart_Frame_CURRENT_USER(currentUser, g_status.medicine_type);
    //     // 同时发送目标重量
        lastUser = currentUser;
    }


    
    delay(1);
}