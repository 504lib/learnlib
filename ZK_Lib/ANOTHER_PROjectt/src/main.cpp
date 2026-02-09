#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define DHT_PIN   10              // DHT11 引脚
#define LED_BUILTIN2 13           // 第二个LED引脚
#define MQ4_AO_PIN 0              // MQ-4 模拟输出引脚
#define MQ4_DO_PIN 1              // MQ-4 数字输出引脚  
#define MOTOR_PIN 7                // 电机控制引脚
#define LIGHT_BULB_PIN 6           // 灯泡控制引脚

#define TEMP_THRESHOLD 30.0       // 温度预警的阈值，单位为摄氏度，根据实际情况调整
#define HUM_THRESHOLD 70.0        // 湿度预警的阈值，单位为百分比，根据实际情况调整
#define MQ4_THRESHOLD 300.0       // MQ-4 模拟预警的阈值，单位为ppm，根据实际情况调整

struct Alarm_Flag
{
  bool temp_alarm;                // 温度预警标志
  bool hum_alarm;                 // 湿度预警标志 
  bool mq4_alarm;                 // MQ-4 预警标志
};

struct Use_Flag
{
  bool isLEDUsed;                 // LED 是否被使用
  bool isMotorUsed;               // 电机是否被使用  
  bool isAlarmActive;            // 预警是否激活
};


struct Data_Monitor
{
  volatile float temperature;     // 温度监测数据
  volatile float humidity;        // 湿度监测数据
  volatile float mq4_ppm;         // MQ-4 监测数据，单位为ppm
};

struct Data_Sim
{
  volatile float temperature;     // 温度模拟数据
  volatile float humidity;        // 湿度模拟数据
  volatile float mq2_ppm;         // MQ-2 模拟数据，单位为ppm
};


// 全局变量定义
Alarm_Flag alarm_flag = {false, false, false};
Use_Flag use_flag = {false, false,false};
Data_Monitor data_monitor = {0.0f, 0.0f, 0.0f};
Data_Sim data_sim = {25.0f, 30.0f,100.0f};

DHT dht(DHT_PIN,DHT11);

float RLOAD = 10.0;               // MQ-4 负载电阻，单位为kΩ，根据实际情况调整
float RZERO = 9.65;               // MQ-4 在100ppm甲烷时的电阻，单位为kΩ，根据实际情况调整
static bool ledState = LOW;       // LED 状态变量
static bool isSimulationMode = false; // 模拟模式标志变量
float temp = 0.0f;
float hum = 0.0f;

const char* WIFI_SSID = "home";
const char* WIFI_PASS = "12345678";

AsyncWebServer server(80);


