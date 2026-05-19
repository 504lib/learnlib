#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include "protocol.h"
#include "static_queue.h"

// 应用级命令队列（供 Task_FSM 消费）
#define APP_CMD_QUEUE_SIZE 16

bool App_CmdDequeue(uint8_t *cmd);
bool App_CmdEnqueue(uint8_t cmd);

void App_Protocol_Init(void);
void App_Protocol_Run(void);   // 调用 Uart_Protocol_Loop
UART_protocol_t* App_GetProtocolInstance(void);

#endif
