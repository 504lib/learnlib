#include "grayscale.h"

// ============================================================
// 三种权值表
//   正常巡线中心 S3-S4，零交叉在 -0.5 和 +0.5 之间
// ============================================================

// 正常巡线：对称权值
static const float weights_normal[GRAY_BITS] = {
    -3.5f,  // S0
    -2.5f,  // S1
    -1.5f,  // S2
    -0.5f,  // S3
     0.5f,  // S4
     1.5f,  // S5
     2.5f,  // S6
     3.5f   // S7
};

// 外圈直行：屏蔽左边 S0~S2（左转分支），只看右边 S4~S7（直行分支）
static const float weights_straight[GRAY_BITS] = {
    -0.5f,  // S0  屏蔽
    -0.4f,  // S1  屏蔽
    -0.3f,  // S2  屏蔽
    -0.2f,  // S3  过渡
     0.5f,  // S4
     1.5f,  // S5
     2.5f,  // S6
     3.5f   // S7
};

// 内圈左转：屏蔽右边 S4~S7（直行分支），只看左边 S0~S2（左转分支）
static const float weights_left[GRAY_BITS] = {
    -3.5f,  // S0
    -2.5f,  // S1
    -1.5f,  // S2
    -0.5f,  // S3  过渡
     0.2f,  // S4  屏蔽
     0.3f,  // S5  屏蔽
     0.4f,  // S6  屏蔽
     0.5f   // S7  屏蔽
};

static const float* g_active_weights = weights_normal;
static ForkMode g_current_mode = FORK_MODE_NORMAL;

// ============================================================
// 动态权值接口
// ============================================================
void Grayscale_SetMode(ForkMode mode)
{
    g_current_mode = mode;
    switch (mode) {
        case FORK_MODE_STRAIGHT: g_active_weights = weights_straight; break;
        case FORK_MODE_LEFT:     g_active_weights = weights_left;     break;
        default:                 g_active_weights = weights_normal;   break;
    }
}

ForkMode Grayscale_GetMode(void)
{
    return g_current_mode;
}

// ============================================================
// 灰度误差计算
// ============================================================

// 原始版本（保持兼容，使用对称权值）
float CalculateGrayError_Advanced(uint8_t gray_byte)
{
    float symmetric_sum = 0.0f;
    int total_count = 0;

    for (int i = 0; i < GRAY_BITS; i++)
    {
        if ((gray_byte & (1 << i)) == 0)
        {
            symmetric_sum += weights_normal[i];
            total_count++;
        }
    }

    if (total_count == 0 || total_count == GRAY_BITS)
        return 0.0f;

    return symmetric_sum / total_count;
}

// 动态权值版本（使用当前激活的权值表）
float CalculateGrayError(uint8_t gray_byte)
{
    float weighted_sum = 0.0f;
    int total_count = 0;

    for (int i = 0; i < GRAY_BITS; i++)
    {
        if ((gray_byte & (1 << i)) == 0)
        {
            weighted_sum += g_active_weights[i];
            total_count++;
        }
    }

    if (total_count == 0 || total_count == GRAY_BITS)
        return 0.0f;

    return weighted_sum / total_count;
}

// ============================================================
// 岔路检测
// ============================================================

int Gray_BlackCount(uint8_t gray_byte)
{
    int count = 0;
    for (int i = 0; i < GRAY_BITS; i++)
    {
        if ((gray_byte & (1 << i)) == 0)
            count++;
    }
    return count;
}

bool Gray_IsAllBlack(uint8_t gray_byte)
{
    // 只检测中间6位 S1~S6（屏蔽最外侧 S0 和 S7）
    return (gray_byte & 0x7E) == 0x00;
}

bool Gray_IsForkSplit(uint8_t gray_byte)
{
    // 中间传感器 S3 变白，且左右各有黑
    bool s3_white  = (gray_byte & (1 << 3)) != 0;   // S3=1 即白
    bool left_has_black  = !(gray_byte & (1 << 2));  // S2 黑
    bool right_has_black = !(gray_byte & (1 << 4));  // S4 黑
    return s3_white && left_has_black && right_has_black;
}

bool Gray_IsStraightPassed(uint8_t gray_byte)
{
    // 左半边 S0~S2 全白（左分支脱离）
    bool left_clean = ((gray_byte >> 0) & 0x07) == 0x07;  // S0,S1,S2 全为1(白)
    // 右半边有 1~2 路黑
    int right_cnt = 0;
    for (int i = 4; i < 8; i++)
        if (!(gray_byte & (1 << i))) right_cnt++;
    return left_clean && right_cnt >= 1 && right_cnt <= 2;
}

bool Gray_IsLeftPassed(uint8_t gray_byte)
{
    // 右半边 S4~S7 全白（右分支脱离）
    bool right_clean = ((gray_byte >> 4) & 0x0F) == 0x0F;  // S4~S7 全为1(白)
    // 左半边有 1~2 路黑
    int left_cnt = 0;
    for (int i = 0; i < 3; i++)
        if (!(gray_byte & (1 << i))) left_cnt++;
    return right_clean && left_cnt >= 1 && left_cnt <= 2;
}

bool Gray_IsNarrowLine(uint8_t gray_byte)
{
    int cnt = Gray_BlackCount(gray_byte);
    return cnt >= 1 && cnt <= 3;
}
