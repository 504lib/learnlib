#include "fork_decide.h"

ForkMode ForkDecide_GetMode(uint8_t task_number)
{
    switch (task_number) {
        case 1:
        case 2:
        case 4:
            return FORK_MODE_STRAIGHT;   // 外圈直行
        case 3:
            return FORK_MODE_LEFT;       // 内圈左转
        default:
            return FORK_MODE_NORMAL;
    }
}
