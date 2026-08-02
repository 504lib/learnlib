#include "app_protocol.h"
#include "Log.h"
#include "usart.h"

volatile int32_t  g_ball_pos    = 0;
volatile int32_t  g_ball_vel    = 0;
volatile uint32_t g_zero_px     = 320;
volatile float     g_vel_value   = 0.0f;
volatile bool     g_ball_updated = false;

static UART_protocol_t g_proto    = {0};
static UART_protocol_t g_proto_u1 = {0};

static bool __uart6_tx(const uint8_t* data, uint16_t len)
{
    return HAL_UART_Transmit(&huart6, (uint8_t*)data, len, 100) == HAL_OK;
}

static bool __uart1_tx(const uint8_t* data, uint16_t len)
{
    return HAL_UART_Transmit(&huart1, (uint8_t*)data, len, 100) == HAL_OK;
}

static void __on_frame(uint8_t type, const uint8_t* payload, uint16_t len)
{
    if (type == APP_FRAME_BALL_POS && len >= 8) {
        g_ball_pos     = (int32_t)rd_u32_be(payload);
        g_ball_vel     = (int32_t)rd_u32_be(payload + 4);
        g_ball_updated = true;
    }
    if (type == APP_FRAME_CALIB && len >= 4) {
        g_zero_px = rd_u32_be(payload);
    }
}

static void __on_frame_u1(uint8_t type, const uint8_t* payload, uint16_t len)
{
    if (type == APP_FRAME_VEL && len >= 4) {
        g_vel_value = rd_f32_be(payload);
        LOG_INFO("VEL %.2f", g_vel_value);
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
    Uart_Protocol_Register_Parse_WatchDog(&g_proto, HAL_GetTick, 1);

    params.transmit_function = __uart1_tx;
    params.frame_received_handler = __on_frame_u1;
    Uart_Protocol_Init(&g_proto_u1, params);
    Uart_Protocol_Register_Parse_WatchDog(&g_proto_u1, HAL_GetTick, 1);
    // LOG_INFO("Protocol init done (UART6+UART1)");
}

void App_Protocol_Loop(void)
{
    Uart_Protocol_Loop(&g_proto);
    Uart_Protocol_Loop(&g_proto_u1);
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
void App_Protocol_FeedByte_UART1(uint8_t data)
{
    Uart_Protocol_ProcessReceivedData8bit(&g_proto_u1, data);
}

void App_Protocol_FeedBuffer_UART1(const uint8_t* data, uint16_t len)
{
    Uart_Protocol_ProcessReceivedDataBuffer(&g_proto_u1, data, len);
}
