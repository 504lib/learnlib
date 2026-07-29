#ifndef __APP_PROTOCOL_H__
#define __APP_PROTOCOL_H__

#include "protocol.h"

/* 帧类型 */
#define APP_FRAME_BALL_POS  0x10   // 球位置 (float cm, 大端)

extern volatile float g_ball_pos_cm;   // 最新球位置
extern volatile bool  g_ball_updated;  // 新数据标志

void App_Protocol_Init(void);
void App_Protocol_Loop(void);
void App_Protocol_FeedByte(uint8_t data);

#endif
