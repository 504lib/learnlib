#include "../lib/main.hpp"
#include "../lib/mqtt_to_onenet.hpp"
#include <Preferences.h>


void wifi_connect();
void Get_data();

// 全局变量定义
DataProvider data_provider = {0.0f, 0.0f,0.0f, false, false};
Alarm_Flag alarm_flag = {false, false, false};
Use_Flag use_flag = {false, false, false};
Data_Monitor data_monitor = {0.0f, 0.0f, 0.0f};
Threshold threshold = {30.0f, 60.0f, 200.0f};

DHT dht(DHT_PIN,DHT11);

WiFiClient espClient;//创建一个WiFiClient对象，用于连接MQTT服务器
PubSubClient client(espClient);//创建一个PubSubClient对象，并将WiFiClient对象作为参数传入

static bool ledState = LOW;       // LED 状态变量

const char* WIFI_SSID = "configure_wifi"; 
const char* WIFI_PASS = "12345678";

Ticker dataTicker(Get_data, 200, 0, MILLIS); // 每200ms调用一次 Get_data 函数
Ticker wifiTicker(wifi_connect, 2000, 0, MILLIS); // 每2秒调用一次 wifi_connect 函数

std::array<char,32> Connect_SSID;
std::array<char,32> Connect_PASS;

volatile bool iswifiConfigured = false;
static uint32_t last_wifi_retry_ms = 0;
static bool wifiNoCredLogged = false;
static uint32_t mq4_warmup_start_ms = 0;
static bool mq4WarmupLogged = false;
static bool mq4InvalidLogged = false;

static constexpr uint32_t MQ4_WARMUP_MS = 90000;     // MQ4 预热静默期
static constexpr float MQ4_MAX_VALID_PPM = 5000.0f;  // 过大值保护上限

AsyncWebServer server(80);
Preferences wifiPrefs;
static bool wifiPrefsReady = false;

static void init_wifi_storage()
{
  wifiPrefsReady = wifiPrefs.begin("wifi", false);
  if (!wifiPrefsReady) {
    Serial.println("NVS init failed");
  }
}

static void load_wifi_credentials_from_nvs()
{
  if (!wifiPrefsReady) {
    return;
  }

  String ssid = wifiPrefs.getString("ssid", "");
  String pass = wifiPrefs.getString("pwd", "");
  if (ssid.length() == 0 || pass.length() == 0) {
    return;
  }

  ssid.toCharArray(Connect_SSID.data(), Connect_SSID.size());
  pass.toCharArray(Connect_PASS.data(), Connect_PASS.size());
  Serial.printf("Loaded WiFi credentials from NVS - SSID: %s\n", Connect_SSID.data());
}

static void save_wifi_credentials_to_nvs(const String& ssid, const String& pass)
{
  if (!wifiPrefsReady) {
    return;
  }

  String oldSsid = wifiPrefs.getString("ssid", "");
  String oldPass = wifiPrefs.getString("pwd", "");
  if (oldSsid == ssid && oldPass == pass) {
    return;
  }

  wifiPrefs.putString("ssid", ssid);
  wifiPrefs.putString("pwd", pass);
}

static void clear_wifi_credentials()
{
  client.disconnect();
  if (wifiPrefsReady) {
    wifiPrefs.remove("ssid");
    wifiPrefs.remove("pwd");
  }
  Connect_SSID.fill('\0');
  Connect_PASS.fill('\0');
  iswifiConfigured = false;
  last_wifi_retry_ms = 0;
}



