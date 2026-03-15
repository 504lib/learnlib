#include "../lib/mqtt_to_onenet.hpp"

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


void OneNET_Prop_Post(PubSubClient& client,float temp,float humi,bool Bulb_status) {
  if(client.connected())
  {
    char parmas[256];//属性参数
    char jsonBuf[256];//JSON字符串缓冲区，用于上报属性的缓存区
    snprintf(
      parmas,
      sizeof(parmas),
      "{\"temp\":{\"value\":%.1f},\"humi\":{\"value\":%.1f},\"LED1\":{\"value\":%s},\"bulb\":{\"value\":%s}}",
      temp,
      humi,
      Bulb_status ? "true" : "false",
      Bulb_status ? "true" : "false"
    );//构造属性上报的参数，格式为JSON字符串
    Serial.println(parmas);
    snprintf(jsonBuf, sizeof(jsonBuf), ONENET_TOPIC_PROP_FORMAT, postMsgId++, parmas);//构造属性上报的JSON字符串，格式为{"id":"<postMsgId>","version":"1.0","params":<parmas>}
    Serial.println(jsonBuf);
    if(client.publish(ONENET_TOPIC_PROP_POST, jsonBuf))//上报属性
    {
      Serial.println("Post property success!");
    }
    else
    {
      Serial.println("Post property failed!");
    }
  }
}