const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>智能家居</title>
  <style>
    :root {
      --bg: #eaf2fb;
      --bg-2: #f7fbff;
      --card: #ffffff;
      --line: #c9d9ea;
      --text: #0b2b52;
      --muted: #55708f;
      --accent: #1677d8;
      --accent-2: #0b4e9e;
      --ok: #1b8f5a;
      --warn: #d88b16;
      --danger: #d43b3b;
      --shadow: 0 12px 28px rgba(11, 43, 82, 0.12);
    }

    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "Segoe UI", "PingFang SC", "Noto Sans SC", sans-serif;
      color: var(--text);
      background: radial-gradient(1000px 600px at 10% 10%, #ffffff 0%, var(--bg) 45%, #e3eefb 100%);
      min-height: 100vh;
    }

    header {
      padding: 24px clamp(16px, 4vw, 40px);
      display: flex;
      align-items: center;
      justify-content: space-between;
      border-bottom: 1px solid var(--line);
      background: linear-gradient(90deg, #ffffff 0%, #f0f7ff 100%);
    }

    .title {
      display: flex;
      flex-direction: column;
      gap: 4px;
    }

    .title h1 {
      margin: 0;
      font-size: clamp(20px, 3vw, 28px);
      letter-spacing: 1px;
    }

    .title span {
      color: var(--muted);
      font-size: 13px;
    }

    .mode-pill {
      padding: 6px 14px;
      border-radius: 999px;
      font-size: 12px;
      letter-spacing: 0.4px;
      background: #e8f2ff;
      color: var(--accent-2);
      border: 1px solid #cfe3ff;
    }

    .container {
      padding: 24px clamp(16px, 4vw, 40px) 36px;
      display: grid;
      gap: 20px;
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 16px;
    }

    .card {
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      padding: 16px;
      box-shadow: var(--shadow);
      position: relative;
      overflow: hidden;
    }

    .card::after {
      content: "";
      position: absolute;
      right: -40px;
      top: -40px;
      width: 120px;
      height: 120px;
      background: radial-gradient(circle, rgba(22, 119, 216, 0.12), rgba(22, 119, 216, 0));
    }

    .card h3 {
      margin: 0 0 8px;
      font-size: 14px;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 1px;
    }

    .value {
      font-size: 26px;
      font-weight: 700;
    }

    .unit {
      font-size: 14px;
      color: var(--muted);
      margin-left: 6px;
    }

    .status-row {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
      margin-top: 10px;
    }

    .status {
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 12px;
      border: 1px solid transparent;
      background: #edf5ff;
      color: var(--accent-2);
    }

    .status.ok { background: #e8f6ef; color: var(--ok); border-color: #b8e4cf; }
    .status.warn { background: #fff5e5; color: var(--warn); border-color: #ffd9a6; }
    .status.danger { background: #fde8e8; color: var(--danger); border-color: #f5b9b9; }

    .panel {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 16px;
    }

    .control-item {
      display: flex;
      flex-direction: column;
      gap: 12px;
    }

    .toggle {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: var(--bg-2);
    }

    .toggle button {
      border: none;
      padding: 8px 14px;
      border-radius: 999px;
      cursor: pointer;
      background: #dbe9fb;
      color: var(--accent-2);
      font-weight: 600;
    }

    .toggle button.active {
      background: var(--accent);
      color: #ffffff;
      box-shadow: 0 6px 16px rgba(22, 119, 216, 0.25);
    }

    .form-grid {
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
    }

    label {
      font-size: 12px;
      color: var(--muted);
    }

    input {
      width: 100%;
      padding: 8px 10px;
      border-radius: 8px;
      border: 1px solid var(--line);
      background: #ffffff;
      color: var(--text);
      font-size: 14px;
    }

    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
    }

    .primary {
      background: var(--accent);
      color: #ffffff;
      border: none;
      padding: 10px 16px;
      border-radius: 10px;
      cursor: pointer;
      font-weight: 600;
      box-shadow: 0 10px 20px rgba(22, 119, 216, 0.25);
    }

    .ghost {
      border: 1px solid var(--line);
      background: #ffffff;
      color: var(--accent-2);
      padding: 10px 16px;
      border-radius: 10px;
      cursor: pointer;
      font-weight: 600;
    }

    .footer {
      color: var(--muted);
      font-size: 12px;
      text-align: right;
    }

    @media (max-width: 640px) {
      header { flex-direction: column; align-items: flex-start; gap: 12px; }
    }
  </style>
</head>
<body>
  <header>
    <div class="title">
      <h1>智能家居</h1>
      <span>温湿度 · 甲烷浓度 · 设备控制</span>
    </div>
    <div class="mode-pill" id="modePill">MODE</div>
  </header>

  <main class="container">
    <section class="grid" id="dataCards">
      <div class="card">
        <h3>温度</h3>
        <div class="value" id="tempValue">--</div>
        <span class="unit">°C</span>
      </div>
      <div class="card">
        <h3>湿度</h3>
        <div class="value" id="humValue">--</div>
        <span class="unit">%</span>
      </div>
      <div class="card">
        <h3>甲烷浓度</h3>
        <div class="value" id="gasValue">--</div>
        <span class="unit">ppm</span>
      </div>
      <div class="card">
        <h3>报警状态</h3>
        <div class="status-row">
          <span class="status" id="tempAlarm">温度</span>
          <span class="status" id="humAlarm">湿度</span>
          <span class="status" id="gasAlarm">气体</span>
        </div>
      </div>
    </section>

    <section class="panel">
      <div class="card control-item">
        <h3>设备控制</h3>
        <div class="toggle">
          <span>灯泡</span>
          <div>
            <button type="button" id="ledOff">OFF</button>
            <button type="button" id="ledOn">ON</button>
          </div>
        </div>
        <div class="toggle">
          <span>风扇/电机</span>
          <div>
            <button type="button" id="motorOff">OFF</button>
            <button type="button" id="motorOn">ON</button>
          </div>
        </div>
        <div class="actions">
          <button class="primary" id="applyControl">提交控制</button>
          <button class="ghost" id="toggleMode">切换模式</button>
        </div>
      </div>

      <div class="card control-item">
        <h3>模拟数据输入</h3>
        <div class="form-grid">
          <div>
            <label for="simTemp">温度 °C</label>
            <input id="simTemp" type="number" step="0.1" value="25" />
          </div>
          <div>
            <label for="simHum">湿度 %</label>
            <input id="simHum" type="number" step="0.1" value="30" />
          </div>
          <div>
            <label for="simGas">气体 ppm</label>
            <input id="simGas" type="number" step="0.1" value="100" />
          </div>
        </div>
        <div class="actions">
          <button class="primary" id="applySim">写入模拟数据</button>
          <button class="ghost" id="refreshNow">立即刷新</button>
        </div>
        <div class="footer" id="lastUpdate">Last update: --</div>
      </div>
    </section>
  </main>

  <script>
    const state = {
      led: "off",
      motor: "off",
      mode: "monitor",
      timer: null
    };

    const $ = (id) => document.getElementById(id);

    function setStatus(el, active) {
      el.classList.remove("ok", "warn", "danger");
      if (active) {
        el.classList.add("danger");
      } else {
        el.classList.add("ok");
      }
    }

    function setButtonGroup(onBtn, offBtn, isOn) {
      if (isOn) {
        onBtn.classList.add("active");
        offBtn.classList.remove("active");
      } else {
        offBtn.classList.add("active");
        onBtn.classList.remove("active");
      }
    }

    async function fetchMode() {
      const res = await fetch("/cur_mode");
      const mode = await res.text();
      state.mode = mode.trim();
      $("modePill").textContent = state.mode === "simulation" ? "SIMULATION" : "MONITOR";
    }

    async function fetchAlarms() {
      const res = await fetch("/alarm_status");
      const data = await res.json();
      setStatus($("tempAlarm"), data.temp_alarm);
      setStatus($("humAlarm"), data.hum_alarm);
      setStatus($("gasAlarm"), data.mq4_alarm);
    }

    async function fetchData() {
      const url = state.mode === "simulation" ? "/data_sim" : "/data";
      const res = await fetch(url);
      const data = await res.json();
      const temp = data.temperature ?? "--";
      const hum = data.humidity ?? "--";
      const gas = data.mq4_ppm ?? data.mq2_ppm ?? "--";
      $("tempValue").textContent = Number(temp).toFixed(1);
      $("humValue").textContent = Number(hum).toFixed(1);
      $("gasValue").textContent = Number(gas).toFixed(1);
      $("lastUpdate").textContent = `Last update: ${new Date().toLocaleTimeString()}`;
    }

    async function refreshAll() {
      await fetchMode();
      await Promise.all([fetchData(), fetchAlarms()]);
    }

    async function applyControl() {
      const form = new FormData();
      form.append("led", state.led);
      form.append("motor", state.motor);
      await fetch("/control", { method: "POST", body: form });
      await fetchData();
    }

    async function applySim() {
      const form = new FormData();
      form.append("temperature", $("simTemp").value);
      form.append("humidity", $("simHum").value);
      form.append("mq4_ppm", $("simGas").value);
      await fetch("/set_sim", { method: "POST", body: form });
      await fetchData();
    }

    async function toggleMode() {
      await fetch("/toggle_mode", { method: "POST" });
      await refreshAll();
    }

    $("ledOn").addEventListener("click", () => {
      state.led = "on";
      setButtonGroup($("ledOn"), $("ledOff"), true);
    });
    $("ledOff").addEventListener("click", () => {
      state.led = "off";
      setButtonGroup($("ledOn"), $("ledOff"), false);
    });
    $("motorOn").addEventListener("click", () => {
      state.motor = "on";
      setButtonGroup($("motorOn"), $("motorOff"), true);
    });
    $("motorOff").addEventListener("click", () => {
      state.motor = "off";
      setButtonGroup($("motorOn"), $("motorOff"), false);
    });
    $("applyControl").addEventListener("click", applyControl);
    $("applySim").addEventListener("click", applySim);
    $("toggleMode").addEventListener("click", toggleMode);
    $("refreshNow").addEventListener("click", refreshAll);

    refreshAll();
    state.timer = setInterval(refreshAll, 500);
  </script>
</body>
</html>
)HTML";

float readMQ135Resistance() 
{
  int adc_value = analogRead(MQ4_AO_PIN);
  float voltage = adc_value * (3.3 / 4095.0);
  return (3.3 - voltage) / voltage * RLOAD;
}

float calculatePPM(float resistance) {
  return 116.6020682 * pow((resistance / RZERO), -2.769034857);
}

void setup() 
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN2, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LIGHT_BULB_PIN, OUTPUT);
  pinMode(MQ4_AO_PIN,INPUT);
  pinMode(MQ4_DO_PIN,INPUT);
  dht.begin();
  if (WiFi.softAP(WIFI_SSID, WIFI_PASS,1,0,10,false)) {
    Serial.println("AP 模式启动成功!");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP 地址: ");
    Serial.println(myIP);
  } 
  else 
  {
    Serial.println("AP 模式启动失败!");
    return;
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html;charset=utf-8", INDEX_HTML);
  });
  
  server.on("/cur_mode", HTTP_GET, [](AsyncWebServerRequest *request){
    String mode = isSimulationMode ? "simulation" : "monitor";
    request->send(200, "text/plain", mode);
  });

  server.on("/toggle_mode", HTTP_POST, [](AsyncWebServerRequest *request){
    isSimulationMode = !isSimulationMode;
    String mode = isSimulationMode ? "simulation" : "monitor";
    request->send(200, "text/plain", mode);
  });

  server.on("/alarm_status", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument jsonDoc;
    jsonDoc["temp_alarm"] = alarm_flag.temp_alarm;
    jsonDoc["hum_alarm"] = alarm_flag.hum_alarm;
    jsonDoc["mq4_alarm"] = alarm_flag.mq4_alarm;
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument jsonDoc;
    jsonDoc["temperature"] = data_monitor.temperature;
    jsonDoc["humidity"] = data_monitor.humidity;
    jsonDoc["mq4_ppm"] = data_monitor.mq4_ppm;
    jsonDoc["led_state"] = use_flag.isLEDUsed ? "on" : "off";
    jsonDoc["motor_state"] = use_flag.isMotorUsed ? "on" : "off";
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  server.on("/data_sim", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument jsonDoc;
    jsonDoc["temperature"] = data_sim.temperature;
    jsonDoc["humidity"] = data_sim.humidity;
    jsonDoc["mq2_ppm"] = data_sim.mq2_ppm;
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("led", true)) {
      String ledValue = request->getParam("led", true)->value();
      use_flag.isLEDUsed = (ledValue == "on");
      Serial.printf("LED 控制: %s\n", use_flag.isLEDUsed ? "ON" : "OFF");
    }
    if (request->hasParam("motor", true)) {
      String motorValue = request->getParam("motor", true)->value();
      use_flag.isMotorUsed = (motorValue == "on");
      Serial.printf("Motor 控制: %s\n", use_flag.isMotorUsed ? "ON" : "OFF");
    }
    request->send(200, "text/plain", "Control updated");
  });

  server.on("/set_sim", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("temperature", true)) {
      data_sim.temperature = request->getParam("temperature", true)->value().toFloat();
    }
    if (request->hasParam("humidity", true)) {
      data_sim.humidity = request->getParam("humidity", true)->value().toFloat();
    }
    if (request->hasParam("mq4_ppm", true)) {
      data_sim.mq2_ppm = request->getParam("mq4_ppm", true)->value().toFloat();
    }
    request->send(200, "text/plain", "Simulation updated");
  });
  server.begin();
}

