#include "app_fsm.h"
#include "app_log.h"
#include "multikey.h"
#include "app_protocol.h"
#include "oled.h"
#include "app_timer.h"
#include "control.h"
/* ======== 电机行动信号 ======== */
/* ======== 状态机 ======== */
float last_distance = 0.0f;
float current_distance = 0.0f;
const float curve_target_distance = 6.1f;
const float straight_target_distance = 1.5f;
/* ---- 状态声明 ---- */
static bool Root_Handler(HSM_Event_Package ev);
static void Root_Entry(HSM_Event_Package ev);
static void Root_Exit(HSM_Event_Package ev);

static bool Idle_Handler(HSM_Event_Package ev);
static void Idle_Entry(HSM_Event_Package ev);
static void Idle_Exit(HSM_Event_Package ev);
static void Idle_Continuous(void);

static bool Straight_Handler(HSM_Event_Package ev);
static void Straight_Entry(HSM_Event_Package ev);
static void Straight_Exit(HSM_Event_Package ev);
static void Straight_Continuous(void);

static bool Curve_Handler(HSM_Event_Package ev);
static void Curve_Entry(HSM_Event_Package ev);
static void Curve_Exit(HSM_Event_Package ev);
static void Curve_Continuous(void);

/* ---- 状态表 ---- */
static const HSM_StateDef state_table[] = {
    HSM_STATE_DEF("Root",     NULL,    Root_Handler,     Root_Entry,     Root_Exit,     NULL),
    HSM_STATE_DEF("Idle",     "Root",  Idle_Handler,     Idle_Entry,     Idle_Exit,     Idle_Continuous),
    HSM_STATE_DEF("Straight", "Root",  Straight_Handler, Straight_Entry, Straight_Exit, Straight_Continuous),
    HSM_STATE_DEF("Curve",    "Root",  Curve_Handler,    Curve_Entry,    Curve_Exit,    Curve_Continuous),
};

/* ---- 迁移表 ---- */
static HSM_Transition trans_table[] = {
    HSM_TRANS(NULL, EV_STOP,     NULL),  // Straight → Idle
    HSM_TRANS(NULL, EV_STRAIGHT, NULL),  // Idle     → Straight
    HSM_TRANS(NULL, EV_CURVE,    NULL),  // Idle     → Curve
    HSM_TRANS(NULL, EV_STOP,     NULL),  // Curve    → Idle
};

/* ---- HSM 实例 ---- */
static HSM_Core_Memory hsm_mem;
static HSM* hsm;
static HSM_Node nodes[4];

/* ======== 按键 ======== */
static MulitKey_t key_straight, key_curve, key_stop;

static uint8_t read_key1(void) { return HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin); }
static uint8_t read_key2(void) { return HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin); }
static uint8_t read_key3(void) { return HAL_GPIO_ReadPin(key3_GPIO_Port, key3_Pin); }

static void on_key_straight(MulitKey_t* k) { (void)k; App_FSM_SendEvent(EV_STRAIGHT); }
static void on_key_curve(MulitKey_t* k)    { (void)k; App_FSM_SendEvent(EV_CURVE); }
static void on_key_stop(MulitKey_t* k)     { (void)k; App_FSM_SendEvent(EV_STOP); }

/* ======== Root ======== */
static bool Root_Handler(HSM_Event_Package ev)
{
    (void)ev;
    return false;  // 冒泡给子状态
}

static void Root_Entry(HSM_Event_Package ev) { (void)ev; }
static void Root_Exit(HSM_Event_Package ev)  { (void)ev; }

/* ======== Idle ======== */
static bool Idle_Handler(HSM_Event_Package ev)
{
    switch (ev.HSM_Event_ID) {
    case EV_STRAIGHT:
    case EV_CURVE:
        return false;  // 交给迁移表处理
    default:
        return false;
    }
}

static void Idle_Entry(HSM_Event_Package ev)
{
    (void)ev;
    OLED_Clear();
    Control_Stop();
    App_Timer_Stop();
    LOG_DEBUG("-> Idle");
}

static void Idle_Exit(HSM_Event_Package ev)
{
    (void)ev;
    LOG_DEBUG("<- Idle");
}

static void Idle_Continuous(void)
{
    OLED_ShowString(0, 0, (uint8_t*)"IDLE", 16, 1);
    OLED_Refresh();

}

/* ======== Straight ======== */
static bool Straight_Handler(HSM_Event_Package ev)
{
    switch (ev.HSM_Event_ID) {
    case EV_STOP:
        return false;  // 交给迁移表 → Idle
    default:
        return false;
    }
}

static void Straight_Entry(HSM_Event_Package ev)
{
    (void)ev;
    Control_Start();
    OLED_Clear();
    last_distance = Control_GetCurrentDistance();
    App_Timer_Start();
    LOG_DEBUG("-> Straight");
}

static void Straight_Exit(HSM_Event_Package ev)
{
    (void)ev;
    LOG_DEBUG("<- Straight");
}

