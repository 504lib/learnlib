/*
 * HSM OLED — 用分层状态机管理 OLED 四页切换
 *
 * 替代原有的 OLED_ShowPage_Index_Global + 4 个 protothread 轮询模式。
 *
 * 状态树:
 *   root
 *   ├── page0 (yaw 显示)
 *   ├── page1 (roll 显示)
 *   ├── page2 (pitch 显示)
 *   └── page3 (电机状态)
 *
 * key1 → 下一页, key2 → 上一页。entry_action 渲染页面。
 * root->active_child 始终指向当前活跃页, 用于判定目标。
 */

#include "HSM_OLED.h"
#include "HSM_Core.h"
#include "oled.h"
#include "mpu6050_user.h"
#include <stdio.h>

static HSM_Core_Memory core_mem;
static HSM_Node        nodes[8];

static HSM*     hsm;
static HSM_Node *page[4];   // page[0..3]

enum { EV_KEY1 = 1, EV_KEY2 = 2 };

// ============================================================
// 每页 entry — 渲染该页内容
// ============================================================
static void entry_page0(HSM_Event_Package ev)
{
    MPU6050_Data_t* d = MPU6050_GetHandle();
    char buf[24];
    snprintf(buf, sizeof(buf), "yaw=%.1f  ", d->yaw);
    OLED_ShowString(0, 0, (uint8_t*)buf, 16, 1);
    OLED_Refresh();
}

static void entry_page1(HSM_Event_Package ev)
{
    MPU6050_Data_t* d = MPU6050_GetHandle();
    char buf[24];
    snprintf(buf, sizeof(buf), "roll=%.1f ", d->roll);
    OLED_ShowString(0, 0, (uint8_t*)buf, 16, 1);
    OLED_Refresh();
}

static void entry_page2(HSM_Event_Package ev)
{
    MPU6050_Data_t* d = MPU6050_GetHandle();
    char buf[24];
    snprintf(buf, sizeof(buf), "pitch=%.1f", d->pitch);
    OLED_ShowString(0, 0, (uint8_t*)buf, 16, 1);
    OLED_Refresh();
}

static void entry_page3(HSM_Event_Package ev)
{
    OLED_ShowString(0, 0, (uint8_t*)"Motor: OK   ", 16, 1);
    OLED_Refresh();
}

// ============================================================
// handler — 按键事件, 返回 true 停止冒泡
// ============================================================
static bool h_page(HSM_Event_Package e)
{
    return e.HSM_Event_ID == EV_KEY1 || e.HSM_Event_ID == EV_KEY2;
}

// ============================================================
// 状态表 — 5 行定义整棵树
// ============================================================
static HSM_StateDef states[] = {
    HSM_STATE_DEF("root",  NULL,    NULL,    NULL,       NULL, NULL),
    HSM_STATE_DEF("page0", "root",  h_page,  entry_page0, NULL, NULL),
    HSM_STATE_DEF("page1", "root",  h_page,  entry_page1, NULL, NULL),
    HSM_STATE_DEF("page2", "root",  h_page,  entry_page2, NULL, NULL),
    HSM_STATE_DEF("page3", "root",  h_page,  entry_page3, NULL, NULL),
};

// ============================================================
// 公开接口
// ============================================================
void HSM_OLED_Init(void)
{
    hsm = HSM_Create(&core_mem, nodes, 8, states, 5);

    page[0] = HSM_FindNode(hsm, "page0");
    page[1] = HSM_FindNode(hsm, "page1");
    page[2] = HSM_FindNode(hsm, "page2");
    page[3] = HSM_FindNode(hsm, "page3");
    HSM_Start(hsm, page[0]);

    // Transition 表 — 拓扑一目了然
    HSM_Transition trans[] = {
        HSM_TRANS(page[0], EV_KEY1, page[1]),
        HSM_TRANS(page[1], EV_KEY1, page[2]),
        HSM_TRANS(page[2], EV_KEY1, page[3]),
        HSM_TRANS(page[3], EV_KEY1, page[0]),

        HSM_TRANS(page[0], EV_KEY2, page[3]),
        HSM_TRANS(page[1], EV_KEY2, page[0]),
        HSM_TRANS(page[2], EV_KEY2, page[1]),
        HSM_TRANS(page[3], EV_KEY2, page[2]),
    };
    HSM_RegisterTransitions(hsm, trans, 8);
}

// 按键回调 —→ 只发事件, transition 表自动跳转
void HSM_OLED_Key1(void)
{
    HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = EV_KEY1 });
}

void HSM_OLED_Key2(void)
{
    HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = EV_KEY2 });
}

// 主循环调用
void HSM_OLED_Process(void)
{
    HSM_Process(hsm);
}
