#include "../lib/main.hpp"
#include <Preferences.h>


void wifi_connect();
void Get_data();

// 全局变量定义
Alarm_Flag alarm_flag = {false, false, false};
Use_Flag use_flag = {false, false};
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

// 报警上升沿触发后交替切换电机强制状态：第一次强制关，第二次强制开。
static bool last_alarm_on = false;
static bool alarm_force_motor_off = false;
static bool alarm_force_motor_on = false;
static uint32_t alarm_rise_count = 0;

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
  Serial.print("Message arrived [");
  Serial.print(topic);//打印主题
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);//打印消息内容
  }
  Serial.println();//换行
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

    // 处理 params 中的 LED1，兼容 "LED1": true / "LED1": {"value": true} / "LED1": 1 等
    JsonVariant v = params["LED1"];
    if (!v.isNull()) {
      bool newState = false;
      if (v.is<bool>()) {
        newState = v.as<bool>();
      } else if (v.is<int>()) {
        newState = (v.as<int>() != 0);
      } else if (v.is<const char*>()) {
        const char* s = v.as<const char*>();
        newState = (strcmp(s, "true") == 0 || strcmp(s, "1") == 0);
      } else if (v.is<JsonObject>() && v["value"].is<bool>()) {
        newState = v["value"].as<bool>();
      }
      JsonVariant bulb_v = params["bulb"];
      if (!bulb_v.isNull())
      {
        bool bulbState = false;
        if (bulb_v.is<bool>()) {
          bulbState = bulb_v.as<bool>();
        } else if (bulb_v.is<int>()) {
          bulbState = (bulb_v.as<int>() != 0);
        } else if (bulb_v.is<const char*>()) {
          const char* s = bulb_v.as<const char*>();
          bulbState = (strcmp(s, "true") == 0 || strcmp(s, "1") == 0);
        } else if (bulb_v.is<JsonObject>() && bulb_v["value"].is<bool>()) {
          bulbState = bulb_v["value"].as<bool>();
        }
        use_flag.isLEDUsed = bulbState;
        printf("Set Bulb -> %s\n", bulbState ? "ON" : "OFF");
      }
      

      bool LED1_status = newState;
      digitalWrite(LED_BUILTIN2, LED1_status ? HIGH : LOW);
      Serial.print("Set LED1 -> ");
      Serial.println(LED1_status ? "ON" : "OFF");
    } else {
      Serial.println("No LED1 in params");
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
    Serial.println(reply);
    delay(100);  
    if (client.publish(ONENET_TOPIC_PROP_SET_REPLY, reply)) {
      Serial.println("Send set reply success!");
    } else {
      Serial.println("Send set reply failed!");
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

    Serial.print("Post reply -> id: ");
    Serial.print(id);
    Serial.print("  code: ");
    Serial.print(code);
    Serial.print("  msg: ");
    Serial.println(info);
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
  client.setCallback(callback);//设置MQTT消息回调函数，当接收到MQTT消息时，调用callback函数处理消息
}

void wifi_connect()
{
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    if (!iswifiConfigured) {
      iswifiConfigured = true;
      Serial.print("WiFi connected, STA IP: ");
      Serial.println(WiFi.localIP());
    }
    return;
  }

  iswifiConfigured = false;
  if (Connect_SSID[0] == '\0' || Connect_PASS[0] == '\0') {
    Serial.println("WiFi not configured yet. Please connect to the AP and send credentials.");
    return;
  }

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

    float resistance = readMQ135Resistance(MQ4_AO_PIN);                                         // 读取MQ-4电阻值
    data_monitor.mq4_ppm = calculatePPM(resistance);                                  // 计算MQ-4的ppm值
    alarm_flag.hum_alarm = (data_monitor.humidity > threshold.hum_threshold);              // 更新湿度预警标志
    alarm_flag.temp_alarm = (data_monitor.temperature > threshold.temp_threshold);        // 更新温度预警标志
    alarm_flag.mq4_alarm = (data_monitor.mq4_ppm > threshold.mq4_threshold);              // 更新MQ-4预警标志
    bool alarm_on = alarm_flag.hum_alarm || alarm_flag.temp_alarm || alarm_flag.mq4_alarm;    // 计算总预警状态

    // 仅在报警上升沿时改变一次强制策略，避免报警持续期间反复抖动。
    if (alarm_on && !last_alarm_on) {
      alarm_rise_count++;
      if (alarm_rise_count % 2 == 1) {
        alarm_force_motor_off = true;
        alarm_force_motor_on = false;
      } else {
        alarm_force_motor_on = true;
        alarm_force_motor_off = false;
      }
    }
    last_alarm_on = alarm_on;

    // 用户端拥有最高权限：用户命令存在时，直接覆盖报警强制状态。
    bool final_motor_on = use_flag.isMotorUsed;
    if (!use_flag.isMotorUsed) {
      if (alarm_force_motor_on) {
        final_motor_on = true;
      }
      if (alarm_force_motor_off) {
        final_motor_on = false;
      }
    } else {
      alarm_force_motor_on = false;
      alarm_force_motor_off = false;
    }
    
    // digitalWrite(bulb, use_flag.isLEDUsed );                             // 控制第二个LED状态
    digitalWrite(LIGHT_BULB_PIN,use_flag.isLEDUsed);                             // 控制灯泡状态
    digitalWrite(MOTOR_PIN, final_motor_on ? HIGH : LOW);                                  // 控制电机状态

    digitalWrite(LED_BUILTIN, ledState);                                    // 板载LED闪烁
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
    OneNET_Prop_Post(client, data_monitor.temperature, data_monitor.humidity, use_flag.isLEDUsed);
    last_prop_post_ms = millis();
  }
  
  client.loop();
  
}
