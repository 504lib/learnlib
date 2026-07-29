#pragma once

#include <stdint.h>

#define GRAY_BITS 8  

float CalculateGrayError_Advanced(uint8_t gray_byte);
void Grayscale_SetWeight(float bit_weight_value,uint8_t bit_loaction);
