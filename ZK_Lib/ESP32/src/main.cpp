#include <Arduino.h>
#include "./protocol/protocol.hpp"

#define LED_Pin GPIO_NUM_48

protocol uart_protocol(0xAA,0x55,0x0D,0x0A);

void setup() 
{
  pinMode(LED_Pin,OUTPUT);
}

void loop() 
{
  uart_protocol.Send_Uart_Frame(5);
  digitalWrite(LED_Pin,0);
  delay(500);
  uart_protocol.Send_Uart_Frame((float)5.1);
  digitalWrite(LED_Pin,1);
  delay(500);
  uart_protocol.Send_Uart_Frame_ACK();
  delay(500);
}