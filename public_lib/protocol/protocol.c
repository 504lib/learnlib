#include "protocol.h"


#define DEFAULT_TIMEOUT_THRESHOLD_MS        200
#define DEFALUT_TIMEOUT_MAX_RETRY_TIMES     3


typedef enum
{
    isNone = 0,
    isInitialized = (1 << 0),                 // 协议库是否已初始化
    isEnableAck =   (1 << 1),            // 是否启用ACK机制
    isWatingForAck = (1 << 2),          // 是否正在等待数据帧的ACK
}Flag_Type;
typedef enum
{
    UART_protocol_WaitingHeader1 = 0,          // 等待帧头一
    UART_protocol_WaitingHeader2,              // 等待帧头二
    UART_protocol_WaitingType,                // 等待帧类型
    UART_protocol_WaitingLength,              // 等待帧长度
    UART_protocol_ReceivingPayload,           // 正在接收数据载荷
    UART_protocol_WaitingChecksum1,           // 等待校验高字节
    UART_protocol_WaitingChecksum2,           // 等待校验低字节
    UART_protocol_WaitingTail1,               // 等待帧尾一
    UART_protocol_WaitingTail2,               // 等待帧尾二
}Frame_Process_Type;


/**
 * @brief    校验和计算
 * 
 * @param    data      数据指针
 * @param    length    计算的数据长度，注意边界问题
 * @return   uint16_t  校验和的值
 */
static uint16_t calculateChecksum(const uint8_t* data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) 
    {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief   反序列化（大端→本机端）：从 4 字节大端序读出 uint32_t
 */
uint32_t rd_u32_be(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           (uint32_t)p[3];
}

/**
 * @brief   序列化（本机端→大端）：将 uint32_t 按大端序写入 4 字节
 */
void wr_u32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFF);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >> 8)  & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}

/**
 * @brief   反序列化（大端→本机端）：从 4 字节大端序读出 float
 * @note    通过 memcpy 将位模式搬运到本机 float
 */
float rd_f32_be(const uint8_t* p)
{
    uint32_t u = rd_u32_be(p);
    float f;
    memcpy(&f, &u, sizeof(u));
    return f;
}

/**
 * @brief   序列化（本机端→大端）：将 float 按大端序写入 4 字节
 * @note    通过 memcpy 将位模式搬运为 uint32_t 后写入
 */
void wr_f32_be(uint8_t* p, float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    wr_u32_be(p, u);
}



static inline bool __Uart_Protocol_IsInitialized(UART_protocol_t* protocol_instance)
{
    return (protocol_instance->hander_flags & (uint32_t)isInitialized) != 0;           // 检查已初始化标志
}

static inline bool __Uart_Protocol_Init_Frame_Process_Type(UART_protocol_t* protocol_instance)
{
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;                                       // 初始化状态机状态
    return true;
}

static inline bool __Uart_Protocol_Init_CheckPayLoadLen()
{
    return (MAX_PAYLOAD_LEN > 0) && (MAX_PAYLOAD_LEN <= 255);    // 数据载荷长度必须大于0且不超过65535（因为长度字段为1字节）
}

static inline bool __Uart_Protocol_Check_isNULLPointer(void* ptr)
{
    return !ptr;
}

static inline bool __Uart_Protocol_Check_isFatalInputRequiredParameters(Uart_Protocol_FunctionsParameters params)
{
    if (params.Head_Tial_Frame_struct.Headerframe1 == 0 && 
        params.Head_Tial_Frame_struct.Headerframe2 == 0 && 
        params.Head_Tial_Frame_struct.Tailframe1 == 0 && 
        params.Head_Tial_Frame_struct.Tailframe2 == 0)
    {
        LOG_FATAL("Frame structure parameters are all zero. Initialization failed.");
        return true;
    }
    
    if (!params.transmit_function)
    {
        LOG_FATAL("Transmit function pointer is NULL. Initialization failed.");
        return true;
    }
    if (!params.frame_received_handler)
    {
        LOG_FATAL("Frame received handler pointer is NULL. Initialization failed.");
        return true;
    }
    if (!params.queue_ops.Queue_instance)
    {
        LOG_FATAL("Queue instance pointer is NULL. Initialization failed.");
        return true;
    }
    if (!params.queue_ops.Queue_popfront)
    {
        LOG_FATAL("Queue popfront function pointer is NULL. Initialization failed.");
        return true;
    }
    if (!params.queue_ops.Queue_pushback)
    {
        LOG_FATAL("Queue pushback function pointer is NULL. Initialization failed.");
        return true;
    }
    if (!params.queue_ops.Queue_size)
    {
        LOG_INFO("If Queue push/pop operations is bases on size of Queue itself, Queue_size function pointer can be NULL. Initialization continues.");
    }
    return false;
}

