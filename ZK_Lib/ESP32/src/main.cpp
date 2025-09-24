#include <Arduino.h>
#include "./protocol/protocol.hpp"

#define LED_Pin GPIO_NUM_48

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);

void IRAM_ATTR handleInterrupt1() {
  static int count = 0;
  uart_protocol.Send_Uart_Frame(count++);  
}

void IRAM_ATTR handleInterrupt2() {
  static float fcount = 0.0;
  fcount += 1.1;
  uart_protocol.Send_Uart_Frame(fcount);  
}


void IRAM_ATTR handleInterrupt3() {
  uart_protocol.Send_Uart_Frame_ACK();  
}


void setup() 
{
  Serial1.begin(115200, SERIAL_8N1, 16, 17);
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
    uart_protocol.Receive_Uart_Frame(Serial1.read());
    // Serial.write(Serial1.read());
  }
  // else
  // {
  //   Serial.println("test");
  // }
}