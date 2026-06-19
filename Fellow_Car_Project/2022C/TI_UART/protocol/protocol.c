#include "protocol.h"

#define DEFAULT_TIMEOUT_THRESHOLD_MS        200
#define DEFALUT_TIMEOUT_MAX_RETRY_TIMES     3
#define UART_PROTOCOL_FRAME_OVERHEAD        8u
#define UART_PROTOCOL_PAYLOAD_OFFSET        4u
#define UART_PROTOCOL_MAX_PAYLOAD_SIZE      (UART_PROTOCOL_FRAME_BUFFER_LEN - UART_PROTOCOL_FRAME_OVERHEAD)

#define UART_PROTOCOL_ACK_TYPE              0xFF

typedef enum
{
    isNone = 0,
    isInitialized = (1 << 0),
    isEnableAck   = (1 << 1),
    isWatingForAck = (1 << 2),
} Flag_Type;

typedef enum
{
    UART_protocol_WaitingHeader1 = 0,
    UART_protocol_WaitingHeader2,
    UART_protocol_WaitingType,
    UART_protocol_WaitingLength,
    UART_protocol_ReceivingPayload,
    UART_protocol_WaitingChecksum1,
    UART_protocol_WaitingChecksum2,
    UART_protocol_WaitingTail1,
    UART_protocol_WaitingTail2,
} Frame_Process_Type;

typedef bool (*Uart_Protocol_Analysis_hander)(UART_protocol_t* protocol_instance, uint8_t data);

static bool Uart_Protocol_TransmitData(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type, uint8_t len);
static inline bool Uart_Protocol_TransmitAck(UART_protocol_t* protocol_instance);
static void __Uart_Protocol_StopWaitingForAck(UART_protocol_t* protocol_instance);

// ============================================================
// 校验和
// ============================================================
static uint16_t calculateChecksum(const uint8_t* data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++)
    {
        sum += data[i];
    }
    return sum;
}

// ============================================================
// 序列化/反序列化（大端序）
// ============================================================
uint32_t rd_u32_be(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           (uint32_t)p[3];
}

void wr_u32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFF);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >> 8)  & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}

float rd_f32_be(const uint8_t* p)
{
    uint32_t u = rd_u32_be(p);
    float f;
    memcpy(&f, &u, sizeof(u));
    return f;
}

void wr_f32_be(uint8_t* p, float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    wr_u32_be(p, u);
}

// ============================================================
// 内部辅助
// ============================================================
static inline void __Uart_Protocol_WriteBitFlag(UART_protocol_t* protocol_instance, Flag_Type flag, bool value)
{
    if (value)
        protocol_instance->hander_flags |= (uint32_t)flag;
    else
        protocol_instance->hander_flags &= (uint32_t)~flag;
}

static inline bool __Uart_Protocol_IsInitialized(UART_protocol_t* protocol_instance)
{
    return (protocol_instance->hander_flags & (uint32_t)isInitialized) != 0;
}

static inline bool __Uart_Protocol_Check_isNULLPointer(const void* pointer)
{
    return pointer == NULL;
}

static inline bool __Uart_Protocol_Check_isFatalInputRequiredParameters(Uart_Protocol_FunctionsParameters params)
{
    if (params.Head_Tial_Frame_struct.Headerframe1 == 0 &&
        params.Head_Tial_Frame_struct.Headerframe2 == 0 &&
        params.Head_Tial_Frame_struct.Tailframe1 == 0 &&
        params.Head_Tial_Frame_struct.Tailframe2 == 0)
    {
        LOG_FATAL("Frame structure parameters are all zero.");
        return true;
    }
    if (!params.transmit_function)
    {
        LOG_FATAL("Transmit function pointer is NULL.");
        return true;
    }
    if (!params.frame_received_handler)
    {
        LOG_FATAL("Frame received handler pointer is NULL.");
        return true;
    }
    if (!params.queue_ops.Queue_instance)
    {
        LOG_FATAL("Queue instance pointer is NULL.");
        return true;
    }
    if (!params.queue_ops.Queue_popfront)
    {
        LOG_FATAL("Queue popfront function pointer is NULL.");
        return true;
    }
    if (!params.queue_ops.Queue_pushback)
    {
        LOG_FATAL("Queue pushback function pointer is NULL.");
        return true;
    }
    return false;
}

