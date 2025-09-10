#pragma once

#include "u8g2.h"
#include "main.h"
#include "spi.h"
#include "freertos.h"
#include "cmsis_os2.h"
#include <stdio.h>

void draw(u8g2_t *u8g2);
uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void u8g2Init(u8g2_t* u8g2);
void updateProgressBar(uint8_t* progress,uint8_t step,uint8_t goal_progress);
void drawProgressBar(u8g2_t *u8g2,uint8_t progress);
