#include "tasks.h"
#include "HSM_OLED.h"

MPU6050_Data_t* mpu_data = NULL;




uint8_t ReadKey1Pin(MulitKey_t* key)
{
    return HAL_GPIO_ReadPin(PCB_KEY0_GPIO_Port, PCB_KEY0_Pin);
}

uint8_t ReadKey2Pin(MulitKey_t* key)
{
  return HAL_GPIO_ReadPin(PCB_KEY1_GPIO_Port, PCB_KEY1_Pin);
}

void Key1PressedCallback(MulitKey_t* key)
{
    HSM_OLED_Key1();
}

void Key2PressedCallback(MulitKey_t* key)
{
    HSM_OLED_Key2();
}

void IMU_task(Protothread_t* pt)
{
    PT_BEGIN(pt);
    MPU_Init();
    MPU6050_Calibrate(200);
    mpu_data = MPU6050_GetHandle();
    MadgwickAHRSsetSampleFreq(1000.0f / IMU_UPDATE_PERIOD_MS);
    while(1){
        MPU6050_Update();
        PT_WAIT_TICK(pt, IMU_UPDATE_PERIOD_MS); // 每 20ms 更新一次 IMU
        HAL_GPIO_TogglePin(TestPin_GPIO_Port, TestPin_Pin); // 用于逻辑分析仪调试，观察IMU更新频率
    }
    PT_END(pt);
}

void SerialTask(Protothread_t* pt)
{
    PT_BEGIN(pt);
    while(1){
        // LOG_DEBUG("Yaw: %.2f, Pitch: %.2f, Roll: %.2f", mpu_data->yaw, mpu_data->pitch, mpu_data->roll);
        // LOG_DEBUG("gz: %.2f", mpu_data->phys.gz);
        PT_WAIT_TICK(pt, 100); // 每隔100毫秒执行一次
    }
    PT_END(pt);
}