static inline bool __Uart_Protocol_Check_OptionalParameters(Uart_Protocol_OptionalFunctionsParameters params)
{
    bool isEnableAck = true;
    if (!params.GetTick)
    {
        LOG_INFO("GetTick function pointer is NULL. ACK disabled.");
        isEnableAck = false;
    }
    if (!params.timeout_handler)
    {
        LOG_INFO("Timeout handler pointer is NULL. ACK disabled.");
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

// ============================================================
// 协议状态机各状态处理函数
// ============================================================
static bool Uart_Protocol_Analysis_WaitingHeader1(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Headerframe1 != data)
        return false;
    protocol_instance->frame_buffer[0] = data;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingHeader2;
    return true;
}

static bool Uart_Protocol_Analysis_WaitingHeader2(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Headerframe2 != data)
    {
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }
    protocol_instance->frame_buffer[1] = data;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingType;
    return true;
}

static bool Uart_Protocol_Analysis_WaitingType(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->event_handler.current_frame_type = data;
    protocol_instance->frame_buffer[2] = data;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingLength;
    return true;
}

static bool Uart_Protocol_Analysis_WaitingLength(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->data_len = (size_t)data;
    if (protocol_instance->data_len > UART_PROTOCOL_FRAME_BUFFER_LEN - UART_PROTOCOL_FRAME_OVERHEAD)
    {
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }
    protocol_instance->frame_buffer[3] = data;
    protocol_instance->payload_index = UART_PROTOCOL_PAYLOAD_OFFSET;
    protocol_instance->Frame_Process_Type =
        (protocol_instance->data_len == 0) ? UART_protocol_WaitingChecksum1 : UART_protocol_ReceivingPayload;
    return true;
}

static bool Uart_Protocol_Analysis_ReceivingPayload(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;
    if (protocol_instance->payload_index >= protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET)
    {
        protocol_instance->Frame_Process_Type = UART_protocol_WaitingChecksum1;
    }
    return true;
}

static bool Uart_Protocol_Analysis_WaitingChecksum1(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingChecksum2;
    return true;
}

static bool Uart_Protocol_Analysis_WaitingChecksum2(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingTail1;
    return true;
}

static bool Uart_Protocol_Analysis_WaitingTail1(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Tailframe1 != data)
    {
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }
    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;
    protocol_instance->Frame_Process_Type = UART_protocol_WaitingTail2;
    return true;
}

static bool Uart_Protocol_Analysis_DealFrame(UART_protocol_t* protocol_instance)
{
    const uint8_t frame_type = protocol_instance->frame_buffer[2];

    if (!protocol_instance) return false;

    // ACK帧由协议层消费
    if (frame_type == UART_PROTOCOL_ACK_TYPE)
    {
        if ((protocol_instance->hander_flags & isWatingForAck) &&
            (protocol_instance->hander_flags & isEnableAck))
        {
            __Uart_Protocol_StopWaitingForAck(protocol_instance);
        }
        return true;
    }

    // 自动回ACK
    if ((protocol_instance->hander_flags & isEnableAck) && !Uart_Protocol_TransmitAck(protocol_instance))
    {
        LOG_WARN("Failed to transmit ACK for type 0x%02X.", frame_type);
    }

    // 调用用户回调
    if (protocol_instance->event_handler.frame_received_handler)
    {
        protocol_instance->event_handler.frame_received_handler(
            protocol_instance->event_handler.current_frame_type,
            &protocol_instance->frame_buffer[UART_PROTOCOL_PAYLOAD_OFFSET],
            protocol_instance->data_len);
    }
    return true;
}

static bool Uart_Protocol_Analysis_WaitingTail2(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->uart_frame_struct.Tailframe2 != data)
    {
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    protocol_instance->frame_buffer[protocol_instance->payload_index++] = data;

    // 校验
    {
        const uint16_t received_checksum =
            ((uint16_t)protocol_instance->frame_buffer[protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET] << 8) |
            (uint16_t)protocol_instance->frame_buffer[protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET + 1];
        const uint16_t calculated_checksum = calculateChecksum(&protocol_instance->frame_buffer[2], protocol_instance->data_len + 2);

        if (received_checksum == calculated_checksum)
        {
            Uart_Protocol_Analysis_DealFrame(protocol_instance);
        }
        else
        {
            LOG_DEBUG("Checksum mismatch: rx=0x%04X calc=0x%04X.", received_checksum, calculated_checksum);
        }
    }

    __Uart_Protocol_ResetReceiveState(protocol_instance);
    return true;
}