static inline bool __Uart_Protocol_Check_OptionalParameters(Uart_Protocol_OptionalFunctionsParameters params)
{
    bool isEnableAck = true;
    if (!params.GetTick)
    {
        LOG_INFO("GetTick function pointer is NULL. Time-based features will be disabled.");
        isEnableAck = false;
    }
    if (!params.timeout_handler)
    {
        LOG_INFO("Timeout handler function pointer is NULL. Timeout handling will be disabled.");
        isEnableAck = false;
    }
    return isEnableAck;
}

bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,Uart_Protocol_FunctionsParameters RequiredParam,Uart_Protocol_OptionalFunctionsParameters OptionalParam)
{
    if (__Uart_Protocol_Check_isNULLPointer((void*)protocol_instance))
    {
        LOG_FATAL("Protocol instance pointer is NULL. Initialization failed.");
        return;
    }
    if (__Uart_Protocol_Check_isFatalInputRequiredParameters(RequiredParam))
    {
        return false;
    }
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;                                       // 初始化状态机状态    
    memset(protocol_instance->frame_payload, 0, sizeof(protocol_instance->frame_payload));       // 清空数据载荷缓冲区

    protocol_instance->event_handler.current_event_mask = 0;                                                    // 初始化用户事件掩码
    protocol_instance->event_handler.event_group = 0;                                                           // 初始化事件组标识
    protocol_instance->event_handler.frame_received_handler = RequiredParam.frame_received_handler;             // 设置数据帧接收处理函数

    protocol_instance->Send_Operations.transmit_function = RequiredParam.transmit_function;
    protocol_instance->queue_ops = RequiredParam.queue_ops;                                                   // 设置串口数据队列操作函数集合

    protocol_instance->hander_flags = (uint32_t)isNone;
    if (__Uart_Protocol_Check_OptionalParameters(OptionalParam))
    {
        protocol_instance->tickBased_timeout.tick = 0;
        protocol_instance->tickBased_timeout.lastTick = 0;
        protocol_instance->tickBased_timeout.timeout_threshold = DEFAULT_TIMEOUT_THRESHOLD_MS;
        protocol_instance->tickBased_timeout.try_times = 0;
        protocol_instance->tickBased_timeout.max_try_times = DEFALUT_TIMEOUT_MAX_RETRY_TIMES;
        protocol_instance->tickBased_timeout.GetTick = OptionalParam.GetTick;
        protocol_instance->tickBased_timeout.timeout_handler = OptionalParam.timeout_handler;
        protocol_instance->hander_flags |= (uint32_t)isEnableAck;
    }
    else
    {
        protocol_instance->hander_flags &= (uint32_t)~isEnableAck;
    }
    protocol_instance->hander_flags &= (uint32_t)~isWatingForAck;    // 初始化时不等待ACK
    protocol_instance->hander_flags |= (uint32_t)isInitialized;
    return true;
}
    

bool Uart_Protocol_ProcessReceivedData8bit(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    return protocol_instance->queue_ops.Queue_pushback(protocol_instance->queue_ops.Queue_instance, data);
}

bool Uart_Protocol_ProcessReceivedDataBuffer(UART_protocol_t* protocol_instance, uint8_t* data,size_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    bool result = true;
    for (size_t i = 0; i < len && result; i++)
    {
        result = Uart_Protocol_ProcessReceivedData8bit(protocol_instance,data[i]);
    }
    return result;
}

