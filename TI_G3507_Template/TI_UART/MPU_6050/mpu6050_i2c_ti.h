/*
 * mpu6050_i2c_ti.h  ——  STM32 HAL → TI MSPM0 DL 适配层
 * 把 mpu6050.c 底层的 HAL 调用翻译成 TI DL 的 I2C 操作
 */
#ifndef __MPU6050_I2C_TI_H__
#define __MPU6050_I2C_TI_H__

#include <stdint.h>

/* 毫秒级延时（替代 HAL_Delay） */
void TI_Delay_ms(uint32_t ms);

/* 四个核心 I2C 函数（替代 HAL_I2C_Mem_Write / Read） */
uint8_t TI_I2C_WriteByte(uint8_t dev_addr, uint8_t reg, uint8_t data);
uint8_t TI_I2C_ReadByte (uint8_t dev_addr, uint8_t reg, uint8_t *data);
uint8_t TI_I2C_WriteLen (uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf);
uint8_t TI_I2C_ReadLen  (uint8_t dev_addr, uint8_t reg, uint8_t len, uint8_t *buf);

#endif
