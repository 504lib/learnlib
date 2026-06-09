#pragma once
#include "mpu6050.h"
#include "./SPI_OLED/oled.h"
#include <stdio.h>
#include <math.h>
#include "./MPU_6050/mpu6050_i2c_ti.h"

// 原始数据（ADC 计数值）
typedef struct {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
} MPU_Raw_t;

// 物理量（已转换、已校准）
typedef struct {
    float ax, ay, az;   // 加速度 (m/s²)
    float gx, gy, gz;   // 角速度 (rad/s)
} MPU_Phys_t;

// 校准偏移量（原始 ADC 单位）
typedef struct {
    float ax_offset, ay_offset, az_offset;
    float gx_offset, gy_offset, gz_offset;
} MPU_Calib_t;

// 主数据结构：包含原始、物理、校准
typedef struct {
    MPU_Raw_t   raw;
    MPU_Phys_t  phys;
    MPU_Calib_t calib;
    float       gz_last;        // 滤波后的角速度（可选）
    float       yaw, pitch, roll; // 欧拉角（度）
} MPU6050_Data_t;

// 对外接口
MPU6050_Data_t* MPU6050_GetHandle(void);  // 获取全局实例指针
void MPU6050_Update(void);                // 读取传感器并更新所有数据（需周期性调用）
void MPU6050_Calibrate(uint16_t samples); // 校准零偏
