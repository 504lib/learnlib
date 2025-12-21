// dht11.h
#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

typedef struct
{
  uint8_t humidity_int;
  uint8_t humidity_dec;
  uint8_t temperature_int;
  uint8_t temperature_dec;
} DHT11_Data_TypeDef;

void DHT11_ReadData(DHT11_Data_TypeDef *data);

#endif