static void Straight_Continuous(void)
{
    static uint8_t last_time = 0;
    char buffer[32] = {0};
    Control_GrayByte_Window_Filter(3);
    current_distance = Control_GetCurrentDistance() - last_distance;
    if (current_distance >= straight_target_distance)
    {
        App_FSM_SendEvent(EV_STOP);
        return;
    }
    if (HAL_GetTick() - last_time >= 20)
    {
        App_Protocol_SendVel(Control_GetAverageSpeed());
        last_time = HAL_GetTick();
    }
    LOG_Snprintf(buffer,sizeof(buffer),"Straight:%.2fm",current_distance);
    OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
    LOG_Snprintf(buffer,sizeof(buffer),"Ave_vel=%.2f",Control_GetAverageSpeed());
    OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
    OLED_Refresh();
}

/* ======== Curve ======== */
static bool Curve_Handler(HSM_Event_Package ev)
{
    switch (ev.HSM_Event_ID) {
    case EV_STOP:
        return false;  // 交给迁移表 → Idle
    default:
        return false;
    }
}

static void Curve_Entry(HSM_Event_Package ev)
{
    (void)ev;
    Control_Start();
    OLED_Clear();
    last_distance = Control_GetCurrentDistance();
    App_Timer_Start();
    LOG_DEBUG("-> Curve");
}

static void Curve_Exit(HSM_Event_Package ev)
{
    (void)ev;
    LOG_DEBUG("<- Curve");
}

static void Curve_Continuous(void)
{
    static uint8_t last_time = 0;
    char buffer[32] = {0};
    // Control_GrayByte_Window_Filter(3);
    current_distance = Control_GetCurrentDistance() - last_distance;
    if (current_distance >= curve_target_distance)
    {
        App_FSM_SendEvent(EV_STOP);
        return;
    }
    if (HAL_GetTick() - last_time >= 100)
    {
        App_Protocol_SendVel(Control_GetAverageSpeed());
        last_time = HAL_GetTick();
    }
    LOG_Snprintf(buffer,sizeof(buffer),"Curve:%.2fm",current_distance);
    OLED_ShowString(0, 0, (uint8_t*)buffer, 16, 1);
    LOG_Snprintf(buffer,sizeof(buffer),"Ave_vel=%.2f",Control_GetAverageSpeed());
    OLED_ShowString(0, 16, (uint8_t*)buffer, 16, 1);
    OLED_Refresh();
}

/* ======== 公开接口 ======== */
void App_FSM_Init(void)
{
    hsm = HSM_Create(&hsm_mem, nodes, 4, state_table,
                     sizeof(state_table) / sizeof(state_table[0]));

    /* EV_STOP: Straight → Idle */
    trans_table[0].from = HSM_FindNode(hsm, "Straight");
    trans_table[0].to   = HSM_FindNode(hsm, "Idle");
    /* EV_STRAIGHT: Idle → Straight */
    trans_table[1].from = HSM_FindNode(hsm, "Idle");
    trans_table[1].to   = HSM_FindNode(hsm, "Straight");
    /* EV_CURVE: Idle → Curve */
    trans_table[2].from = HSM_FindNode(hsm, "Idle");
    trans_table[2].to   = HSM_FindNode(hsm, "Curve");
    /* EV_STOP: Curve → Idle */
    trans_table[3].from = HSM_FindNode(hsm, "Curve");
    trans_table[3].to   = HSM_FindNode(hsm, "Idle");

    HSM_RegisterTransitions(hsm, trans_table,
                            sizeof(trans_table) / sizeof(trans_table[0]));

    HSM_Start(hsm, HSM_FindNode(hsm, "Idle"));

    MulitKey_Init(&key_straight, read_key1, on_key_straight, NULL, RISE_BORDER_TRIGGER);
    MulitKey_Init(&key_curve,    read_key2, on_key_curve,    NULL, RISE_BORDER_TRIGGER);
    MulitKey_Init(&key_stop,     read_key3, on_key_stop,     NULL, RISE_BORDER_TRIGGER);

    LOG_INFO("FSM init done (key1=straight, key2=curve, key3=stop)");
}

void App_FSM_Process(void)
{
    char buffer[32] = {0};
    MulitKey_Scan(&key_straight);
    MulitKey_Scan(&key_curve);
    MulitKey_Scan(&key_stop);
    HSM_Process(hsm);
    App_Timer_Update();
    App_Timer_GetString(buffer,sizeof(buffer));
    OLED_ShowString(0,32,buffer,16,1);
    LOG_Snprintf(buffer,sizeof(buffer),"Distance: % .2fm",current_distance);
    OLED_ShowString(0, 48, (uint8_t*)buffer, 16, 1);
    OLED_Refresh();
}

void App_FSM_SendEvent(uint8_t event_id)
{
    HSM_SendEvent(hsm, (HSM_Event_Package){ .HSM_Event_ID = event_id });
}
