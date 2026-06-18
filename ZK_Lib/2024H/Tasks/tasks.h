#ifndef __TASKS_H
#define __TASKS_H

#include "main.h"
#include "Protothreads.h"
#include "Log.h"
#include "oled.h"
#include "mpu6050_user.h"
#include "multikey.h"
#include "MadgwickAHRS.h"

#define IMU_UPDATE_PERIOD_MS 20U


uint8_t ReadKey1Pin(MulitKey_t* key);
uint8_t ReadKey2Pin(MulitKey_t* key);
void Key1PressedCallback(MulitKey_t* key);
void Key2PressedCallback(MulitKey_t* key);

void IMU_task(Protothread_t* pt);
void SerialTask(Protothread_t* pt);

#endif // !__TASKS_

