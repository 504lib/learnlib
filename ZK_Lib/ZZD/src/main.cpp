#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "INDUSTRY-Measurement";
const char* WIFI_PASS = "12345678";

AsyncWebServer server(80);

// 模拟值和真实值
volatile float simCO2 = 420, simTemp = 25.0, simHum = 45.0, simMQ135 = 250.0;
volatile float realCO2 = 0, realTemp = 0.0, realHum = 0.0, mq135PPM = 0.0;
volatile bool isSimData = false;

// 阈值范围（可动态修改）
float co2Min = 0.0f, co2Max = 1000.0f;
float tempMin = -10.0f, tempMax = 28.0f;
float humMin = 0.0f, humMax = 60.0f;
float mq135Min = 0.0f, mq135Max = 300.0f;

// 美化后的页面
const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="zh-cn">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 环境监测系统</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; }
        .container { max-width: 1200px; margin: 0 auto; }
        .header { text-align: center; color: white; margin-bottom: 30px; }
        .header h1 { font-size: 2.5rem; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }
        .dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 30px; }
        .card { background: rgba(255, 255, 255, 0.95); border-radius: 15px; padding: 25px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); }
        .card h3 { color: #333; margin-bottom: 20px; font-size: 1.3rem; }
        .data-grid { display: grid; gap: 15px; }
        .data-item { display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid rgba(0,0,0,0.1); }
        .data-label { font-weight: 600; color: #555; }
        .data-value { font-weight: 700; font-size: 1.2rem; }
        .real-data { color: #2ecc71; }
        .sim-data { color: #e74c3c; }
        .threshold-info { font-size: 0.9rem; color: #7f8c8d; margin-top: 8px; padding-top: 8px; border-top: 1px dashed #ddd; }
        .control-panel, .mode-panel, .threshold-panel { background: rgba(255, 255, 255, 0.95); border-radius: 15px; padding: 25px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); margin-bottom: 20px; }
        .control-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-bottom: 20px; }
        .input-group { display: flex; flex-direction: column; gap: 5px; }
        .input-group label { font-weight: 600; color: #333; }
        .input-group input { padding: 12px; border: 2px solid #ddd; border-radius: 8px; font-size: 1rem; }
        .btn { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; padding: 15px 30px; border-radius: 8px; font-size: 1.1rem; font-weight: 600; cursor: pointer; width: 100%; margin: 5px 0; }
        .btn-success { background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); }
        .btn-warning { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); }
        .btn-active { transform: scale(1.05); box-shadow: 0 0 0 3px rgba(255,255,255,0.5); }
        .mode-indicator { display: inline-block; padding: 8px 16px; border-radius: 20px; font-weight: 600; margin-left: 10px; }
        .mode-real { background: #2ecc71; color: white; }
        .mode-sim { background: #e74c3c; color: white; }
        .last-update { text-align: center; color: rgba(255,255,255,0.8); margin-top: 20px; font-size: 0.9rem; }
        .threshold-panel h3 i { font-style: normal; margin-right: 8px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌿 ESP32 环境监测系统</h1>
            <p>实时监控环境数据 | 支持模拟数据调试</p>
        </div>
        
        <div class="dashboard">
            <div class="card">
                <h3>📊 CO2 浓度监测</h3>
                <div class="data-grid">
                    <div class="data-item">
                        <span class="data-label">真实 CO2:</span>
                        <span id="real-co2" class="data-value real-data">-- ppm</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">模拟 CO2:</span>
                        <span id="sim-co2" class="data-value sim-data">-- ppm</span>
                    </div>
                    <div class="threshold-info">阈值范围: <span id="co2-range">-- ~ -- ppm</span></div>
                </div>
            </div>
            
            <div class="card">
                <h3>🌡️ 温湿度监测</h3>
                <div class="data-grid">
                    <div class="data-item">
                        <span class="data-label">真实温度:</span>
                        <span id="real-temp" class="data-value real-data">-- °C</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">模拟温度:</span>
                        <span id="sim-temp" class="data-value sim-data">-- °C</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">真实湿度:</span>
                        <span id="real-hum" class="data-value real-data">-- %</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">模拟湿度:</span>
                        <span id="sim-hum" class="data-value sim-data">-- %</span>
                    </div>
                    <div class="threshold-info">温度范围: <span id="temp-range">-- ~ -- °C</span>&nbsp;&nbsp;|&nbsp;&nbsp;湿度范围: <span id="hum-range">-- ~ -- %</span></div>
                </div>
            </div>
            
            <div class="card">
                <h3>📈 MQ135 气体监测</h3>
                <div class="data-grid">
                    <div class="data-item">
                        <span class="data-label">真实 MQ135:</span>
                        <span id="real-mq135" class="data-value real-data">-- ppm</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">模拟 MQ135:</span>
                        <span id="sim-mq135" class="data-value sim-data">-- ppm</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">数据模式:</span>
                        <span id="data-mode" class="data-value">
                            <span id="mode-indicator" class="mode-indicator mode-real">真实模式</span>
                        </span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">报警状态:</span>
                        <span id="alarm-status" class="data-value">正常</span>
                    </div>
                    <div class="data-item">
                        <span class="data-label">电机状态:</span>
                        <span id="motor-status" class="data-value">停止</span>
                    </div>
                    <div class="threshold-info">阈值范围: <span id="mq135-range">-- ~ -- ppm</span></div>
                </div>
            </div>
        </div>
        
        <div class="mode-panel">
            <h3>🔁 数据模式控制</h3>
            <button class="btn btn-success btn-active" id="use-real-data">使用真实数据</button>
            <button class="btn btn-warning" id="use-sim-data">使用模拟数据</button>
            <div style="margin-top: 15px; text-align: center;">
                <span id="mode-description">当前使用传感器真实数据进行报警判断</span>
            </div>
        </div>
        
        <div class="threshold-panel">
            <h3><i>⚙️</i> 环境监测数据范围设置</h3>
            <div class="control-grid">
                <div class="input-group">
                    <label>CO2 范围 (ppm)</label>
                    <input type="number" id="co2-min" step="1" placeholder="最小值">
                    <input type="number" id="co2-max" step="1" placeholder="最大值">
                </div>
                <div class="input-group">
                    <label>温度范围 (°C)</label>
                    <input type="number" id="temp-min" step="0.1" placeholder="最小值">
                    <input type="number" id="temp-max" step="0.1" placeholder="最大值">
                </div>
                <div class="input-group">
                    <label>湿度范围 (%)</label>
                    <input type="number" id="hum-min" step="0.1" placeholder="最小值">
                    <input type="number" id="hum-max" step="0.1" placeholder="最大值">
                </div>
                <div class="input-group">
                    <label>MQ135 范围 (ppm)</label>
                    <input type="number" id="mq135-min" step="1" placeholder="最小值">
                    <input type="number" id="mq135-max" step="1" placeholder="最大值">
                </div>
            </div>
            <button class="btn" id="update-thresholds">提交范围修改</button>
        </div>
        
        <div class="control-panel">
            <h3>🎛️ 模拟数据控制</h3>
            <div class="control-grid">
                <div class="input-group">
                    <label for="co2">CO2 浓度 (ppm)</label>
                    <input type="number" id="co2" value="420" min="0" max="5000">
                </div>
                <div class="input-group">
                    <label for="temp">温度 (°C)</label>
                    <input type="number" id="temp" value="25.0" step="0.1" min="-40" max="85">
                </div>
                <div class="input-group">
                    <label for="hum">湿度 (%)</label>
                    <input type="number" id="hum" value="45.0" step="0.1" min="0" max="100">
                </div>
                <div class="input-group">
                    <label for="mq135">MQ135 气体 (ppm)</label>
                    <input type="number" id="mq135" value="250.0" step="0.1" min="0" max="2000">
                </div>
            </div>
            <button class="btn" id="send">提交模拟数据</button>
        </div>
        
        <div class="last-update">
            <span>最后更新: </span>
            <span id="last-update-time">--</span>
        </div>
    </div>

    <script>
        // 更新阈值范围显示及输入框
        function updateThresholdsDisplay(thresholds) {
            document.getElementById('co2-range').innerText = thresholds.co2.min + ' ~ ' + thresholds.co2.max + ' ppm';
            document.getElementById('temp-range').innerText = thresholds.temp.min + ' ~ ' + thresholds.temp.max + ' °C';
            document.getElementById('hum-range').innerText = thresholds.hum.min + ' ~ ' + thresholds.hum.max + ' %';
            document.getElementById('mq135-range').innerText = thresholds.mq135.min + ' ~ ' + thresholds.mq135.max + ' ppm';
            // 设置面板输入框的值
            document.getElementById('co2-min').value = thresholds.co2.min;
            document.getElementById('co2-max').value = thresholds.co2.max;
            document.getElementById('temp-min').value = thresholds.temp.min;
            document.getElementById('temp-max').value = thresholds.temp.max;
            document.getElementById('hum-min').value = thresholds.hum.min;
            document.getElementById('hum-max').value = thresholds.hum.max;
            document.getElementById('mq135-min').value = thresholds.mq135.min;
            document.getElementById('mq135-max').value = thresholds.mq135.max;
        }
        
        // 更新显示的数据
        function updateDisplay(data) {
            // 真实数据
            document.getElementById('real-co2').textContent = data.real.co2.toFixed(0) + ' ppm';
            document.getElementById('real-temp').textContent = data.real.temp.toFixed(1) + ' °C';
            document.getElementById('real-hum').textContent = data.real.hum.toFixed(1) + ' %';
            document.getElementById('real-mq135').textContent = data.real.mq135.toFixed(1) + ' ppm';
            
            // 模拟数据
            document.getElementById('sim-co2').textContent = data.sim.co2.toFixed(0) + ' ppm';
            document.getElementById('sim-temp').textContent = data.sim.temp.toFixed(1) + ' °C';
            document.getElementById('sim-hum').textContent = data.sim.hum.toFixed(1) + ' %';
            document.getElementById('sim-mq135').textContent = data.sim.mq135.toFixed(1) + ' ppm';
            
            // 系统状态
            document.getElementById('alarm-status').textContent = data.status.alarm ? '🚨 报警' : '✅ 正常';
            document.getElementById('alarm-status').style.color = data.status.alarm ? '#e74c3c' : '#2ecc71';
            document.getElementById('motor-status').textContent = data.status.motor ? '运行' : '停止';
            
            // 数据模式
            const isSimMode = data.status.simMode;
            document.getElementById('mode-indicator').textContent = isSimMode ? '模拟模式' : '真实模式';
            document.getElementById('mode-indicator').className = isSimMode ? 'mode-indicator mode-sim' : 'mode-indicator mode-real';
            document.getElementById('mode-description').textContent = isSimMode 
                ? '当前使用模拟数据进行报警判断' 
                : '当前使用传感器真实数据进行报警判断';
            
            // 更新按钮状态
            document.getElementById('use-real-data').classList.toggle('btn-active', !isSimMode);
            document.getElementById('use-sim-data').classList.toggle('btn-active', isSimMode);
            
            // 更新时间
            document.getElementById('last-update-time').textContent = new Date().toLocaleString('zh-CN');
        }
        
        // 获取所有数据
        async function fetchAllData() {
            try {
                const [simResponse, realResponse, statusResponse] = await Promise.all([
                    fetch('/api/sim'),
                    fetch('/api/real'),
                    fetch('/api/status')
                ]);
                
                const simData = await simResponse.json();
                const realData = await realResponse.json();
                const statusData = await statusResponse.json();
                
                updateDisplay({
                    sim: simData,
                    real: realData,
                    status: {
                        alarm: realData.alarm || false,
                        motor: realData.motor || false,
                        simMode: statusData.simMode || false
                    }
                });
            } catch (error) {
                console.error('获取数据失败:', error);
            }
        }
        
        // 获取阈值范围
        async function fetchThresholds() {
            try {
                const response = await fetch('/api/thresholds');
                const thresholds = await response.json();
                updateThresholdsDisplay(thresholds);
            } catch (error) {
                console.error('获取阈值失败:', error);
            }
        }
        
        // 提交阈值修改
        document.getElementById('update-thresholds').addEventListener('click', async () => {
            const payload = {
                co2: { min: parseFloat(document.getElementById('co2-min').value), max: parseFloat(document.getElementById('co2-max').value) },
                temp: { min: parseFloat(document.getElementById('temp-min').value), max: parseFloat(document.getElementById('temp-max').value) },
                hum: { min: parseFloat(document.getElementById('hum-min').value), max: parseFloat(document.getElementById('hum-max').value) },
                mq135: { min: parseFloat(document.getElementById('mq135-min').value), max: parseFloat(document.getElementById('mq135-max').value) }
            };
            try {
                const response = await fetch('/api/thresholds', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                if (response.ok) {
                    await fetchThresholds();
                    await fetchAllData();   // 重新获取数据以更新报警状态
                } else {
                    console.error('阈值更新失败');
                }
            } catch (error) {
                console.error('提交阈值失败:', error);
            }
        });
        
        // 提交模拟数据
        document.getElementById('send').addEventListener('click', async () => {
            const payload = {
                co2: Number(document.getElementById('co2').value),
                temp: Number(document.getElementById('temp').value),
                hum: Number(document.getElementById('hum').value),
                mq135: Number(document.getElementById('mq135').value)
            };
            
            try {
                await fetch('/api/sim', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                await fetchAllData();
            } catch (error) {
                console.error('提交数据失败:', error);
            }
        });
        
        // 切换数据模式
        async function setDataMode(simMode) {
            try {
                const response = await fetch('/api/mode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ simMode: simMode })
                });
                
                if (response.ok) {
                    await fetchAllData();
                }
            } catch (error) {
                console.error('切换模式失败:', error);
            }
        }
        
        document.getElementById('use-real-data').addEventListener('click', () => setDataMode(false));
        document.getElementById('use-sim-data').addEventListener('click', () => setDataMode(true));
        
        // 初始化：获取阈值、获取数据，并定时更新
        fetchThresholds();
        fetchAllData();
        setInterval(fetchAllData, 2000);
    </script>
</body>
</html>)HTML";

#define DHT_PIN   10
#define LED_PIN  LED_BUILTIN 
#define Motor_PIN 7

#define MQ135_AO_PIN 0
#define MQ135_DO_PIN 1

float RLOAD = 10.0;
float RZERO = 9.65;

float temp = 0.0f;
float hum = 0.0f;
bool motorState = false;
bool ledState = false;
bool alarmState = false;

DHT dht(DHT_PIN, DHT11);

float readMQ135Resistance() {
  int adc_value = analogRead(MQ135_AO_PIN);
  float voltage = adc_value * (3.3 / 4095.0);
  return (3.3 - voltage) / voltage * RLOAD;
}

float calculatePPM(float resistance) {
  return 116.6020682 * pow((resistance / RZERO), -2.769034857);
}

void setup() {
  Serial1.begin(9600, SERIAL_8N1, 5, 4);
  pinMode(LED_PIN, OUTPUT);
  pinMode(Motor_PIN, OUTPUT);
  pinMode(MQ135_AO_PIN, INPUT);
  pinMode(MQ135_DO_PIN, INPUT);
  dht.begin();
  Serial.begin(115200);

  if (WiFi.softAP(WIFI_SSID, WIFI_PASS, 1, 0, 10, false)) {
    Serial.println("AP 模式启动成功!");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP 地址: ");
    Serial.println(myIP);
  } else {
    Serial.println("AP 模式启动失败!");
    return;
  }

  // 网页路由
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });

  server.on("/api/sim", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["co2"] = simCO2;
    doc["temp"] = simTemp;
    doc["hum"] = simHum;
    doc["mq135"] = simMQ135;
    String s;
    serializeJson(doc, s);
    req->send(200, "application/json", s);
  });

  server.on("/api/real", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["co2"] = realCO2;
    doc["temp"] = realTemp;
    doc["hum"] = realHum;
    doc["mq135"] = mq135PPM;
    doc["alarm"] = alarmState;
    doc["motor"] = motorState;
    String s;
    serializeJson(doc, s);
    req->send(200, "application/json", s);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["simMode"] = isSimData;
    String s;
    serializeJson(doc, s);
    req->send(200, "application/json", s);
  });

  // 获取阈值范围
  server.on("/api/thresholds", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["co2"]["min"] = co2Min;
    doc["co2"]["max"] = co2Max;
    doc["temp"]["min"] = tempMin;
    doc["temp"]["max"] = tempMax;
    doc["hum"]["min"] = humMin;
    doc["hum"]["max"] = humMax;
    doc["mq135"]["min"] = mq135Min;
    doc["mq135"]["max"] = mq135Max;
    String s;
    serializeJson(doc, s);
    req->send(200, "application/json", s);
  });

  // 更新阈值范围
  server.on("/api/thresholds", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
    [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
      JsonDocument doc;
      if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
        // CO2
        if (doc["co2"]["min"].is<float>() && doc["co2"]["max"].is<float>()) {
          float minv = doc["co2"]["min"].as<float>();
          float maxv = doc["co2"]["max"].as<float>();
          if (minv <= maxv) { co2Min = minv; co2Max = maxv; }
        }
        // 温度
        if (doc["temp"]["min"].is<float>() && doc["temp"]["max"].is<float>()) {
          float minv = doc["temp"]["min"].as<float>();
          float maxv = doc["temp"]["max"].as<float>();
          if (minv <= maxv) { tempMin = minv; tempMax = maxv; }
        }
        // 湿度
        if (doc["hum"]["min"].is<float>() && doc["hum"]["max"].is<float>()) {
          float minv = doc["hum"]["min"].as<float>();
          float maxv = doc["hum"]["max"].as<float>();
          if (minv <= maxv) { humMin = minv; humMax = maxv; }
        }
        // MQ135
        if (doc["mq135"]["min"].is<float>() && doc["mq135"]["max"].is<float>()) {
          float minv = doc["mq135"]["min"].as<float>();
          float maxv = doc["mq135"]["max"].as<float>();
          if (minv <= maxv) { mq135Min = minv; mq135Max = maxv; }
        }

        JsonDocument out;
        out["ok"] = true;
        out["co2"]["min"] = co2Min;
        out["co2"]["max"] = co2Max;
        out["temp"]["min"] = tempMin;
        out["temp"]["max"] = tempMax;
        out["hum"]["min"] = humMin;
        out["hum"]["max"] = humMax;
        out["mq135"]["min"] = mq135Min;
        out["mq135"]["max"] = mq135Max;
        String s;
        serializeJson(out, s);
        req->send(200, "application/json", s);

        Serial.println("阈值范围已更新");
      } else {
        req->send(400, "application/json", "{\"error\":\"阈值格式错误\"}");
      }
    }
  );

  server.on("/api/sim", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
    [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
      JsonDocument doc;
      if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
        simCO2 = constrain(doc["co2"] | simCO2, 0, 5000);
        simTemp = constrain(doc["temp"] | simTemp, -40, 85);
        simHum = constrain(doc["hum"] | simHum, 0, 100);
        simMQ135 = constrain(doc["mq135"] | simMQ135, 0, 2000);

        JsonDocument out;
        out["ok"] = true;
        out["co2"] = simCO2;
        out["temp"] = simTemp;
        out["hum"] = simHum;
        out["mq135"] = simMQ135;
        String s;
        serializeJson(out, s);
        req->send(200, "application/json", s);

        Serial.print("模拟数据更新: CO2=");
        Serial.print(simCO2);
        Serial.print("ppm, 温度=");
        Serial.print(simTemp);
        Serial.print("°C, 湿度=");
        Serial.print(simHum);
        Serial.print("%, MQ135=");
        Serial.print(simMQ135);
        Serial.println("ppm");
      } else {
        req->send(400, "application/json", "{\"error\":\"数据格式错误\"}");
      }
    }
  );

  server.on("/api/mode", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
    [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (!error) {
        bool newMode = doc["simMode"];
        isSimData = newMode;

        JsonDocument out;
        out["ok"] = true;
        out["simMode"] = isSimData;
        String s;
        serializeJson(out, s);
        req->send(200, "application/json", s);

        Serial.print("数据模式切换: ");
        Serial.println(isSimData ? "模拟数据" : "真实数据");
      } else {
        req->send(400, "application/json", "{\"error\":\"模式切换失败\"}");
      }
    }
  );

  server.begin();
  Serial.println("初始化成功 - 等待传感器数据...");
}

