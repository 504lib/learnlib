#include "app_protocol.h"
#include "usart.h"
#include "string.h"
#include "main.h"

extern volatile uint8_t uart2_rx_byte;
extern volatile uint8_t uart3_rx_byte;

// ---------- 应用命令队列（摄像头和蓝牙共用）----------
DECLARE_STATIC_QUEUE(UART_PROTOCOL_CAM_QUEUE, uint8_t, UART_PROTOCOL_FRAME_BUFFER_LEN * 2);
DECLARE_STATIC_QUEUE(UART_PROTOCOL_BT_QUEUE,  uint8_t, UART_PROTOCOL_FRAME_BUFFER_LEN * 2);
DECLARE_STATIC_QUEUE(App_CmdQueue_Cam, uint8_t, APP_CMD_QUEUE_SIZE);
DECLARE_STATIC_QUEUE(App_CmdQueue,     uint8_t, APP_CMD_QUEUE_SIZE);

static UART_PROTOCOL_CAM_QUEUE_t cam_queue = {0};
static UART_PROTOCOL_BT_QUEUE_t  bt_queue  = {0};
static App_CmdQueue_Cam_t app_cmd_queue_cam = {0};
static App_CmdQueue_t     app_cmd_queue     = {0};

// ---------- 串口发送函数（各自绑定不同串口）----------
// 注意：必须保护 USART2 RX 中断，因为 HAL_UART_Transmit（阻塞）
// 和 HAL_UART_Receive_IT（中断）共用同一个 huart2 句柄会冲突。
// 发送期间短暂关闭中断，发完后重启 RX IT 接收。
static bool uart_send_cam(const uint8_t* data, uint16_t len) {
    // HAL_NVIC_DisableIRQ(USART2_IRQn);
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 100);
    // 发送期间 RX 中断被屏蔽，可能有字节残留在硬件 buffer 中
    // AbortReceive 清掉旧的 BUSY_RX 状态，重新干净注册接收
    // HAL_UART_AbortReceive(&huart2);
    // HAL_UART_Receive_IT(&huart2, (uint8_t*)&uart2_rx_byte, 1);
    // HAL_NVIC_EnableIRQ(USART2_IRQn);
    return (status == HAL_OK);
}

static bool uart_send_bt(const uint8_t* data, uint16_t len) {
    // HAL_NVIC_DisableIRQ(USART3_IRQn);
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, (uint8_t*)data, len, 100);
    // HAL_UART_Receive_IT(&huart3, (uint8_t*)&uart3_rx_byte, 1);
    // HAL_NVIC_EnableIRQ(USART3_IRQn);
    return (status == HAL_OK);
}

// ---------- 命令队列操作 ----------
// 摄像头命令队列
bool App_CmdDequeue_Cam(uint8_t *cmd) {
    return App_CmdQueue_Cam_POP(&app_cmd_queue_cam, cmd);
}

bool App_CmdEnqueue_Cam(uint8_t cmd) {
    return App_CmdQueue_Cam_PUSH(&app_cmd_queue_cam, cmd);
}

// 蓝牙命令队列
bool App_CmdDequeue(uint8_t *cmd) {
    return App_CmdQueue_POP(&app_cmd_queue, cmd);
}

bool App_CmdEnqueue(uint8_t cmd) {
    return App_CmdQueue_PUSH(&app_cmd_queue, cmd);
}

// ---------- 帧接收回调 ----------
// 摄像头帧回调：来自摄像头的识别结果，压入应用命令队列
static void on_cam_frame_received(uint8_t frame_type, const uint8_t* payload, uint16_t len) {
    LOG_INFO("[CAM] frame type=0x%02X, len=%d", frame_type, len);
    for (size_t i = 0; i < len; i++) {
        LOG_INFO("[CAM] payload[%d]=0x%02X", i, payload[i]);
    }
    App_CmdEnqueue_Cam(frame_type);   // 摄像头命令 → 摄像头队列
}

// 蓝牙帧回调：来自蓝牙/PC 的测试指令或控制命令
static void on_bt_frame_received(uint8_t frame_type, const uint8_t* payload, uint16_t len) {
    LOG_INFO("[BT] frame type=0x%02X, len=%d", frame_type, len);
    for (size_t i = 0; i < len; i++) {
        LOG_INFO("[BT] payload[%d]=0x%02X", i, payload[i]);
    }
    App_CmdEnqueue(frame_type);
}

// ---------- 双协议实例 ----------
static UART_protocol_t protocol_cam;   // USART2 摄像头
static UART_protocol_t protocol_bt;    // USART3 蓝牙

void App_Protocol_Init(void) {
    App_CmdQueue_Cam_INIT(&app_cmd_queue_cam);
    App_CmdQueue_INIT(&app_cmd_queue);

    Custom_Frame_HT_T frame_ht = {0xAA, 0x55, 0x55, 0xAA};

    // ---- 摄像头协议实例 ----
    UART_PROTOCOL_CAM_QUEUE_INIT(&cam_queue);
    Queue_Operations cam_q_ops = {
        .Queue_instance = &cam_queue,
        .Queue_pushback = UART_PROTOCOL_CAM_QUEUE_PUSH,
        .Queue_popfront = UART_PROTOCOL_CAM_QUEUE_POP
    };
    Uart_Protocol_FunctionsParameters cam_req = {
        .Head_Tial_Frame_struct = frame_ht,
        .transmit_function = uart_send_cam,
        .frame_received_handler = on_cam_frame_received,
        .queue_ops = cam_q_ops
    };
    Uart_Protocol_OptionalFunctionsParameters opt = {NULL, NULL};
    Uart_Protocol_Init(&protocol_cam, cam_req, opt);

    // ---- 蓝牙协议实例 ----
    UART_PROTOCOL_BT_QUEUE_INIT(&bt_queue);
    Queue_Operations bt_q_ops = {
        .Queue_instance = &bt_queue,
        .Queue_pushback = UART_PROTOCOL_BT_QUEUE_PUSH,
        .Queue_popfront = UART_PROTOCOL_BT_QUEUE_POP
    };
    Uart_Protocol_FunctionsParameters bt_req = {
        .Head_Tial_Frame_struct = frame_ht,
        .transmit_function = uart_send_bt,
        .frame_received_handler = on_bt_frame_received,
        .queue_ops = bt_q_ops
    };
    Uart_Protocol_Init(&protocol_bt, bt_req, opt);
}

void App_Protocol_Run(void) {
    Uart_Protocol_Loop(&protocol_cam);
    Uart_Protocol_Loop(&protocol_bt);
}

UART_protocol_t* App_GetProtocolInstance_Cam(void) {
    return &protocol_cam;
}

UART_protocol_t* App_GetProtocolInstance_Bt(void) {
    return &protocol_bt;
}

bool App_SendCamFrame(uint8_t type, const uint8_t* data, uint8_t len) {
    return Uart_Protocol_Transmit_Frame(&protocol_cam, data, type, len);
}
