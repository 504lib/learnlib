#include "app_protocol.h"
#include "usart.h"          // 你的蓝牙串口句柄 huart3
#include "string.h"

// ---------- 应用命令队列 ----------
DECLARE_STATIC_QUEUE(UART_PROTOCOL_QUEUE,uint8_t,UART_PROTOCOL_FRAME_BUFFER_LEN * 2);
DECLARE_STATIC_QUEUE(App_CmdQueue,uint8_t,APP_CMD_QUEUE_SIZE);
static UART_PROTOCOL_QUEUE_t uart_protocol_queue = {0};
static App_CmdQueue_t app_cmd_queue = {0};



// ---------- 蓝牙发送函数 ----------
static bool uart_send(const uint8_t* data, uint16_t len) {
    return (HAL_UART_Transmit(&huart3, (uint8_t*)data, len, 100) == HAL_OK);
}

bool App_CmdDequeue(uint8_t *cmd) 
{
    return App_CmdQueue_POP(&app_cmd_queue, cmd);
}

bool App_CmdEnqueue(uint8_t cmd) 
{
    return App_CmdQueue_PUSH(&app_cmd_queue, cmd);
}

// ---------- 协议解析成功回调 ----------
static void on_frame_received(uint8_t frame_type, const uint8_t* payload, uint16_t len) {
    // 将帧类型作为指令压入应用队列（也可根据 payload 进一步解析）
    LOG_INFO("Received frame type: 0x%02X, payload length: %d", frame_type, len);
    for (size_t i = 0; i < len; i++)
    {
        LOG_INFO("Payload byte %d: 0x%02X", i, payload[i]);
    }
    App_CmdEnqueue(frame_type);
    // 例如：frame_type 0x01=去病房1, 0x02=去病房2, 0x03=返回...
}

// ---------- 全局协议实例 ----------
static UART_protocol_t protocol_inst;      // 协议实例

void App_Protocol_Init(void) {
    App_CmdQueue_INIT(&app_cmd_queue);
    // 定义帧边界（与电脑端约定相同）
    Custom_Frame_HT_T frame_ht = {0xAA, 0x55, 0x55, 0xAA};

    // 队列操作绑定
    Queue_Operations q_ops = {
        .Queue_instance = &uart_protocol_queue,
        .Queue_pushback = UART_PROTOCOL_QUEUE_PUSH,
        .Queue_popfront = UART_PROTOCOL_QUEUE_POP
    };

    // 初始化队列
    UART_PROTOCOL_QUEUE_INIT(&uart_protocol_queue);
    

    // 必要参数
    Uart_Protocol_FunctionsParameters req = {
        .Head_Tial_Frame_struct = frame_ht,
        .transmit_function = uart_send,
        .frame_received_handler = on_frame_received,
        .queue_ops = q_ops
    };

    // 可选参数：不使用 ACK 和超时
    Uart_Protocol_OptionalFunctionsParameters opt = {NULL, NULL};

    Uart_Protocol_Init(&protocol_inst, req, opt);
}

// 周期调用，推进协议状态机
void App_Protocol_Run(void) {
    Uart_Protocol_Loop(&protocol_inst);
}

UART_protocol_t* App_GetProtocolInstance(void) {
    return &protocol_inst;
}