void loop() {
  // 读取 JW01 CO2 传感器数据
  if (Serial1.available() >= 6) {
    byte packet[6];
    for (int i = 0; i < 6; i++) {
      packet[i] = Serial1.read();
    }
    realCO2 = packet[1] * 256 + packet[2];

    Serial.println("=== JW01 CO2 传感器读数 ===");
    Serial.print("CO2: ");
    Serial.print(realCO2);
    Serial.println(" ppm");
  }

  // 读取 MQ135 传感器数据
  float resistance = readMQ135Resistance();
  mq135PPM = calculatePPM(resistance);

  // 读取 DHT11 温湿度数据
  static uint32_t last_time = 0;
  if (millis() - last_time > 2000) {
    float temp_cur = dht.readTemperature();
    float hum_cur = dht.readHumidity();

    if (!isnan(temp_cur)) {
      temp = 0.8f * temp + 0.2f * temp_cur;
      realTemp = temp;
    }

    if (!isnan(hum_cur)) {
      hum = 0.8f * hum + 0.2f * hum_cur;
      realHum = hum;
    }

    Serial.printf("温度: %.2f°C, 湿度: %.2f%%\n", realTemp, realHum);
    last_time = millis();
  }

  // 报警逻辑 - 根据模式选择数据源
  float currentCO2, currentTemp, currentHum, currentMQ135;

  if (isSimData) {
    currentCO2 = simCO2;
    currentTemp = simTemp;
    currentHum = simHum;
    currentMQ135 = simMQ135;
  } else {
    currentCO2 = realCO2;
    currentTemp = realTemp;
    currentHum = realHum;
    currentMQ135 = mq135PPM;
  }

  // 报警条件判断：超出阈值范围即报警
  bool mq135Alarm = (currentMQ135 < mq135Min || currentMQ135 > mq135Max);
  bool co2Alarm   = (currentCO2   < co2Min   || currentCO2   > co2Max);
  bool tempAlarm  = (currentTemp  < tempMin  || currentTemp  > tempMax);
  bool humAlarm   = (currentHum   < humMin   || currentHum   > humMax);

  alarmState = (mq135Alarm || co2Alarm || tempAlarm || humAlarm);

  // 控制电机
  if (alarmState) {
    digitalWrite(Motor_PIN, HIGH);
    motorState = true;
    Serial.println("🚨 报警：环境参数超出阈值范围!");
    if (mq135Alarm) Serial.println("  - MQ135气体浓度超出范围");
    if (co2Alarm) Serial.println("  - CO2浓度超出范围");
    if (tempAlarm) Serial.println("  - 温度超出范围");
    if (humAlarm) Serial.println("  - 湿度超出范围");
  } else {
    digitalWrite(Motor_PIN, LOW);
    motorState = false;
    Serial.println("✅ 环境参数正常");
  }

  Serial.print("数据模式: ");
  Serial.println(isSimData ? "模拟数据" : "真实数据");
  Serial.print("当前MQ135值: ");
  Serial.print(currentMQ135);
  Serial.println(" ppm");
  Serial.println();

  // LED 闪烁
  static bool led_status = false;
  digitalWrite(LED_PIN, led_status);
  ledState = led_status;
  led_status = !led_status;

  delay(200);
}