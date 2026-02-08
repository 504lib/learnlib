#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN   10              // DHT11 引脚
#define LED_BUILTIN2 13           // 第二个LED引脚
#define MQ4_AO_PIN 0              // MQ-4 模拟输出引脚
#define MQ4_DO_PIN 1              // MQ-4 数字输出引脚  

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
Use_Flag use_flag = {false, false};
Data_Monitor data_monitor = {0.0f, 0.0f, 0.0f};
Data_Sim data_sim = {0.0f, 0.0f,0.0f};

DHT dht(DHT_PIN,DHT11);

float RLOAD = 10.0;               // MQ-4 负载电阻，单位为kΩ，根据实际情况调整
float RZERO = 9.65;               // MQ-4 在100ppm甲烷时的电阻，单位为kΩ，根据实际情况调整
static bool ledState = LOW;       // LED 状态变量
float temp = 0.0f;
float hum = 0.0f;


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
  pinMode(MQ4_AO_PIN,INPUT);
  pinMode(MQ4_DO_PIN,INPUT);
  dht.begin();
}

void loop() {
  ledState = !ledState;
  static uint32_t last_time = 0;
  if(millis() - last_time > 500) {
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
    Serial.printf("LED State: %s\n", ledState ? "ON" : "OFF");
    Serial.printf("MQ-4 PPM: %.2f\n", data_monitor.mq4_ppm);

    Serial.printf("温度: %.2f°C, 湿度: %.2f%%\n", data_monitor.temperature, data_monitor.humidity);
    last_time = millis();                                                            // 更新上次读取时间
  }
}