void callback(char* topic,byte* payload,unsigned int length){
  if(strcmp(topic,ONENET_TOPIC_PROP_SET) == 0)//如果收到的消息是属性设置请求
  {
    JsonDocument doc; // 使用 JsonDocument（大小根据实际 payload 调整）
    DeserializationError err = deserializeJson(doc, payload, length); // 使用 length 重载
    if (err) {
      Serial.print("deserializeJson failed: ");
      Serial.println(err.c_str());
      return;
    }

    JsonObject msg = doc.as<JsonObject>();
    JsonObject params = msg["params"].as<JsonObject>();

    auto readBoolParam = [](JsonVariantConst value, bool& out) -> bool {
      if (value.isNull()) {
        return false;
      }

      JsonVariantConst actual = value;
      if (value.is<JsonObjectConst>() && !value["value"].isNull()) {
        actual = value["value"].as<JsonVariantConst>();
      }

      if (actual.is<bool>()) {
        out = actual.as<bool>();
        return true;
      }
      if (actual.is<int>()) {
        out = (actual.as<int>() != 0);
        return true;
      }
      if (actual.is<const char*>()) {
        const char* s = actual.as<const char*>();
        out = (strcmp(s, "true") == 0 || strcmp(s, "1") == 0);
        return true;
      }
      return false;
    };

    auto readFloatParam = [](JsonVariantConst value, float& out) -> bool {
      if (value.isNull()) {
        return false;
      }

      JsonVariantConst actual = value;
      if (value.is<JsonObjectConst>() && !value["value"].isNull()) {
        actual = value["value"].as<JsonVariantConst>();
      }

      if (actual.is<float>() || actual.is<double>() || actual.is<int>()) {
        out = actual.as<float>();
        return true;
      }
      if (actual.is<const char*>()) {
        out = atof(actual.as<const char*>());
        return true;
      }
      return false;
    };

    // 处理 params 中的 LED1，兼容 "LED1": true / "LED1": {"value": true} / "LED1": 1 等
    JsonVariant v = params["LED1"];
    if (!v.isNull()) {
      bool newState = false;
      if (readBoolParam(v, newState)) {
        bool LED1_status = newState;
        digitalWrite(LED_BUILTIN2, LED1_status ? HIGH : LOW);
        Serial.print("Set LED1 -> ");
        Serial.println(LED1_status ? "ON" : "OFF");
      } else {
        Serial.println("Invalid LED1 value");
      }
    } else {
      Serial.println("No LED1 in params");
    }

    JsonVariant bulb_v = params["bulb"];
    if (!bulb_v.isNull())
    {
      bool bulbState = false;
      if (readBoolParam(bulb_v, bulbState)) {
        use_flag.isLEDUsed = bulbState;
        printf("Set Bulb -> %s\n", bulbState ? "ON" : "OFF");
      } else {
        Serial.println("Invalid bulb value");
      }
    }
    else
    {
      Serial.println("No bulb in params");
    }
    JsonVariant Motor_v = params["Motor"];
    if (!Motor_v.isNull())
    {
      bool MotorState = false;
      if (readBoolParam(Motor_v, MotorState)) {
        use_flag.isMotorUsed = MotorState;
        printf("Set Motor -> %s\n", MotorState ? "ON" : "OFF");
      } else {
        Serial.println("Invalid Motor value");
      }
    }
    else
    {
      Serial.println("No Motor in params");
    }

    JsonVariant temp_threshold_v = params["temp_threshold"];
    if (!temp_threshold_v.isNull())
    {
      float tempThreshold = 0.0f;
      if (readFloatParam(temp_threshold_v, tempThreshold))
      {
        threshold.temp_threshold = tempThreshold;
        Serial.printf("Set temp_threshold -> %.1f\n", threshold.temp_threshold);
      }
      else
      {
        Serial.println("Invalid temp_threshold value");
      }
    }
    else
    {
      Serial.println("No temp_threshold in params");
    }
    JsonVariant humi_threshold_v = params["humi_threshold"];
    if (!humi_threshold_v.isNull())
    {
      float humiThreshold = 0.0f;
      if (readFloatParam(humi_threshold_v, humiThreshold))
      {
        threshold.hum_threshold = humiThreshold;
        Serial.printf("Set humi_threshold -> %.1f\n", threshold.hum_threshold);
      }
      else
      {
        Serial.println("Invalid humi_threshold value");
      }
    }
    else
    {
      Serial.println("No humi_threshold in params");
    }
    JsonVariant mq4_threshold_v = params["mq4_threshold"];
    if (!mq4_threshold_v.isNull())
    {
      float mq4Threshold = 0.0f;
      if (readFloatParam(mq4_threshold_v, mq4Threshold))
      {
        threshold.mq4_threshold = mq4Threshold;
        Serial.printf("Set mq4_threshold -> %.1f\n", threshold.mq4_threshold);
      }
      else
      {
        Serial.println("Invalid mq4_threshold value");
      }
    }
    else
    {
      Serial.println("No mq4_threshold in params");
    }



    // 构造回复（如果带 id 则带上）
    const char* id_c = msg["id"];
    String id = id_c ? String(id_c) : String("");
    char reply[128];
    if (id.length()) {
      snprintf(reply, sizeof(reply), "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", id.c_str());
    } else {
      snprintf(reply, sizeof(reply), "{\"code\":200,\"msg\":\"success\"}");
    }
    delay(100);  
    if (client.publish(ONENET_TOPIC_PROP_SET_REPLY, reply)) {
      Serial.println("[SET_REPLY_OK]");
    } else {
      Serial.println("[SET_REPLY_FAIL]");
    }
  }
  else if(strcmp(topic,ONENET_TOPIC_PROP_POST_REPLY) == 0)//如果收到的消息是属性设置回复
  {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
      Serial.print("post_reply deserializeJson failed: ");
      Serial.println(err.c_str());
      return;
    }

    JsonObject msg = doc.as<JsonObject>();
    const char* id = msg["id"] | "";
    int code = msg["code"] | 0;
    const char* info = msg["msg"] | "";

    if (code != 200) {
      Serial.print("[POST_REPLY_ERR] id=");
      Serial.print(id);
      Serial.print(" code=");
      Serial.print(code);
      Serial.print(" msg=");
      Serial.println(info);
    }
  }
}

