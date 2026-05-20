#ifndef __F_SM_H
#define __F_SM_H

#include "Finite_State_Machine.h"
#include <stdbool.h>

// ---------- 2021 电赛 F 题状态定义 ----------
typedef enum {
    FSM_STATE_IDLE = 0,
    FSM_STATE_STRAIGHT,          // 直行（角度 PID 保持航向）
    FSM_STATE_TRACKING,          // 循迹（灰度 PID 沿黑线）
    FSM_STATE_STOP_AT_WARD,      // 在病房前停车
    FSM_STATE_RETURN_STRAIGHT,   // 返回直行
    FSM_STATE_RETURN_TRACKING,   // 返回循迹
    FSM_STATE_FINISH,            // 任务完成
    FSM_STATE_MAX                // 状态总数
} FSM_StateID_t;

// ---------- 控制模式（与 control.c 共享） ----------
typedef enum {
    CTRL_MODE_IDLE = 0,
    CTRL_MODE_STRAIGHT,          // 使用角度 PID
    CTRL_MODE_TRACKING,          // 使用灰度 PID
    CTRL_MODE_STOP
} CtrlMode_t;

// ---------- 全局变量（供 control.c 等使用） ----------
extern float   g_target_yaw;      // 直行时的目标航向角
extern float   g_distance_traveled; // 当前阶段累计路程（m）
extern int     g_target_ward;     // 目标病房号（1 或 2）
extern CtrlMode_t g_ctrl_mode;    // 当前控制模式

// ---------- 公共接口 ----------
void FSM_App_Init(void);                     // 初始化整个应用状态机
void FSM_App_Run(void);                      // 每个控制周期调用一次，处理转移和动作
void FSM_App_ReceiveCommand(uint8_t cmd);    // 蓝牙 / 串口指令输入
bool FSM_App_IsRunning(void);                // 是否有任务正在执行

#endif
