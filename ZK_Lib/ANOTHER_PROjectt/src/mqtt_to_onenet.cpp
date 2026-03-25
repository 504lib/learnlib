#include "../lib/mqtt_to_onenet.hpp"
#include <math.h>

static float quantizeToTenth(float value) {
  return roundf(value * 10.0f) / 10.0f;
}

int postMsgId = 0;//消息ID，消息ID是需要改变的，每次上报时属性递增
static uint32_t last_mqtt_retry_ms = 0;

bool OneNET_Connect(PubSubClient& client){
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  uint32_t now_ms = millis();
  if (last_mqtt_retry_ms != 0 && (now_ms - last_mqtt_retry_ms) < 5000) {
    return false;
  }

  last_mqtt_retry_ms = now_ms;
  client.setServer(MQTT_SERVER, MQTT_PORT);//设置MQTT服务器地址和端口
  Serial.println("Connecting to MQTT...");
  if (!client.connect(device_id, product_id, token)) {
    Serial.print("Failed to connect to MQTT server, state: ");
    Serial.println(client.state());
    return false;
  }

  Serial.println("Connected to MQTT server");
  client.subscribe(ONENET_TOPIC_PROP_SET);//订阅属性设置主题，接收来自OneNET的属性设置请求，OneNET--->设备
  client.subscribe(ONENET_TOPIC_PROP_POST_REPLY);//订阅属性设置回复主题，接收来自OneNET的属性设置回复，OneNET--->设备
  return true;
}


void OneNET_Prop_Post(PubSubClient& client,DataProvider provider,Threshold threshold,Alarm_Flag alarm_flag) {
  if(client.connected())
  {
    JsonDocument root;
    int currentMsgId = postMsgId++;

    float tempValue = quantizeToTenth(provider.temperature);
    float humiValue = quantizeToTenth(provider.humidity);
    float mq4Value = quantizeToTenth(provider.mq4_ppm);
    float tempThresholdValue = quantizeToTenth(threshold.temp_threshold);
    float humiThresholdValue = quantizeToTenth(threshold.hum_threshold);
    float mq4ThresholdValue = quantizeToTenth(threshold.mq4_threshold);

    root["id"] = String(currentMsgId);
    root["version"] = "1.0";

    JsonObject params = root["params"].to<JsonObject>();
    params["temp"]["value"] = tempValue;
    params["humi"]["value"] = humiValue;
    params["mq4"]["value"] = mq4Value;
    params["bulb"]["value"] = provider.bulb_status;
    params["Motor"]["value"] = provider.motor_status;
    params["temp_threshold"]["value"] = tempThresholdValue;
    params["humi_threshold"]["value"] = humiThresholdValue;
    params["mq4_threshold"]["value"] = mq4ThresholdValue;
    params["temp_alarm"]["value"] = alarm_flag.temp_alarm;
    params["hum_alarm"]["value"] = alarm_flag.hum_alarm;
    params["mq4_alarm"]["value"] = alarm_flag.mq4_alarm;

    String payload;
    serializeJson(root, payload);

    if(client.publish(ONENET_TOPIC_PROP_POST, payload.c_str()))//上报属性
    {
      Serial.printf("[TX_OK] id=%d\n", currentMsgId);
    }
    else
    {
      Serial.printf("[TX_FAIL] id=%d len=%u state=%d\n", currentMsgId, payload.length(), client.state());
    }
  }
}