// ============================================================
// 函数指针表
// ============================================================
static const Uart_Protocol_Analysis_hander Uart_Protocol_Analysis_hander_state[] = {
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
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;

    uint8_t data;
    void* queue_instance = protocol_instance->queue_ops.Queue_instance;
    if (!protocol_instance->queue_ops.Queue_popfront(queue_instance, &data))
        return true;

    if (protocol_instance->Frame_Process_Type > UART_protocol_WaitingTail2)
    {
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    return Uart_Protocol_Analysis_hander_state[protocol_instance->Frame_Process_Type](protocol_instance, data);
}

// ============================================================
// 公开API
// ============================================================
bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,
                        Uart_Protocol_FunctionsParameters RequiredParam,
                        Uart_Protocol_OptionalFunctionsParameters OptionalParam)
{
    if (__Uart_Protocol_Check_isNULLPointer((void*)protocol_instance)) return false;
    if (__Uart_Protocol_Check_isFatalInputRequiredParameters(RequiredParam)) return false;

    protocol_instance->Frame_Process_Type = (uint8_t)UART_protocol_WaitingHeader1;
    protocol_instance->uart_frame_struct = RequiredParam.Head_Tial_Frame_struct;
    memset(protocol_instance->frame_buffer, 0, sizeof(protocol_instance->frame_buffer));

    protocol_instance->event_handler.current_frame_type = 0;
    protocol_instance->event_handler.frame_received_handler = RequiredParam.frame_received_handler;

    protocol_instance->Send_Operations.transmit_function = RequiredParam.transmit_function;
    protocol_instance->queue_ops = RequiredParam.queue_ops;

    protocol_instance->hander_flags = (uint32_t)isNone;
    if (__Uart_Protocol_Check_OptionalParameters(OptionalParam))
    {
        protocol_instance->tickBased_timeout.pending_frame_type = 0;
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
    protocol_instance->hander_flags &= (uint32_t)~isWatingForAck;
    protocol_instance->hander_flags |= (uint32_t)isInitialized;
    return true;
}

bool Uart_Protocol_ProcessReceivedData8bit(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;
    return protocol_instance->queue_ops.Queue_pushback(protocol_instance->queue_ops.Queue_instance, data);
}

bool Uart_Protocol_ProcessReceivedDataBuffer(UART_protocol_t* protocol_instance, uint8_t* data, size_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;
    bool result = true;
    for (size_t i = 0; i < len && result; i++)
    {
        result = Uart_Protocol_ProcessReceivedData8bit(protocol_instance, data[i]);
    }
    return result;
}

// ============================================================
// ACK 超时/重传
// ============================================================
static bool __Uart_Protocol_Backup_LastTransmitData(UART_protocol_t* protocol_instance, const uint8_t* data, size_t len)
{
    if ((data == NULL) && len > 0) return false;
    if (len > UART_PROTOCOL_FRAME_BUFFER_LEN) return false;
    memcpy(protocol_instance->tickBased_timeout.temp_transmit_buffer, data, len);
    protocol_instance->tickBased_timeout.temp_buffer_len = len;
    return true;
}

static void __Uart_Protocol_BeginWaitingForAck(UART_protocol_t* protocol_instance)
{
    if ((protocol_instance->hander_flags & isEnableAck) == 0) return;
    __Uart_Protocol_WriteBitFlag(protocol_instance, isWatingForAck, true);
    protocol_instance->tickBased_timeout.try_times = 0;
    if (protocol_instance->tickBased_timeout.GetTick)
    {
        protocol_instance->tickBased_timeout.lastTick = protocol_instance->tickBased_timeout.GetTick();
    }
}

static void __Uart_Protocol_StopWaitingForAck(UART_protocol_t* protocol_instance)
{
    __Uart_Protocol_WriteBitFlag(protocol_instance, isWatingForAck, false);
    protocol_instance->tickBased_timeout.try_times = 0;
    protocol_instance->tickBased_timeout.temp_buffer_len = 0;
    protocol_instance->tickBased_timeout.pending_frame_type = 0;
    memset(protocol_instance->tickBased_timeout.temp_transmit_buffer, 0,
           sizeof(protocol_instance->tickBased_timeout.temp_transmit_buffer));
}

static size_t __Uart_Protocol_Framing(Custom_Frame_HT_T HT, uint8_t* buffer, size_t buffer_size,
                                       uint8_t type, const uint8_t* data, uint8_t payload_size)
{
    if ((buffer == NULL) || (buffer_size < UART_PROTOCOL_FRAME_OVERHEAD + payload_size)) return 0;
    if ((data == NULL) && (payload_size > 0)) return 0;

    buffer[0] = HT.Headerframe1;
    buffer[1] = HT.Headerframe2;
    buffer[2] = type;
    buffer[3] = payload_size;
    if (payload_size > 0)
        memcpy(&buffer[UART_PROTOCOL_PAYLOAD_OFFSET], data, payload_size);
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size]     = (calculateChecksum(&buffer[2], payload_size + 2) >> 8) & 0xFF;
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size + 1] = calculateChecksum(&buffer[2], payload_size + 2) & 0xFF;
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size + 2] = HT.Tailframe1;
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size + 3] = HT.Tailframe2;
    return UART_PROTOCOL_FRAME_OVERHEAD + payload_size;
}

