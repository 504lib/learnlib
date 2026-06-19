/**
 * @file app_protocol.c
 * @brief 蓝牙双车通信应用层协议实现
 */

#include "app_protocol.h"
#include "ti_msp_dl_config.h"
#include "../Log/Log.h"

// ============================================================
// 蓝牙 UART 配置
// 需要在 syscfg 中配置对应 UART 引脚
// 这里假设使用 UART1 外设（若用 UART3 则改为 UART3）
// ============================================================
#ifndef UART_BT_INST
#define UART_BT_INST               UART1
#endif
#ifndef UART_BT_INST_INT_IRQN
#define UART_BT_INST_INT_IRQN      UART1_INT_IRQn
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
    LOG_INFO("[BT] RX frame type=0x%02X, len=%u", frame_type, frame_len);

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
// 队列操作（适配 protocol 框架）
// ============================================================

// 字节级队列：蓝牙接收的原始字节
DECLARE_STATIC_QUEUE(BtByteQueue, uint8_t, UART_BT_FRAME_BUFFER_LEN * 2);
static BtByteQueue_t bt_byte_queue;

static bool bt_byte_push(void* queue, const uint8_t data)
{
    (void)queue;
    return BtByteQueue_PUSH(&bt_byte_queue, data);
}

static bool bt_byte_pop(void* queue, uint8_t* data)
{
    (void)queue;
    return BtByteQueue_POP(&bt_byte_queue, data);
}

// ============================================================
// 公开 API
// ============================================================
void App_Protocol_Init(void)
{
    // 初始化队列
    BtByteQueue_INIT(&bt_byte_queue);
    App_CmdQueue_INIT(&cmd_queue);

    // 帧边界
    Custom_Frame_HT_T frame_ht = {0xAA, 0x55, 0x55, 0xAA};

    // 队列操作
    Queue_Operations q_ops = {
        .Queue_instance  = NULL,  // 不使用，回调内部已绑定
        .Queue_pushback  = bt_byte_push,
        .Queue_popfront  = bt_byte_pop,
    };

    // 必选参数
    Uart_Protocol_FunctionsParameters req = {
        .Head_Tial_Frame_struct = frame_ht,
        .transmit_function      = uart_bt_send,
        .frame_received_handler = on_bt_frame_received,
        .queue_ops              = q_ops,
    };

    // 可选参数：关闭 ACK（蓝牙近距离通信丢帧概率低）
    Uart_Protocol_OptionalFunctionsParameters opt = {NULL, NULL};

    Uart_Protocol_Init(&protocol_bt, req, opt);

    LOG_INFO("[BT] Protocol initialized");
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
