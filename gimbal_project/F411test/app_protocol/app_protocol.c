#include "app_protocol.h"
#include "Log.h"
#include "usart.h"

volatile float g_ball_pos_cm   = 0.0f;
volatile bool  g_ball_updated  = false;

static UART_protocol_t g_proto = {0};

/* ---- UART6 TX ---- */
static bool __uart6_tx(const uint8_t* data, uint16_t len)
{
    return HAL_UART_Transmit(&huart6, (uint8_t*)data, len, 100) == HAL_OK;
}

/* ---- 帧回调 ---- */
static void __on_frame(uint8_t type, const uint8_t* payload, uint16_t len)
{
    switch (type) {
    case APP_FRAME_BALL_POS:
        if (len >= 4) {
            g_ball_pos_cm  = rd_f32_be(payload);
            g_ball_updated = true;
        }
        break;
    default:
        break;
    }
}

/* ---- 初始化 ---- */
void App_Protocol_Init(void)
{
    Uart_Protocol_FunctionsParameters params = {
        .Head_Tial_Frame_struct = {0xAA, 0x55, 0x0D, 0x0A},
        .transmit_function      = __uart6_tx,
        .frame_received_handler = __on_frame,
    };
    Uart_Protocol_Init(&g_proto, params);

    /* 解析看门狗: 状态机卡住超过 100ms 自动复位, 防止乱字节锁死 */
    Uart_Protocol_Register_Parse_WatchDog(&g_proto, HAL_GetTick, 100);

    /* 启动 UART6 RX 中断 (单字节接收) */
    static uint8_t rx_byte;
    HAL_UART_Receive_IT(&huart6, &rx_byte, 1);

    LOG_INFO("Protocol init done (UART6)");
}

/* ---- 主循环 ---- */
void App_Protocol_Loop(void)
{
    Uart_Protocol_Loop(&g_proto);
}

/* ---- UART RX 中断回调 ---- */
void App_Protocol_FeedByte(uint8_t data)
{
    Uart_Protocol_ProcessReceivedData8bit(&g_proto, data);
}
