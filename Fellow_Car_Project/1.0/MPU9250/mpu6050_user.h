#pragma once
#include "mpu6050.h"
#include "oled.h"
#include <stdio.h>
#include <math.h>
#include "usart.h"

typedef struct
{
    float gx_offset;
    float gy_offset;
    float gz_offset;
}Gyro_offset;

typedef struct
{
    float ax_offset;
    float ay_offset;
    float az_offset;
}Accelero_offset;

typedef struct
{
    float mx_offset;
    float my_offset;
    float mz_offset;
}Magneto_offset;

typedef struct
{
    float yaw;
    float pitch;
    float roll;
}Euler_Angles;


typedef struct
{
    Gyro_offset* gyro_offset;
    Accelero_offset* accelero_offset;
    Magneto_offset* magneto_offset;
    Euler_Angles* euler_angles;
}MPU6050_Offsets;


typedef struct
{
    float scale_x;
    float scale_y;
    float scale_z;
}Scale;


typedef struct
{
    float ax;
    float ay;
    float az;
}Accelero_Data;


typedef struct
{
    float mx;
    float my;
    float mz;
}Magneto_Data;

typedef struct
{
    float gx;
    float gy;
    float gz;
}Gyro_Data;


typedef struct
{
    Gyro_Data* gyro;
    Accelero_Data* accelero;
    Magneto_Data* magneto;
    MPU6050_Offsets* offsets;
    Scale* scale;
}MPU6050_Data;

MPU6050_Data* MPU6050_Init_Data(void);
void MPU6050_Test_Display(MPU6050_Data* mpu6050_data);

void update_MPU6050_Data(MPU6050_Data* mpu6050_data);

void get_gyro_filtered(MPU6050_Data* mpu6050_data);
void get_accelero_filtered(MPU6050_Data* mpu6050_data);
void Get_yaw_ComGyroMagnetometer(MPU6050_Data* mpu6050_data, float* yaw , float* pitch , float* roll , float dt);
