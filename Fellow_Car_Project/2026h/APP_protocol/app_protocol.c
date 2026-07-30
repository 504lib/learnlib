#include "app_protocol.h"
#include "app_log.h"
#include "usart.h"

UART_protocol_t g_app_protocol = {0};

/* ---- 底层 TX ---- */
static bool __uart_tx(const uint8_t* data, uint16_t len)
{
    return HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 100) == HAL_OK;
}

/* ---- 接收回调 ---- */
static void __on_frame(uint8_t type, const uint8_t* payload, uint16_t len)
{
    switch (type) {
    case APP_FRAME_CMD:
        LOG_DEBUG("CMD received, len=%u", len);
        break;
    default:
        LOG_DEBUG("Unknown frame type 0x%02X", type);
        break;
    }
}

/* ---- 初始化 ---- */
bool App_Protocol_Init(void)
{
    Uart_Protocol_FunctionsParameters params = {
        .Head_Tial_Frame_struct = {0xAA, 0x55, 0x0D, 0x0A},
        .transmit_function      = __uart_tx,
        .frame_received_handler = __on_frame,
    };

    if (!Uart_Protocol_Init(&g_app_protocol, params)) {
        LOG_FATAL("Protocol init failed");
        return false;
    }

    LOG_INFO("Protocol initialized");
    return true;
}

/* ---- 主循环 ---- */
void App_Protocol_Loop(void)
{
    Uart_Protocol_Loop(&g_app_protocol);
}

/* ---- 发送球位置 ---- */
bool App_Protocol_SendBallPos(float pos_cm)
{
    uint8_t buf[4];
    wr_f32_be(buf, pos_cm);
    return Uart_Protocol_Transmit_Frame(&g_app_protocol, buf,
                                        APP_FRAME_BALL_POS, sizeof(buf));
}

bool App_Protocol_SendVel(float average_vel)
{
    uint8_t buf[4];
    wr_f32_be(buf, average_vel);
    return Uart_Protocol_Transmit_Frame(&g_app_protocol, buf,
                                        APP_FRAME_SEND_VEL, sizeof(buf));
}

/* ---- 发送系统状态 ---- */
bool App_Protocol_SendSysState(SysState state, uint32_t elapsed_ms)
{
    uint8_t buf[5];
    buf[0] = (uint8_t)state;
    wr_u32_be(&buf[1], elapsed_ms);
    return Uart_Protocol_Transmit_Frame(&g_app_protocol, buf,
                                        APP_FRAME_SYS_STATE, sizeof(buf));
}