void setup() 
{
  dataTicker.start();
  wifiTicker.start();
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN2, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LIGHT_BULB_PIN, OUTPUT);
  pinMode(MQ4_AO_PIN,INPUT);
  pinMode(MQ4_DO_PIN,INPUT);
  Connect_SSID.fill('\0');
  Connect_PASS.fill('\0');
  init_wifi_storage();
  load_wifi_credentials_from_nvs();
  mq4_warmup_start_ms = millis();
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
    request->send_P(200, "text/html;charset=utf-8", WIFI_CONFIG_HTML);
  });

  server.on("/wifi_status", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument jsonDoc;
    bool configured = (Connect_SSID[0] != '\0' && Connect_PASS[0] != '\0');
    bool connected = (WiFi.status() == WL_CONNECTED);

    jsonDoc["configured"] = configured;
    jsonDoc["connected"] = connected;
    jsonDoc["ap_ssid"] = WIFI_SSID;
    jsonDoc["ap_ip"] = WiFi.softAPIP().toString();
    jsonDoc["target_ssid"] = configured ? String(Connect_SSID.data()) : String("");
    jsonDoc["sta_ip"] = connected ? WiFi.localIP().toString() : String("0.0.0.0");

    String jsonString;
    serializeJson(jsonDoc, jsonString);
    request->send(200, "application/json", jsonString);
  });

  server.on("/connect_ssid",HTTP_POST,[](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String pass = request->getParam("pass", true)->value();

      if (ssid.length() == 0 || pass.length() == 0) {
        request->send(400, "text/plain", "SSID or password is empty");
        return;
      }

      save_wifi_credentials_to_nvs(ssid, pass);

      // 重新配网前，先断开当前 STA 并清理旧状态/凭证。
      WiFi.disconnect(true, true);
      Connect_SSID.fill('\0');
      Connect_PASS.fill('\0');
      iswifiConfigured = false;
      last_wifi_retry_ms = 0;

      ssid.toCharArray(Connect_SSID.data(), Connect_SSID.size());
      pass.toCharArray(Connect_PASS.data(), Connect_PASS.size());
      Serial.printf("Received WiFi credentials - SSID: %s\n", Connect_SSID.data());
      request->send(200, "text/plain", "Credentials received, reconnecting...");
    }
    else
    {
      request->send(400, "text/plain", "Missing ssid or pass parameter");
    }
  });

  server.on("/clear_wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    WiFi.disconnect(true, true);
    clear_wifi_credentials();
    request->send(200, "text/plain", "STA disconnected and credentials cleared");
  });

  server.on("/WifiConfigure",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send_P(200, "text/html;charset=utf-8", WIFI_CONFIG_HTML);
  });
  server.begin();
  client.setBufferSize(512);
  client.setCallback(callback);//设置MQTT消息回调函数，当接收到MQTT消息时，调用callback函数处理消息
}

