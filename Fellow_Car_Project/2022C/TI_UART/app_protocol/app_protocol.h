/**
 * @file app_protocol.h
 * @brief 蓝牙双车通信应用层协议
 *
 * 命令码定义 + 收发API
 * 基于 protocol.c 帧协议
 *
 * 帧结构: | 0xAA | 0x55 | TYPE | LEN | PAYLOAD | CSUM_H | CSUM_L | 0x55 | 0xAA |
 */

#ifndef __APP_PROTOCOL_H
#define __APP_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../protocol/protocol.h"
#include "../static_queue/static_queue.h"

// ============================================================
// 蓝牙串口配置（需在 syscfg 中配置 UART1 或 UART3 给蓝牙）
// ============================================================
#ifndef UART_BT_FRAME_BUFFER_LEN
#define UART_BT_FRAME_BUFFER_LEN  16
#endif

// 接收命令队列容量
#define APP_CMD_QUEUE_CAPACITY    8

// ============================================================
// 双车通信命令码
// ============================================================
#define CMD_BT_GO           0x01    // 出发命令 (领头→跟随) payload: 任务号(1字节)
#define CMD_BT_STOP         0x02    // 停止命令 (领头→跟随) payload: 无
#define CMD_BT_FORK         0x03    // 岔路决策 (领头→跟随) payload: 0=外圈直行, 1=内圈左转
#define CMD_BT_SPEED        0x04    // 速度设置 (领头→跟随) payload: float×4字节
#define CMD_BT_OVERTAKE     0x05    // 超车指令 (领头→跟随) payload: 无
#define CMD_BT_TASK_SELECT  0x06    // 任务选择 (领头→跟随) payload: 任务号(1字节)
#define CMD_BT_ARRIVED            0x07    // 到达确认 (跟随→领头) payload: 无
#define CMD_BT_OVERTAKE_DONE       0x08    // 超车完成 (双向) payload: 无
#define CMD_BT_ACK                 0xFF    // ACK (双向) payload: 无

// ============================================================
// 命令队列
// ============================================================
DECLARE_STATIC_QUEUE(App_CmdQueue, uint8_t, APP_CMD_QUEUE_CAPACITY);

// ============================================================
// API
// ============================================================

/** 初始化蓝牙协议（绑定UART、注册回调） */
void App_Protocol_Init(void);

/** 主循环驱动：消费接收队列 + 解析帧 */
void App_Protocol_Run(void);

/** 发送数据帧 */
bool App_Protocol_SendFrame(uint8_t cmd_type, const uint8_t* payload, uint8_t len);

/** 从命令队列取出一条命令（非阻塞） */
bool App_Protocol_DequeueCmd(uint8_t* cmd);

/** 从命令队列取出一条命令，并读取1字节载荷 */
bool App_Protocol_DequeueCmdWithPayload(uint8_t* cmd, uint8_t* payload);

/** 串口中断中调用：将收到的字节推入协议队列 */
void App_Protocol_ProcessByte(uint8_t byte);

/** 获取协议实例指针（ISR中调用） */
UART_protocol_t* App_Protocol_GetInstance(void);

#endif
