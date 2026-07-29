#include "app_protocol.h"
#include "Log.h"
#include "usart.h"

volatile int32_t g_ball_pos    = 0;
volatile bool    g_ball_updated = false;

static UART_protocol_t g_proto = {0};

static bool __uart6_tx(const uint8_t* data, uint16_t len)
{
    return HAL_UART_Transmit(&huart6, (uint8_t*)data, len, 100) == HAL_OK;
}

static void __on_frame(uint8_t type, const uint8_t* payload, uint16_t len)
{
    if (type == APP_FRAME_BALL_POS) {
        LOG_INFO("ball pos: %d", (int32_t)rd_u32_be(payload));
        g_ball_pos     = (int32_t)rd_u32_be(payload);
        g_ball_updated = true;
    }
}

void App_Protocol_Init(void)
{
    Uart_Protocol_FunctionsParameters params = {
        .Head_Tial_Frame_struct = {0xAA, 0x55, 0x0D, 0x0A},
        .transmit_function      = __uart6_tx,
        .frame_received_handler = __on_frame,
    };
    Uart_Protocol_Init(&g_proto, params);
    Uart_Protocol_Register_Parse_WatchDog(&g_proto, HAL_GetTick, 100);
    // 帧间隔 1s, 不需要看门狗

    LOG_INFO("Protocol init done (UART6)");
}

void App_Protocol_Loop(void)
{
    Uart_Protocol_Loop(&g_proto);
}

void App_Protocol_FeedByte(uint8_t data)
{
    Uart_Protocol_ProcessReceivedData8bit(&g_proto, data);
}

void APP_Protocol_FeedBuffer( uint8_t* data, uint16_t len)
{
    Uart_Protocol_ProcessReceivedDataBuffer(&g_proto, data, len);
}

int32_t App_Protocol_GetBallPos(void)
{
  return g_ball_pos;
}

bool App_Protocol_IsBallPosUpdated(void)
{
  return g_ball_updated;
}