static bool __Uart_Protocol_TransmitRawData(UART_protocol_t* protocol_instance, const uint8_t* data, uint16_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;
    if ((data == NULL) && (len > 0)) return false;
    if (!protocol_instance->Send_Operations.transmit_function) return false;
    return protocol_instance->Send_Operations.transmit_function(data, len);
}

static bool Uart_Protocol_TransmitData(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type, uint8_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;
    if ((data == NULL) && (len > 0)) return false;
    if (!protocol_instance->Send_Operations.transmit_function) return false;
    if (len > UART_PROTOCOL_MAX_PAYLOAD_SIZE) return false;

    uint8_t frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    size_t frame_len = __Uart_Protocol_Framing(protocol_instance->uart_frame_struct, frame, sizeof(frame), type, data, len);
    if (frame_len == 0) return false;

    bool success = protocol_instance->Send_Operations.transmit_function(frame, frame_len);
    if (!success) return false;

    if (type != UART_PROTOCOL_ACK_TYPE)
    {
        protocol_instance->tickBased_timeout.pending_frame_type = type;
        __Uart_Protocol_BeginWaitingForAck(protocol_instance);
        (void)__Uart_Protocol_Backup_LastTransmitData(protocol_instance, frame, frame_len);
    }
    return success;
}

static bool __Uart_Protocol_Retry(UART_protocol_t* protocol_instance)
{
    if (protocol_instance->tickBased_timeout.try_times < protocol_instance->tickBased_timeout.max_try_times)
    {
        protocol_instance->tickBased_timeout.try_times++;
        return __Uart_Protocol_TransmitRawData(protocol_instance,
            protocol_instance->tickBased_timeout.temp_transmit_buffer,
            protocol_instance->tickBased_timeout.temp_buffer_len);
    }
    else
    {
        const uint8_t pending_frame_type = protocol_instance->tickBased_timeout.pending_frame_type;
        __Uart_Protocol_StopWaitingForAck(protocol_instance);
        LOG_WARN("ACK timeout, max retries exceeded.");
        if (protocol_instance->tickBased_timeout.timeout_handler)
        {
            protocol_instance->tickBased_timeout.timeout_handler(pending_frame_type);
        }
        return false;
    }
}

static bool __Uart_Protocol_ElapsedTimeExceeded(UART_protocol_t* protocol_instance)
{
    if ((protocol_instance->hander_flags & isEnableAck) == 0 ||
        (protocol_instance->hander_flags & isWatingForAck) == 0)
        return false;
    if (!protocol_instance->tickBased_timeout.GetTick) return false;

    uint32_t current_tick = protocol_instance->tickBased_timeout.GetTick();
    uint32_t elapsed = current_tick - protocol_instance->tickBased_timeout.lastTick;
    if (elapsed > protocol_instance->tickBased_timeout.timeout_threshold)
    {
        protocol_instance->tickBased_timeout.lastTick = current_tick;
        return __Uart_Protocol_Retry(protocol_instance);
    }
    return false;
}

bool Uart_Protocol_Transmit_Frame(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type, uint8_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;
    if (type == UART_PROTOCOL_ACK_TYPE) return false;
    if ((data == NULL) && (len > 0)) return false;
    if (!protocol_instance->Send_Operations.transmit_function) return false;
    if (len > UART_PROTOCOL_MAX_PAYLOAD_SIZE) return false;
    return Uart_Protocol_TransmitData(protocol_instance, data, type, len);
}

bool Uart_Protocol_Loop(UART_protocol_t* protocol_instance)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance)) return false;
    (void)__Uart_Protocol_ElapsedTimeExceeded(protocol_instance);
    return Uart_Protocol_Analysis(protocol_instance);
}
