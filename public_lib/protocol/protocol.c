#include "protocol.h"


#define DEFAULT_TIMEOUT_THRESHOLD_MS        200
#define DEFALUT_TIMEOUT_MAX_RETRY_TIMES     3
#define UART_PROTOCOL_FRAME_OVERHEAD        8u
#define UART_PROTOCOL_PAYLOAD_OFFSET        4u
#define UART_PROTOCOL_MAX_PAYLOAD_SIZE      (UART_PROTOCOL_FRAME_BUFFER_LEN - UART_PROTOCOL_FRAME_OVERHEAD)

#define UART_PROTOCOL_ACK_TYPE              0xFF    // ACK帧类型定义为0xFF，用户应避免使用此类型作为普通数据帧类型

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

typedef bool (*Uart_Protocol_Analysis_hander)(UART_protocol_t* protocol_instance, uint8_t data);

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

static void __Uart_Protocol_ResetReceiveState(UART_protocol_t* protocol_instance)
{
    protocol_instance->data_len = 0;
    protocol_instance->payload_index = 0;
    protocol_instance->event_handler.current_frame_type = 0;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader1;
}

static bool Uart_Protocol_Analysis_WaitingHeader1(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Headerframe1 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Headerframe1. Ignoring and continuing to wait.", data);
        return false;   // 接收到的字节不匹配帧头一，继续等待
    }
    protocol_instance->frame_buffer[0] = data;    // 存储帧头一
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader2;    // 转到等待帧头二的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingHeader2(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Headerframe2 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Headerframe2. Resetting to wait for Headerframe1.", data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }
    protocol_instance->frame_buffer[1] = data;    // 存储帧头二
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingType;    // 转到等待帧类型的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingType(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->event_handler.current_frame_type = data;    // 记录当前帧类型，具体是否处理交给用户回调决定
    protocol_instance->frame_buffer[2] = data;    // 存储帧类型
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingLength;    // 转到等待帧长度的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingLength(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->data_len = (size_t)data;
    if (protocol_instance->data_len > UART_PROTOCOL_FRAME_BUFFER_LEN - UART_PROTOCOL_FRAME_OVERHEAD)
    {
        LOG_DEBUG("length byte indicates payload length %zu exceeds maximum. Resetting to wait for Headerframe1.", (size_t)data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    protocol_instance->frame_buffer[3] = data;    // 存储帧长度
    protocol_instance->payload_index = UART_PROTOCOL_PAYLOAD_OFFSET;    // 数据载荷从frame_buffer[4]开始存储
    protocol_instance->Frame_Process_Type =
        (protocol_instance->data_len == 0) ? UART_protocol_WaitingChecksum1 : UART_protocol_ReceivingPayload;
    return true;
}

static bool Uart_Protocol_Analysis_ReceivingPayload(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;    // 存储数据载荷
    if (protocol_instance->payload_index >= protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET)
    {
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingChecksum1;    // 转到等待校验高字节的状态
    }
    return true;
}

static bool Uart_Protocol_Analysis_WaitingChecksum1(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;    // 存储校验高字节
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingChecksum2;    // 转到等待校验低字节的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingChecksum2(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;    // 存储校验低字节
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingTail1;    // 转到等待帧尾一的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingTail1(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Tailframe1 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Tailframe1. Resetting to wait for Headerframe1.", data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;    // 存储帧尾一
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingTail2;    // 转到等待帧尾二的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingTail2(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Tailframe2 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Tailframe2. Resetting to wait for Headerframe1.", data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;    // 存储帧尾二

    {
        const uint16_t received_checksum = ((uint16_t)protocol_instance->frame_buffer[protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET] << 8) |
            (uint16_t)protocol_instance->frame_buffer[protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET + 1];
        const uint16_t calculated_checksum = calculateChecksum(&protocol_instance->frame_buffer[2], protocol_instance->data_len + 2);

        if (received_checksum == calculated_checksum )
        {
            if (protocol_instance->frame_buffer[2] == UART_PROTOCOL_ACK_TYPE && (protocol_instance->hander_flags & isWatingForAck) )
            {
                protocol_instance->hander_flags &= (uint32_t)~isWatingForAck;    // 收到ACK帧，退出等待ACK状态
                LOG_DEBUG("Received ACK frame. Exiting waiting for ACK state.");
                __Uart_Protocol_ResetReceiveState(protocol_instance);
                return true;    // ACK帧不进入用户回调处理，直接返回
            }
            if (protocol_instance->hander_flags & isEnableAck )
            {
                Uart_Protocol_TransmitAck(protocol_instance);
            }
            if (protocol_instance->event_handler.frame_received_handler)
            {
                protocol_instance->event_handler.frame_received_handler(
                    protocol_instance->event_handler.current_frame_type,
                    &protocol_instance->frame_buffer[UART_PROTOCOL_PAYLOAD_OFFSET],
                    protocol_instance->data_len);
            }
        }
        else
        {
            LOG_DEBUG("Checksum mismatch: received 0x%04X but calculated 0x%04X. Discarding frame.", received_checksum, calculated_checksum);
        }
    }

    __Uart_Protocol_ResetReceiveState(protocol_instance);
    return true;
}

static const Uart_Protocol_Analysis_hander Uart_Protocol_Analysis_hander_state[UART_protocol_WaitingTail2 + 1] = {
    Uart_Protocol_Analysis_WaitingHeader1,
    Uart_Protocol_Analysis_WaitingHeader2,
    Uart_Protocol_Analysis_WaitingType,
    Uart_Protocol_Analysis_WaitingLength,
    Uart_Protocol_Analysis_ReceivingPayload,
    Uart_Protocol_Analysis_WaitingChecksum1,
    Uart_Protocol_Analysis_WaitingChecksum2,
    Uart_Protocol_Analysis_WaitingTail1,
    Uart_Protocol_Analysis_WaitingTail2,
};

static inline bool Uart_Protocol_TransmitAck(UART_protocol_t* protocol_instance)
{
    return Uart_Protocol_TransmitData(protocol_instance, NULL, UART_PROTOCOL_ACK_TYPE, 0);
}

static bool Uart_Protocol_Analysis(UART_protocol_t* protocol_instance)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    uint8_t data;
    void* queue_instance = protocol_instance->queue_ops.Queue_instance;
    if (!protocol_instance->queue_ops.Queue_popfront(queue_instance,&data))
    {
        return true;  // 从队列获取数据失败,队列数据不存在稍后重试
    }

    if (protocol_instance->Frame_Process_Type > UART_protocol_WaitingTail2)
    {
        LOG_WARN("Unknown frame processing state. Resetting to wait for Headerframe1.");
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    return Uart_Protocol_Analysis_hander_state[protocol_instance->Frame_Process_Type](protocol_instance, data);
}

bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,Uart_Protocol_FunctionsParameters RequiredParam,Uart_Protocol_OptionalFunctionsParameters OptionalParam)
{
    if (__Uart_Protocol_Check_isNULLPointer((void*)protocol_instance))
    {
        LOG_FATAL("Protocol instance pointer is NULL. Initialization failed.");
        return false;
    }
    if (__Uart_Protocol_Check_isFatalInputRequiredParameters(RequiredParam))
    {
        return false;
    }
    protocol_instance->Frame_Process_Type = (uint8_t)UART_protocol_WaitingHeader1;                                       // 初始化状态机状态    
    memset(protocol_instance->frame_buffer, 0, sizeof(protocol_instance->frame_buffer));       // 清空整帧接收缓冲区

    protocol_instance->event_handler.current_frame_type = 0;                                                    // 初始化当前帧类型
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

static bool Uart_Protocol_TransmitData(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type , uint8_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    if ((data == NULL) && (len > 0))
    {
        LOG_FATAL("Data pointer is NULL while payload length is %u. Cannot transmit data.", len);
        return false;
    }
    if (!protocol_instance->Send_Operations.transmit_function)
    {
        LOG_FATAL("Transmit function pointer is NULL. Cannot transmit data.");
        return false;
    }
    if (len > UART_PROTOCOL_MAX_PAYLOAD_SIZE)
    {
        LOG_ERROR("Data length %u exceeds maximum payload size. Cannot transmit data.", len);
        return false;
    }
    uint8_t frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    frame[0] = protocol_instance->uart_frame_struct.Headerframe1;
    frame[1] = protocol_instance->uart_frame_struct.Headerframe2;   
    frame[2] = type;    // 帧类型
    frame[3] = len;    // 帧长度
    if (len > 0)
    {
        memcpy(&frame[UART_PROTOCOL_PAYLOAD_OFFSET], data, len);    // 数据载荷
    }
    uint16_t checksum = calculateChecksum(&frame[2], len + 2);    // 计算帧类型、帧长度和数据载荷的校验和
    frame[UART_PROTOCOL_PAYLOAD_OFFSET + len] = (checksum >> 8) & 0xFF;    // 校验高字节
    frame[UART_PROTOCOL_PAYLOAD_OFFSET + len + 1] = checksum & 0xFF;    // 校验低字节
    frame[UART_PROTOCOL_PAYLOAD_OFFSET + len + 2] = protocol_instance->uart_frame_struct.Tailframe1;    // 帧尾一
    frame[UART_PROTOCOL_PAYLOAD_OFFSET + len + 3] = protocol_instance->uart_frame_struct.Tailframe2;    // 帧尾二
    bool success =  protocol_instance->Send_Operations.transmit_function(frame, UART_PROTOCOL_FRAME_OVERHEAD + len);
    if (!success)
    {
        LOG_WARN("Failed to transmit frame. Transmit function returned false.");
    }
    else if (type != UART_PROTOCOL_ACK_TYPE)
    {
        protocol_instance->hander_flags |= (uint32_t)isWatingForAck;    // 只有非ACK帧才进入等待ACK状态
    }
    return success;
}

bool Uart_Protocol_Transmit_Frame(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type , uint8_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    if (type == UART_PROTOCOL_ACK_TYPE)
    {
        LOG_WARN("Frame type 0x%02X is reserved for ACK frames. Consider using a different type for data frames.", UART_PROTOCOL_ACK_TYPE);
        return false;
    }
    if ((data == NULL) && (len > 0))
    {
        LOG_FATAL("Data pointer is NULL while payload length is %u. Cannot transmit data.", len);
        return false;
    }
    if (!protocol_instance->Send_Operations.transmit_function)
    {
        LOG_FATAL("Transmit function pointer is NULL. Cannot transmit data.");
        return false;
    }
    if (len > UART_PROTOCOL_MAX_PAYLOAD_SIZE)
    {
        LOG_ERROR("Data length %u exceeds maximum payload size. Cannot transmit data.", len);
        return false;
    }
    return Uart_Protocol_TransmitData(protocol_instance, data, type, len);
}

bool Uart_Protocol_Loop(UART_protocol_t* protocol_instance)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    return Uart_Protocol_Analysis(protocol_instance);
}


