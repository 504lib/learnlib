#include <Arduino.h>
#include "./protocol/protocol.hpp"

#define LED_Pin GPIO_NUM_48

static int count = 0;
static float fcount = 0.0;

bool Flag_INT = false;
bool Flag_FLOAT = false;
bool Flag_ACK = false;

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);

void IRAM_ATTR handleInterrupt1() {
  Flag_INT = true;
}

void IRAM_ATTR handleInterrupt2() {
  
  Flag_FLOAT = true;
}


void IRAM_ATTR handleInterrupt3() {
  Flag_ACK = true;
}


void setup() 
{
  
  pinMode(LED_Pin,OUTPUT);
  pinMode(11, INPUT); // 设置为输入
  pinMode(12, INPUT); // 设置为输入
  pinMode(13, INPUT); // 设置为输入
  attachInterrupt(11, handleInterrupt1, RISING); // 上升沿触发
  attachInterrupt(12, handleInterrupt2, RISING); // 上升沿触发
  attachInterrupt(13, handleInterrupt3, RISING); // 上升沿触发
}

void loop() 
{
  if(Serial1.available())
  {
    // uart_protocol.Receive_Uart_Frame(Serial1.read());
    Serial.write(Serial1.read());
  }
  if (Flag_INT)
  {
    uart_protocol.Send_Uart_Frame(count++);
    // Serial.println(count++);
    Flag_INT = false;
  }
  if (Flag_FLOAT)
  {
    uart_protocol.Send_Uart_Frame(fcount);
   fcount += 0.1;
    // Serial.println(fcount, 1);
    Flag_FLOAT = false;
  }
  if (Flag_ACK)
  {
    uart_protocol.Send_Uart_Frame_ACK();
    // Serial.println("ACK");
    Flag_ACK = false;
  }
  
}