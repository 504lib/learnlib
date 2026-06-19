/**
 * @file app_protocol.c
 * @brief 蓝牙双车通信应用层协议（新 protocol API，内置队列）
 */

#include "app_protocol.h"
#include "ti_msp_dl_config.h"
#include "../Log/Log.h"

// ============================================================
// 蓝牙 UART 配置
// ============================================================
#ifndef UART_BT_INST
#define UART_BT_INST               UART0
#endif
#ifndef UART_BT_INST_INT_IRQN
#define UART_BT_INST_INT_IRQN      UART0_INT_IRQn
#endif

// ============================================================
// 静态变量
// ============================================================
static UART_protocol_t    protocol_bt;
static App_CmdQueue_t     cmd_queue;

// ============================================================
// 蓝牙 UART 发送函数
// ============================================================
static bool uart_bt_send(const uint8_t* data, uint16_t len)
{
    if (!data || len == 0) return false;
    for (uint16_t i = 0; i < len; i++)
    {
        DL_UART_Main_transmitDataBlocking(UART_BT_INST, data[i]);
    }
    return true;
}

// ============================================================
// 蓝牙帧接收回调（协议层调用）
// ============================================================
static void on_bt_frame_received(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len)
{
    // 将命令码推入命令队列
    if (!App_CmdQueue_PUSH(&cmd_queue, frame_type))
    {
        LOG_WARN("[BT] Cmd queue full, dropping frame 0x%02X", frame_type);
        return;
    }

    // 对于带载荷的命令，将第一个字节也推入队列
    if (frame_len > 0 && frame_data)
    {
        if (!App_CmdQueue_PUSH(&cmd_queue, frame_data[0]))
        {
            LOG_WARN("[BT] Cmd queue full (payload), dropping");
        }
    }
}

// ============================================================
// 公开 API
// ============================================================
void App_Protocol_Init(void)
{
    // 命令队列
    App_CmdQueue_INIT(&cmd_queue);

    // 帧边界
    Custom_Frame_HT_T frame_ht = {0xAA, 0x55, 0x55, 0xAA};

    // 必选参数（新 API 无需 queue_ops，协议内置 rx_queue）
    Uart_Protocol_FunctionsParameters req = {
        .Head_Tial_Frame_struct = frame_ht,
        .transmit_function      = uart_bt_send,
        .frame_received_handler = on_bt_frame_received,
    };

    Uart_Protocol_Init(&protocol_bt, req);
}

void App_Protocol_Run(void)
{
    Uart_Protocol_Loop(&protocol_bt);
}

bool App_Protocol_SendFrame(uint8_t cmd_type, const uint8_t* payload, uint8_t len)
{
    return Uart_Protocol_Transmit_Frame(&protocol_bt, payload, cmd_type, len);
}

bool App_Protocol_DequeueCmd(uint8_t* cmd)
{
    return App_CmdQueue_POP(&cmd_queue, cmd);
}

bool App_Protocol_DequeueCmdWithPayload(uint8_t* cmd, uint8_t* payload)
{
    if (!App_CmdQueue_POP(&cmd_queue, cmd))
        return false;
    if (payload)
    {
        if (!App_CmdQueue_POP(&cmd_queue, payload))
        {
            *payload = 0;
        }
    }
    return true;
}

void App_Protocol_ProcessByte(uint8_t byte)
{
    Uart_Protocol_ProcessReceivedData8bit(&protocol_bt, byte);
}

UART_protocol_t* App_Protocol_GetInstance(void)
{
    return &protocol_bt;
}
