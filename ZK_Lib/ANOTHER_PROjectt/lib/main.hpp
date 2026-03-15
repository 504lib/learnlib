#ifndef _MAIN_HPP
#define _MAIN_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <array>
#include "../lib/dht11.hpp"
#include "../lib/Webserver_router.hpp"
#include "../lib/MQ4_calculate.hpp"

#define LED_BUILTIN2 13           // 第二个LED引脚
#define MQ4_AO_PIN 0              // MQ-4 模拟输出引脚
#define MQ4_DO_PIN 1              // MQ-4 数字输出引脚  
#define MOTOR_PIN 7                // 电机控制引脚
#define LIGHT_BULB_PIN 6           // 灯泡控制引脚

struct Alarm_Flag
{
  bool temp_alarm;                // 温度预警标志
  bool hum_alarm;                 // 湿度预警标志 
  bool mq4_alarm;                 // MQ-4 预警标志
};

struct Use_Flag
{
  bool isLEDUsed;                 // LED 是否被使用
  bool isMotorUsed;               // 用户电机控制位
  bool isMotorForceOn;            // 安全策略强制开电机位
};

struct Threshold
{
    float temp_threshold;           // 温度预警阈值
    float hum_threshold;            // 湿度预警阈值
    float mq4_threshold;            // MQ-4 预警阈值，单位为ppm
};


struct Data_Monitor
{
  volatile float temperature;     // 温度监测数据
  volatile float humidity;        // 湿度监测数据
  volatile float mq4_ppm;         // MQ-4 监测数据，单位为ppm
};



#endif 