#ifndef __APP_PROTOCOL_H__
#define __APP_PROTOCOL_H__

#include "protocol.h"
#include <stdint.h>

#define APP_FRAME_BALL_POS  0x10
#define APP_FRAME_CALIB     0x11

extern volatile int32_t  g_ball_pos;
extern volatile uint32_t g_zero_px;
extern volatile bool     g_ball_updated;

void App_Protocol_Init(void);
void App_Protocol_Loop(void);
void App_Protocol_FeedByte(uint8_t data);
int32_t App_Protocol_GetBallPos(void);
void APP_Protocol_FeedBuffer( uint8_t* data, uint16_t len);
bool App_Protocol_IsBallPosUpdated(void);


#endif
