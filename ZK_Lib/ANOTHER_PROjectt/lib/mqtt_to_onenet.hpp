#ifndef __MQTT_TO_ONENET_HPP
#define __MQTT_TO_ONENET_HPP

#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "main.hpp"

#define MQTT_SERVER "mqtts.heclouds.com"
#define MQTT_PORT   1883

#define product_id  "82aJXC88GG"//产品ID
#define device_id   "DHT11"//设备ID
#define token       "version=2018-10-31&res=products%2F82aJXC88GG%2Fdevices%2FDHT11&et=1899638412&method=md5&sign=PR60sbPIua0jPSWPmtS40w%3D%3D"

#define ONENET_TOPIC_PROP_POST "$sys/" product_id "/" device_id "/thing/property/post" 
//设备属性上报请求，设备--->OneNET
#define ONENET_TOPIC_PROP_SET "$sys/" product_id "/" device_id "/thing/property/set" 
//设备属性上报请求，OneNET--->设备
#define ONENET_TOPIC_PROP_POST_REPLY "$sys/" product_id "/" device_id "/thing/property/post/reply" 
//设备属性上报请求，OneNET--->设备
#define ONENET_TOPIC_PROP_SET_REPLY "$sys/" product_id "/" device_id "/thing/property/set_reply" 
//设备属性上报请求，设备--->OneNET
#define ONENET_TOPIC_PROP_FORMAT "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":%s}"


bool OneNET_Connect(PubSubClient& client);
void OneNET_Prop_Post(PubSubClient& client,float temp,float humi,bool Bulb_status);

#endif // !1