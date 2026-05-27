#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include "protocol.h"
#include "static_queue.h"

// 应用级命令队列（供 Task_FSM 消费，摄像头和蓝牙共用）
#define APP_CMD_QUEUE_SIZE 16

// 摄像头命令队列
bool App_CmdDequeue_Cam(uint8_t *cmd);
bool App_CmdEnqueue_Cam(uint8_t cmd);

// 蓝牙命令队列
bool App_CmdDequeue(uint8_t *cmd);
bool App_CmdEnqueue(uint8_t cmd);

void App_Protocol_Init(void);
void App_Protocol_Run(void);   // 驱动两个协议实例的 Loop

// 获取各自的协议实例指针
UART_protocol_t* App_GetProtocolInstance_Cam(void);   // USART2 摄像头
UART_protocol_t* App_GetProtocolInstance_Bt(void);    // USART3 蓝牙

// 向摄像头发送数据帧
bool App_SendCamFrame(uint8_t type, const uint8_t* data, uint8_t len);

#endif