void wifi_connect()
{
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifiNoCredLogged = false;
    if (!iswifiConfigured) {
      iswifiConfigured = true;
      Serial.print("WiFi connected, STA IP: ");
      Serial.println(WiFi.localIP());
    }
    return;
  }

  iswifiConfigured = false;
  if (Connect_SSID[0] == '\0' || Connect_PASS[0] == '\0') {
    if (!wifiNoCredLogged) {
      Serial.println("[WIFI] waiting credentials");
      wifiNoCredLogged = true;
    }
    return;
  }
  wifiNoCredLogged = false;

  uint32_t now_ms = millis();
  if (last_wifi_retry_ms != 0 && (now_ms - last_wifi_retry_ms) < 10000) {
    return;
  }

  last_wifi_retry_ms = now_ms;
  Serial.printf("Connecting to WiFi SSID: %s\n", Connect_SSID.data());
  WiFi.begin(Connect_SSID.data(), Connect_PASS.data());
}

void Get_data()
{
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

    uint32_t now_ms = millis();
    bool mq4_warmed = (now_ms - mq4_warmup_start_ms) >= MQ4_WARMUP_MS;
    if (!mq4_warmed && !mq4WarmupLogged) {
      Serial.println("[MQ4] warming up, ppm protection enabled");
      mq4WarmupLogged = true;
    }

    float resistance = readMQ135Resistance(MQ4_AO_PIN);                                         // 读取MQ-4电阻值
    float mq4_ppm_raw = calculatePPM(resistance);                                                // 计算MQ-4的ppm值
    bool mq4_valid = mq4_warmed && isfinite(mq4_ppm_raw) && mq4_ppm_raw >= 0.0f && mq4_ppm_raw <= MQ4_MAX_VALID_PPM;

    if (mq4_valid) {
      data_monitor.mq4_ppm = mq4_ppm_raw;
      if (mq4InvalidLogged) {
        Serial.println("[MQ4] reading back to normal");
        mq4InvalidLogged = false;
      }
    } else {
      if (mq4_warmed && !mq4InvalidLogged) {
        Serial.printf("[MQ4] invalid reading ignored raw=%.1f\n", mq4_ppm_raw);
        mq4InvalidLogged = true;
      }
    }

    alarm_flag.hum_alarm = (data_monitor.humidity >= threshold.hum_threshold);              // 更新湿度预警标志
    alarm_flag.temp_alarm = (data_monitor.temperature >= threshold.temp_threshold);        // 更新温度预警标志
    alarm_flag.mq4_alarm = mq4_valid && (data_monitor.mq4_ppm >= threshold.mq4_threshold); // 更新MQ-4预警标志
    bool alarm_on = alarm_flag.hum_alarm || alarm_flag.temp_alarm || alarm_flag.mq4_alarm;    // 计算总预警状态

    bool motor_is_high = (digitalRead(MOTOR_PIN) == HIGH);
    if (alarm_on && !motor_is_high) {
      use_flag.isMotorForceOn = true;
    }
    if (!alarm_on) {
      use_flag.isMotorForceOn = false;
    }
    bool final_motor_on = use_flag.isMotorUsed || use_flag.isMotorForceOn;
    
    // digitalWrite(bulb, use_flag.isLEDUsed );                             // 控制第二个LED状态
    digitalWrite(LIGHT_BULB_PIN,use_flag.isLEDUsed);                             // 控制灯泡状态
    digitalWrite(MOTOR_PIN, final_motor_on ? HIGH : LOW);                                  // 控制电机状态

    digitalWrite(LED_BUILTIN, ledState);                                    // 板载LED闪烁
    data_provider.temperature = data_monitor.temperature;
    data_provider.humidity = data_monitor.humidity;
    data_provider.mq4_ppm = data_monitor.mq4_ppm;
    data_provider.bulb_status = use_flag.isLEDUsed;
    data_provider.motor_status = (digitalRead(MOTOR_PIN) == HIGH);
}

void loop() {
  static uint32_t last_prop_post_ms = 0;
  dataTicker.update();
  wifiTicker.update();

  if (WiFi.status() != WL_CONNECTED) {
    if (client.connected()) {
      client.disconnect();
    }
  } else if (!client.connected()) {
    OneNET_Connect(client);
  }

  if (client.connected() && millis() - last_prop_post_ms > 2000)
  {
    OneNET_Prop_Post(client, data_provider,threshold,alarm_flag);
    last_prop_post_ms = millis();
  }
  
  client.loop();
  
}