void loop() {
  static uint32_t last_time = 0;
  if(millis() - last_time > 200) {
    ledState = !ledState;
    // use_flag.isMotorUsed = !use_flag.isMotorUsed;  // 模拟电机状态切换
    float temp_cur = dht.readTemperature();       // 读取当前温度 
    float hum_cur = dht.readHumidity();           // 读取当前湿度

    if (!isnan(temp_cur)) {
      data_monitor.temperature = 0.8f * data_monitor.temperature + 0.2f * temp_cur;   // 使用低通滤波更新温度数据
    }
    
    if (!isnan(hum_cur)) {
      data_monitor.humidity = 0.8f * data_monitor.humidity + 0.2f * hum_cur;          // 使用低通滤波更新湿度数据
    }

    float resistance = readMQ135Resistance();                                         // 读取MQ-4电阻值
    data_monitor.mq4_ppm = calculatePPM(resistance);                                  // 计算MQ-4的ppm值

    digitalWrite(LED_BUILTIN, ledState);                                    // 板载LED闪烁
    digitalWrite(LED_BUILTIN2, ledState);                                   // 第二个LED闪烁

    float temp = (isSimulationMode) ? data_sim.temperature : data_monitor.temperature;
    float hum = (isSimulationMode) ? data_sim.humidity : data_monitor.humidity;
    float mq4_ppm = (isSimulationMode) ? data_sim.mq2_ppm : data_monitor.mq4_ppm;

    alarm_flag.temp_alarm = (temp >= TEMP_THRESHOLD);
    alarm_flag.hum_alarm = (hum >= HUM_THRESHOLD);
    alarm_flag.mq4_alarm = (mq4_ppm >= MQ4_THRESHOLD);
    
    if ((alarm_flag.temp_alarm || alarm_flag.hum_alarm || alarm_flag.mq4_alarm))
    {
      use_flag.isAlarmActive = true;  // 触发预警时开启电机
    }
    else
    {
      use_flag.isAlarmActive = false; // 预警解除时关闭电机
    }
    
    
    if (use_flag.isMotorUsed || use_flag.isAlarmActive)
    {
      digitalWrite(MOTOR_PIN, HIGH);  // 模拟电机开启
    } 
    else 
    {
      digitalWrite(MOTOR_PIN, LOW);   // 模拟电机关闭
    }

    if (use_flag.isLEDUsed)
    {
      digitalWrite(LIGHT_BULB_PIN, HIGH);  // 模拟灯泡开启
    } 
    else 
    {
      digitalWrite(LIGHT_BULB_PIN, LOW);   // 模拟灯泡关闭
    }
    
    // Serial.printf("LED State: %s\n", ledState ? "ON" : "OFF");
    // Serial.printf("MQ-4 PPM: %.2f\n", data_monitor.mq4_ppm);

    // Serial.printf("温度: %.2f°C, 湿度: %.2f%%\n", data_monitor.temperature, data_monitor.humidity);
    last_time = millis();                                                            // 更新上次读取时间
  }
}
