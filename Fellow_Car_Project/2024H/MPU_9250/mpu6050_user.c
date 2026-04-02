#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#include <math.h>
#include "mpu6050_user.h"
#include <math.h>
#include "mpu6050.h"
#include "MadgwickAHRS.h"
#include "oled.h"

//extern tPid pid_yaw;
//extern float gz_offset;
// 定义全局静态实例（内存只有一份）
static MPU6050_Data_t mpu_data;

// 获取句柄
MPU6050_Data_t* MPU6050_GetHandle(void) {
    return &mpu_data;
}

// 读取原始数据（调用底层驱动）
static void read_raw(void) {
    MPU_Get_Accelerometer(&mpu_data.raw.ax, &mpu_data.raw.ay, &mpu_data.raw.az);
    MPU_Get_Gyroscope(&mpu_data.raw.gx, &mpu_data.raw.gy, &mpu_data.raw.gz);
}

// 应用校准并转换为物理量
static void convert_to_physical(void) {
    const float ACCEL_SCALE = 16384.0f;  // ±2g
    const float GYRO_SCALE  = 16.4f;     // ±2000dps
    const float DEG_TO_RAD  = M_PI / 180.0f;

    // 加速度：原始值 → 减去偏移 → 转换为 g → 转换为 m/s²
    mpu_data.phys.ax = (mpu_data.raw.ax - mpu_data.calib.ax_offset) / ACCEL_SCALE * 9.8f;
    mpu_data.phys.ay = (mpu_data.raw.ay - mpu_data.calib.ay_offset) / ACCEL_SCALE * 9.8f;
    mpu_data.phys.az = (mpu_data.raw.az - mpu_data.calib.az_offset) / ACCEL_SCALE * 9.8f;

    // 角速度：原始值 → 减去偏移 → 转换为 dps → 转换为 rad/s
    mpu_data.phys.gx = (mpu_data.raw.gx - mpu_data.calib.gx_offset) / GYRO_SCALE * DEG_TO_RAD;
    mpu_data.phys.gy = (mpu_data.raw.gy - mpu_data.calib.gy_offset) / GYRO_SCALE * DEG_TO_RAD;
    mpu_data.phys.gz = (mpu_data.raw.gz - mpu_data.calib.gz_offset) / GYRO_SCALE * DEG_TO_RAD;
}

// 一阶低通滤波（可选）
static void lowpass_filter(float alpha) {
    static float filtered_gz = 0.0f;
    filtered_gz = alpha * filtered_gz + (1.0f - alpha) * mpu_data.phys.gz;
    mpu_data.gz_last = filtered_gz;
}

// 对外更新函数：读取 → 转换 → 滤波
void MPU6050_Update(void) {
    read_raw();
    convert_to_physical();
    lowpass_filter(0.7f);   // 可选，你可以保留或去掉

    // 调用 Madgwick 算法（输入单位：角速度 rad/s，加速度 m/s²）
    // 注意：某些 Madgwick 实现期望加速度以 g 为单位，如果你发现姿态解算错误，可以尝试将加速度除以 9.8
    MadgwickAHRSupdateIMU(mpu_data.phys.gx, mpu_data.phys.gy, mpu_data.phys.gz,
                          mpu_data.phys.ax, mpu_data.phys.ay, mpu_data.phys.az);

    // 获取欧拉角（单位：度）
    Get_Euler_Angles(&mpu_data.yaw, &mpu_data.pitch, &mpu_data.roll);

    // 可选：将角度归一化到 [-180, 180] 范围（Madgwick 输出可能已经在范围内，但为了安全）
    if (mpu_data.yaw > 180.0f) mpu_data.yaw -= 360.0f;
    if (mpu_data.yaw < -180.0f) mpu_data.yaw += 360.0f;
    if (mpu_data.pitch > 180.0f) mpu_data.pitch -= 360.0f;
    if (mpu_data.pitch < -180.0f) mpu_data.pitch += 360.0f;
    if (mpu_data.roll > 180.0f) mpu_data.roll -= 360.0f;
    if (mpu_data.roll < -180.0f) mpu_data.roll += 360.0f;
}

// 校准函数：采样静止状态，计算偏移量（原始 ADC 单位）
void MPU6050_Calibrate(uint16_t samples) {
    int32_t ax_sum = 0, ay_sum = 0, az_sum = 0;
    int32_t gx_sum = 0, gy_sum = 0, gz_sum = 0;
    char buffer[20];
    for (uint16_t i = 0; i < samples; i++) {
        read_raw();
        ax_sum += mpu_data.raw.ax;
        ay_sum += mpu_data.raw.ay;
        az_sum += mpu_data.raw.az;
        gx_sum += mpu_data.raw.gx;
        gy_sum += mpu_data.raw.gy;
        gz_sum += mpu_data.raw.gz;
        snprintf(buffer,sizeof(buffer),"try %d times",i);
		OLED_ShowString(0, 32, buffer, 16,1);
		OLED_Refresh();
		HAL_Delay(5);
    }

    mpu_data.calib.ax_offset = ax_sum / (float)samples;
    mpu_data.calib.ay_offset = ay_sum / (float)samples;
    mpu_data.calib.az_offset = az_sum / (float)samples - 16384.0f; // 静止时 Z 轴应有 1g
    mpu_data.calib.gx_offset = gx_sum / (float)samples;
    mpu_data.calib.gy_offset = gy_sum / (float)samples;
    mpu_data.calib.gz_offset = gz_sum / (float)samples;
	OLED_Clear();
    OLED_ShowString(0, 0, "Calibration", 16,1);
    OLED_ShowString(0, 16, "Complete!", 16,1);
    OLED_Refresh();
    HAL_Delay(1000);
    OLED_Clear();
}
