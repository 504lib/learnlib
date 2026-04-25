#include "protocol.h"


#define DEFAULT_TIMEOUT_THRESHOLD_MS        200
#define DEFALUT_TIMEOUT_MAX_RETRY_TIMES     3
#define UART_PROTOCOL_FRAME_OVERHEAD        8u
#define UART_PROTOCOL_PAYLOAD_OFFSET        4u


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
    return (UART_PROTOCOL_FRAME_BUFFER_LEN > 0) && (UART_PROTOCOL_FRAME_BUFFER_LEN <= 255);    // 整帧缓冲区长度必须大于0且不超过255（长度字段为1字节）
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

static bool Uart_Protocol_Analysis(UART_protocol_t* protocol_instance)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    static size_t len = 0;
    static size_t payload_index = 0;
    uint8_t data;
    void* queue_instance = protocol_instance->queue_ops.Queue_instance;
    if (!protocol_instance->queue_ops.Queue_popfront(queue_instance,&data))
    {
        return true;  // 从队列获取数据失败,队列数据不存在稍后重试
    }
    switch (protocol_instance->Frame_Process_Type)
    {
    case UART_protocol_WaitingHeader1:
        if (protocol_instance->uart_frame_struct.Headerframe1 != data)
        {
            LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Headerframe1. Ignoring and continuing to wait.", data);
            return false;   // 接收到的字节不匹配帧头一，继续等待
        }
        protocol_instance->frame_buffer[0] = data;    // 存储帧头一
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader2;    // 转到等待帧头二的状态
        break;
    case UART_protocol_WaitingHeader2:
        if (protocol_instance->uart_frame_struct.Headerframe2 != data)
        {
            LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Headerframe2. Resetting to wait for Headerframe1.", data);
            protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;    // 接收到的字节不匹配帧头二，重置状态机回到等待帧头一
            return false;
        }
        protocol_instance->frame_buffer[1] = data;    // 存储帧头二
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingType;    // 转到等待帧类型的状态
        break;
    case UART_protocol_WaitingType:
        protocol_instance->frame_buffer[2] = data;    // 存储帧类型
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingLength;    // 转到等待帧长度的状态
        break;
    case UART_protocol_WaitingLength:
        len = (size_t)data;
        if (len > UART_PROTOCOL_FRAME_BUFFER_LEN - UART_PROTOCOL_FRAME_OVERHEAD)  // 整帧缓存需要容纳帧头、类型、长度、校验和和帧尾
        {
            len = 0;
            protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;
            LOG_DEBUG("length byte indicates payload length %zu exceeds maximum. Resetting to wait for Headerframe1.", (size_t)data);
            return false;   // 接收到的长度字节超过最大载荷长度，重置状态机回到等待帧头一
        }
        protocol_instance->frame_buffer[3] = data;    // 存储帧长度
        payload_index = UART_PROTOCOL_PAYLOAD_OFFSET;    // 数据载荷从frame_buffer[4]开始存储
        protocol_instance->Frame_Process_Type =
            (len == 0) ? UART_protocol_WaitingChecksum1 : UART_protocol_ReceivingPayload;    // 零长度帧不应继续接收载荷
        break;
    case UART_protocol_ReceivingPayload:
        protocol_instance->frame_buffer[payload_index++] = data;    // 存储数据载荷
        if (payload_index >= len + UART_PROTOCOL_PAYLOAD_OFFSET)    // 已经接收了指定长度的数据载荷
        {
            protocol_instance->Frame_Process_Type = UART_protocol_WaitingChecksum1;    // 转到等待校验高字节的状态
        }
        break;
    case UART_protocol_WaitingChecksum1:
        protocol_instance->frame_buffer[payload_index++] = data;    // 存储校验高字节
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingChecksum2;    // 转到等待校验低字节的状态
        break;
    case UART_protocol_WaitingChecksum2:
        protocol_instance->frame_buffer[payload_index++] = data;    // 存储校验低字节
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingTail1;    // 转到等待帧尾一的状态
        break;
    case UART_protocol_WaitingTail1:
        if (protocol_instance->uart_frame_struct.Tailframe1 != data)
        {
            LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Tailframe1. Resetting to wait for Headerframe1.", data);
            protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;    // 接收到的字节不匹配帧尾一，重置状态机回到等待帧头一
            return false;
        }
        protocol_instance->frame_buffer[payload_index++] = data;    // 存储帧尾一
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingTail2;    // 转到等待帧尾二的状态
        break;
    case UART_protocol_WaitingTail2:
        if (protocol_instance->uart_frame_struct.Tailframe2 != data)
        {
            LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Tailframe2. Resetting to wait for Headerframe1.", data);
            protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;    // 接收到的字节不匹配帧尾二，重置状态机回到等待帧头一
            return false;
        }
        protocol_instance->frame_buffer[payload_index++] = data;    // 存储帧尾二
        // 已经接收完整帧，进行校验和验证
        uint16_t received_checksum = ((uint16_t)protocol_instance->frame_buffer[len + UART_PROTOCOL_PAYLOAD_OFFSET] << 8) |
                                      (uint16_t)protocol_instance->frame_buffer[len + UART_PROTOCOL_PAYLOAD_OFFSET + 1];
        uint16_t calculated_checksum = calculateChecksum(&protocol_instance->frame_buffer[2], len + 2);
        if (received_checksum == calculated_checksum)
        {
            // 校验成功，调用用户的帧接收处理函数
            protocol_instance->event_handler.frame_received_handler(protocol_instance->frame_buffer, len + UART_PROTOCOL_FRAME_OVERHEAD);
        }
        else
        {
            LOG_DEBUG("Checksum mismatch: received 0x%04X but calculated 0x%04X. Discarding frame.", received_checksum, calculated_checksum);
            // 校验失败，丢弃该帧（不调用用户处理函数）
        }
        len = 0;
        payload_index = 0;
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;    // 重置状态机回到等待帧头一
        break;
    default:
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;    // 出现未知状态，重置状态机回到等待帧头一
        LOG_WARN("Unknown frame processing state. Resetting to wait for Headerframe1.");
        return false;
        break;
    }
    return true;
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
    protocol_instance->Frame_Process_Type = (uint8_t)UART_protocol_WaitingHeader1;                                       // 初始化状态机状态    
    memset(protocol_instance->frame_buffer, 0, sizeof(protocol_instance->frame_buffer));       // 清空整帧接收缓冲区

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

bool Uart_Protocol_Loop(UART_protocol_t* protocol_instance)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
}
