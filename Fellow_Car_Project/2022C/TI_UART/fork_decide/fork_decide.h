#pragma once

#include <stdint.h>
#include "../grayscale/grayscale.h"

// ============================================================
// 根据当前任务号，返回岔路走外圈直行还是内圈左转
//
//   任务 1/2/4 → FORK_MODE_STRAIGHT (外圈)
//   任务 3     → FORK_MODE_LEFT     (内圈)
//   其他       → FORK_MODE_NORMAL   (不处理)
// ============================================================

ForkMode ForkDecide_GetMode(uint8_t task_number);
