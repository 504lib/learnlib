#include "tasks.h"

MPU6050_Data_t* mpu_data = NULL;
uint8_t OLED_ShowPage_Index_Global = 0;




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
  OLED_ShowPage_Index_Global = (OLED_ShowPage_Index_Global + 1) % 4; // 假设有4页
}

void Key2PressedCallback(MulitKey_t* key)
{
  OLED_ShowPage_Index_Global = (OLED_ShowPage_Index_Global == 0U) ? 3U : (OLED_ShowPage_Index_Global - 1U);
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

void OLED_ShowPage0(Protothread_t* pt)
{
  char buffer[50];
  PT_BEGIN(pt);
  while(1){
    PT_WAIT_UNTIL(pt, OLED_ShowPage_Index_Global == 0);
    PT_WAIT_TICK(pt, 20); // 每隔2000毫秒执行一次 a
    snprintf(buffer, sizeof(buffer), "yaw = %.2f", mpu_data->yaw);
    OLED_ShowString(0, 0, (uint8_t*)buffer,16,1);
    OLED_Refresh();
  }
  PT_END(pt);
}

void OLED_ShowPage1(Protothread_t* pt)
{
  char buffer[50];
  PT_BEGIN(pt);
  while(1){
    PT_WAIT_UNTIL(pt, OLED_ShowPage_Index_Global == 1);
    PT_WAIT_TICK(pt, 20); // 每隔2000毫秒执行一次 a
    snprintf(buffer, sizeof(buffer), "Page Index: %d", OLED_ShowPage_Index_Global);
    OLED_ShowString(0, 0, (uint8_t*)buffer,16,1);
    OLED_Refresh();
  }
  PT_END(pt);
}


void OLED_ShowPage2(Protothread_t* pt)
{
  char buffer[50];
  PT_BEGIN(pt);
  while(1){
    PT_WAIT_UNTIL(pt, OLED_ShowPage_Index_Global == 2);
    PT_WAIT_TICK(pt, 20); // 每隔2000毫秒执行一次 a
    snprintf(buffer, sizeof(buffer), "Page Index: %d", OLED_ShowPage_Index_Global);
    OLED_ShowString(0, 0, (uint8_t*)buffer,16,1);
    OLED_Refresh();
  }
  PT_END(pt);
}

void OLED_ShowPage3(Protothread_t* pt)
{
  char buffer[50];
  PT_BEGIN(pt);
  while(1){
    PT_WAIT_UNTIL(pt, OLED_ShowPage_Index_Global == 3);
    PT_WAIT_TICK(pt, 20); // 每隔2000毫秒执行一次 a
    snprintf(buffer, sizeof(buffer), "Page Index: %d", OLED_ShowPage_Index_Global);
    OLED_ShowString(0, 0, (uint8_t*)buffer,16,1);
    OLED_Refresh();
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

