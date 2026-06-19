#pragma once

#include <stdint.h>
#include <stdbool.h>

#define GRAY_BITS 8

// ============================================================
// 岔路模式
// ============================================================
typedef enum {
    FORK_MODE_NORMAL   = 0,   // 正常巡线（对称权值）
    FORK_MODE_STRAIGHT = 1,   // 外圈直行（屏蔽左边S0~S2，信任右边）
    FORK_MODE_LEFT     = 2,   // 内圈左转（屏蔽右边S4~S7，信任左边）
} ForkMode;

// ============================================================
// 灰度误差计算
// ============================================================

/** 原始版本（保持兼容，使用对称权值） */
float CalculateGrayError_Advanced(uint8_t gray_byte);

/** 动态权值版本（根据当前 ForkMode 选权值表） */
float CalculateGrayError(uint8_t gray_byte);

/** 切换权值模式 */
void Grayscale_SetMode(ForkMode mode);

/** 获取当前权值模式 */
ForkMode Grayscale_GetMode(void);

// ============================================================
// 岔路检测（正常巡线中心 S3-S4）
// ============================================================

/** 获取黑线传感器数量（bit=0 为黑） */
int Gray_BlackCount(uint8_t gray_byte);

/** 是否全黑 (8路全黑 = 岔路预警标记线) */
bool Gray_IsAllBlack(uint8_t gray_byte);

/** 是否岔路分裂 (S3变白 + 左右各有黑) */
bool Gray_IsForkSplit(uint8_t gray_byte);

/** 是否已通过直行岔路 (左半边S0~S2全白, 右半边1~2路黑) */
bool Gray_IsStraightPassed(uint8_t gray_byte);

/** 是否已通过左转岔路 (右半边S4~S7全白, 左半边1~2路黑) */
bool Gray_IsLeftPassed(uint8_t gray_byte);

/** 是否窄线巡线状态 (1~3路黑) */
bool Gray_IsNarrowLine(uint8_t gray_byte);
