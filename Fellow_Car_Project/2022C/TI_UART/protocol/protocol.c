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
    isEnableParseWatchDog = (1 << 3),       // 是否使能阶段看门狗
    isCustomQueue = (1 << 4),                   // 是否使用用户自定义队列
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

/* ========================================================================
 *  前向声明
 * ======================================================================== */
static bool Uart_Protocol_TransmitData(UART_protocol_t* protocol_instance, const uint8_t* data, uint8_t type , uint8_t len);
static inline bool Uart_Protocol_TransmitAck(UART_protocol_t* protocol_instance);
static void __Uart_Protocol_StopWaitingForAck(UART_protocol_t* protocol_instance);

/* ========================================================================
 *  Section 1 — 纯工具函数（不访问 protocol_instance，无副作用）
 * ======================================================================== */

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

int16_t rd_i16_be(const uint8_t* p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
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
 * @brief   序列化（本机端→大端）：将 int16_t 按大端序写入 2 字节
 */
void wr_i16_be(uint8_t* p, int16_t v)
{
    uint16_t u = (uint16_t)v;
    p[0] = (uint8_t)((u >> 8) & 0xFF);
    p[1] = (uint8_t)(u & 0xFF);
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

/* ========================================================================
 *  Section 2 — 标志位操作 & 参数校验（只读写 flags / 校验参数合法性）
 * ======================================================================== */

static inline void __Uart_Protocol_WriteBitFlag(UART_protocol_t* protocol_instance, Flag_Type flag,bool value)
{
    if (value)
    {
        protocol_instance->flags |= (uint32_t)flag;    // 设置标志位
    }
    else
    {
        protocol_instance->flags &= (uint32_t)~flag;   // 清除标志位
    }
    
}

static inline bool __Uart_Protocol_ReadBitFlag(const UART_protocol_t* protocol_instance, Flag_Type flag)
{
    return (protocol_instance->flags & (uint32_t)flag) != 0;   // 读取标志位
}

static inline Frame_Process_Type __Uart_Protocol_ReadCurrentState(const UART_protocol_t* protocol_instance)
{
    return (Frame_Process_Type)protocol_instance->parse_state;   // 读取当前状态
}

static inline bool __Uart_Protocol_IsInitialized(UART_protocol_t* protocol_instance)
{
    return (protocol_instance->flags & (uint32_t)isInitialized) != 0;           // 检查已初始化标志
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
    return false;
}

static inline bool __Uart_Protocol_Check_ACKParameters(Uart_Protocol_ACKFunctionsParameters params)
{
    bool isEnable_ACK = true;
    if (!params.GetTick)
    {
        LOG_INFO("GetTick function pointer is NULL. Time-based features will be disabled.");
        isEnable_ACK = false;
    }
    if (!params.timeout_handler)
    {
        LOG_INFO("Timeout handler function pointer is NULL. Timeout handling will be disabled.");
        isEnable_ACK = false;
    }
    return isEnable_ACK;
}

/* ========================================================================
 *  Section 3 — 解析状态机（逐字节接收 → 识别完整帧）
 *
 *  状态流转: H1 → H2 → Type → Len → Payload → CHK1 → CHK2 → T1 → T2
 *                               ↑_____________↑
 *                               Len=0 时跳过 Payload
 *
 *  核心入口: Uart_Protocol_Analysis() 从队列取1字节，查表分发到对应 handler
 *  完整帧出口: Uart_Protocol_Analysis_DealFrame() 校验通过后调用户回调
 * ======================================================================== */

static void __Uart_Protocol_ResetReceiveState(UART_protocol_t* protocol_instance)
{
    protocol_instance->data_len = 0;
    protocol_instance->payload_index = 0;
    protocol_instance->parsing_frame_type = 0;
    protocol_instance->parse_state = UART_protocol_WaitingHeader1;
}

static bool Uart_Protocol_Analysis_WaitingHeader1(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->frame_cfg.Headerframe1 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Headerframe1. Ignoring and continuing to wait.", data);
        return false;   // 接收到的字节不匹配帧头一，继续等待
    }
    protocol_instance->frame_buf[0] = data;    // 存储帧头一
    protocol_instance->parse_state = UART_protocol_WaitingHeader2;    // 转到等待帧头二的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingHeader2(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->frame_cfg.Headerframe2 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Headerframe2. Resetting to wait for Headerframe1.", data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }
    protocol_instance->frame_buf[1] = data;    // 存储帧头二
    protocol_instance->parse_state = UART_protocol_WaitingType;    // 转到等待帧类型的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingType(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->parsing_frame_type = data;    // 记录当前帧类型，具体是否处理交给用户回调决定
    protocol_instance->frame_buf[2] = data;    // 存储帧类型
    protocol_instance->parse_state = UART_protocol_WaitingLength;    // 转到等待帧长度的状态
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

    protocol_instance->frame_buf[3] = data;    // 存储帧长度
    protocol_instance->payload_index = UART_PROTOCOL_PAYLOAD_OFFSET;    // 数据载荷从frame_buffer[4]开始存储
    protocol_instance->parse_state =
        (protocol_instance->data_len == 0) ? UART_protocol_WaitingChecksum1 : UART_protocol_ReceivingPayload;
    return true;
}

static bool Uart_Protocol_Analysis_ReceivingPayload(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buf[protocol_instance->payload_index++] = data;    // 存储数据载荷
    if (protocol_instance->payload_index >= protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET)
    {
        protocol_instance->parse_state = UART_protocol_WaitingChecksum1;    // 转到等待校验高字节的状态
    }
    return true;
}

static bool Uart_Protocol_Analysis_WaitingChecksum1(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buf[protocol_instance->payload_index++] = data;    // 存储校验高字节
    protocol_instance->parse_state = UART_protocol_WaitingChecksum2;    // 转到等待校验低字节的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingChecksum2(UART_protocol_t* protocol_instance, uint8_t data)
{
    protocol_instance->frame_buf[protocol_instance->payload_index++] = data;    // 存储校验低字节
    protocol_instance->parse_state = UART_protocol_WaitingTail1;    // 转到等待帧尾一的状态
    return true;
}

static bool Uart_Protocol_Analysis_WaitingTail1(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->frame_cfg.Tailframe1 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Tailframe1. Resetting to wait for Headerframe1.", data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    protocol_instance->frame_buf[protocol_instance->payload_index++] = data;    // 存储帧尾一
    protocol_instance->parse_state = UART_protocol_WaitingTail2;    // 转到等待帧尾二的状态
    return true;
}

static bool Uart_Protocol_Analysis_DealFrame(UART_protocol_t* protocol_instance)
{
    const uint8_t frame_type = protocol_instance->frame_buf[2];

    if (!protocol_instance)
    {
        LOG_FATAL("Protocol instance pointer is NULL. Cannot process frame.");
        return false;
    }

    if (frame_type == UART_PROTOCOL_ACK_TYPE)
    {
        if ((protocol_instance->flags & isWatingForAck) &&
            (protocol_instance->flags & isEnableAck))
        {
            __Uart_Protocol_StopWaitingForAck(protocol_instance);
            LOG_DEBUG("Received ACK frame. Exiting waiting for ACK state.");
        }
        else
        {
            LOG_DEBUG("Received ACK frame while not waiting for ACK. Ignoring.");
        }
        return true;    // ACK帧由协议层消费，不进入用户回调
    }

    if ((protocol_instance->flags & isEnableAck) && !Uart_Protocol_TransmitAck(protocol_instance))
    {
        LOG_WARN("Failed to transmit ACK frame for type 0x%02X.", frame_type);
    }

    if (protocol_instance->on_frame)
    {
        protocol_instance->on_frame(
            protocol_instance->parsing_frame_type,
            &protocol_instance->frame_buf[UART_PROTOCOL_PAYLOAD_OFFSET],
            protocol_instance->data_len);
    }
    return true;
}

static bool Uart_Protocol_Analysis_WaitingTail2(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (protocol_instance->frame_cfg.Tailframe2 != data)
    {
        LOG_DEBUG("Receive exception byte: 0x%02X while waiting for Tailframe2. Resetting to wait for Headerframe1.", data);
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    protocol_instance->frame_buf[protocol_instance->payload_index++] = data;    // 存储帧尾二

    {
        const uint16_t received_checksum = ((uint16_t)protocol_instance->frame_buf[protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET] << 8) |
            (uint16_t)protocol_instance->frame_buf[protocol_instance->data_len + UART_PROTOCOL_PAYLOAD_OFFSET + 1];
        const uint16_t calculated_checksum = calculateChecksum(&protocol_instance->frame_buf[2], protocol_instance->data_len + 2);

        if (received_checksum == calculated_checksum )
        {
            Uart_Protocol_Analysis_DealFrame(protocol_instance);
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
    bool result = false;
    if (!__Uart_Protocol_ReadBitFlag(protocol_instance,isCustomQueue))
    {
        result = UART_PROTOCOL_QUEUE_POP(&protocol_instance->rx_queue, &data);
    }
    else
    {
        void* queue_instance = protocol_instance->queue.instance;
        result = protocol_instance->queue.pop(queue_instance,&data);
    }    
    if (!result)
    {
        return true;  // 队列中没有数据可供处理，正常返回
    }
    if (protocol_instance->parse_state > UART_protocol_WaitingTail2)
    {
        LOG_WARN("Unknown frame processing state. Resetting to wait for Headerframe1.");
        __Uart_Protocol_ResetReceiveState(protocol_instance);
        return false;
    }

    return Uart_Protocol_Analysis_hander_state[protocol_instance->parse_state](protocol_instance, data);
}

/* ========================================================================
 *  Section 4 — 公开 API（初始化 & 字节接收）
 * ======================================================================== */

bool Uart_Protocol_Init(UART_protocol_t* protocol_instance,Uart_Protocol_FunctionsParameters RequiredParam)
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
    protocol_instance->parse_state = (uint8_t)UART_protocol_WaitingHeader1;                                       // 初始化状态机状态    
    protocol_instance->frame_cfg = RequiredParam.Head_Tial_Frame_struct;
    memset(protocol_instance->frame_buf, 0, sizeof(protocol_instance->frame_buf));       // 清空整帧接收缓冲区

    protocol_instance->parsing_frame_type = 0;                                                    // 初始化当前帧类型
    protocol_instance->on_frame = RequiredParam.frame_received_handler;             // 设置数据帧接收处理函数

    protocol_instance->tx = RequiredParam.transmit_function;
    protocol_instance->queue = (Queue_Operations){
        .instance = NULL,
        .pop = NULL,
        .push = NULL
    };                                                   // 设置串口数据队列操作函数集合
    UART_PROTOCOL_QUEUE_INIT(&protocol_instance->rx_queue);    // 初始化串口数据队列

    __Uart_Protocol_WriteBitFlag(protocol_instance,isCustomQueue,false);    // 默认不使用用户自定义队列
    protocol_instance->flags = (uint32_t)isNone;
    protocol_instance->flags &= (uint32_t)~isWatingForAck;    // 初始化时不等待ACK
    protocol_instance->flags |= (uint32_t)isInitialized;
    return true;
}

bool Uart_Protocol_Register_ACK(UART_protocol_t* protocol_instance,Uart_Protocol_ACKFunctionsParameters ACKParam)
{
    if (__Uart_Protocol_Check_ACKParameters(ACKParam))
    {
        protocol_instance->ack.pending_type = 0;
        protocol_instance->ack.last_tick = 0;
        protocol_instance->ack.timeout_ms = DEFAULT_TIMEOUT_THRESHOLD_MS;
        protocol_instance->ack.retry_n = 0;
        protocol_instance->ack.retry_max = DEFALUT_TIMEOUT_MAX_RETRY_TIMES;
        protocol_instance->ack.get_tick = ACKParam.GetTick;
        protocol_instance->ack.on_timeout = ACKParam.timeout_handler;
        __Uart_Protocol_WriteBitFlag(protocol_instance,isEnableAck,true);
        return true;
    }
    else
    {
        __Uart_Protocol_WriteBitFlag(protocol_instance,isEnableAck,false);
        return false;
    }
    return false;
}

bool Uart_Protocol_Register_Parse_WatchDog(UART_protocol_t* protocol_instance,uint32_t (*get_tick)(void),const uint32_t time_out)
{
    if (!get_tick)
    {
        LOG_INFO("GetTick function pointer is NULL. Parse watchdog will be disabled.");
        __Uart_Protocol_WriteBitFlag(protocol_instance,isEnableParseWatchDog,false);
        return false;
    }
    if (time_out <= 0)
    {
        LOG_INFO("Time out is invaild!!! Set hardfault value: %u ms for parse watchdog.", DEFAULT_TIMEOUT_THRESHOLD_MS);
        __Uart_Protocol_WriteBitFlag(protocol_instance,isEnableParseWatchDog,false);
        return false;
    }
    protocol_instance->parse_watchdog.get_tick = get_tick;
    protocol_instance->parse_watchdog.timeout_ms = time_out;
    protocol_instance->parse_watchdog.last_tick = get_tick();
    protocol_instance->parse_watchdog.last_frame_type = __Uart_Protocol_ReadCurrentState(protocol_instance);
    __Uart_Protocol_WriteBitFlag(protocol_instance,isEnableParseWatchDog,true);
    return true;
}

bool Uart_Protocol_Register_CustomQueue(UART_protocol_t* protocol_instance, Queue_Operations queue_ops)
{
    if (!queue_ops.instance || !queue_ops.push || !queue_ops.pop)
    {
        LOG_INFO("Custom queue parameters are incomplete. Custom queue will not be enabled.");
        return false;
    }
    protocol_instance->queue = queue_ops;
    __Uart_Protocol_WriteBitFlag(protocol_instance, isCustomQueue, true);
    return true;
}

bool Uart_Protocol_ProcessReceivedData8bit(UART_protocol_t* protocol_instance, uint8_t data)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    if (!__Uart_Protocol_ReadBitFlag(protocol_instance,isCustomQueue))
    {
        return UART_PROTOCOL_QUEUE_PUSH(&protocol_instance->rx_queue, data);
    }
    return protocol_instance->queue.push(protocol_instance->queue.instance, data);
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

/* ========================================================================
 *  Section 5 — 内部：ACK 等待 / 组帧 / 发送 / 超时重传
 *
 *  调用链: TransmitData → BeginWaitingForAck + Backup → 用户发送函数
 *         Loop → ElapsedTimeExceeded → Retry → TransmitRawData → 用户发送函数
 * ======================================================================== */

static bool __Uart_Protocol_Backup_LastTransmitData(UART_protocol_t* protocol_instance, const uint8_t* data, size_t len)
{
    if ((data == NULL) && len > 0)
    {
        LOG_FATAL("Data pointer is NULL while length is %zu. Cannot backup data.", len);
        return false;
    }
    if (len > UART_PROTOCOL_FRAME_BUFFER_LEN)
    {
        LOG_FATAL("Data length %zu exceeds backup buffer size. Cannot backup data.", len);
        return false;
    }
    memcpy(protocol_instance->ack.backup, data, len);
    protocol_instance->ack.backup_len = len;
    return true;
}

static void __Uart_Protocol_BeginWaitingForAck(UART_protocol_t* protocol_instance)
{
    if ((protocol_instance->flags & isEnableAck) == 0)
    {
        return;
    }

    __Uart_Protocol_WriteBitFlag(protocol_instance, isWatingForAck, true);
    protocol_instance->ack.retry_n = 0;
    if (protocol_instance->ack.get_tick)
    {
        protocol_instance->ack.last_tick = protocol_instance->ack.get_tick();
    }
}

static void __Uart_Protocol_StopWaitingForAck(UART_protocol_t* protocol_instance)
{
    __Uart_Protocol_WriteBitFlag(protocol_instance, isWatingForAck, false);
    protocol_instance->ack.retry_n = 0;
    protocol_instance->ack.backup_len = 0;
    protocol_instance->ack.pending_type = 0;
    memset(protocol_instance->ack.backup, 0, sizeof(protocol_instance->ack.backup));
}

static size_t __Uart_Protocol_Framing(Custom_Frame_HT_T HT,uint8_t* buffer,size_t buffer_size,uint8_t type,const uint8_t* data ,uint8_t payload_size)        
{
    if ((buffer == NULL) || (buffer_size < UART_PROTOCOL_FRAME_OVERHEAD + payload_size))
    {
        LOG_FATAL("Buffer is NULL or too small. Required size: %zu. Cannot frame data.", (size_t)(UART_PROTOCOL_FRAME_OVERHEAD + payload_size));
        return 0;
    }
    if ((data == NULL) && (payload_size > 0))
    {
        LOG_FATAL("Data pointer is NULL while payload size is %u. Cannot frame data.", payload_size);
        return 0;
    }
    buffer[0] = HT.Headerframe1;
    buffer[1] = HT.Headerframe2;
    buffer[2] = type;    // 帧类型
    buffer[3] = payload_size;    // 帧长度
    if (payload_size > 0)
    {
        memcpy(&buffer[UART_PROTOCOL_PAYLOAD_OFFSET], data, payload_size);    // 数据载荷
    }
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size] = (calculateChecksum(&buffer[2], payload_size + 2) >> 8) & 0xFF;    // 校验高字节
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size + 1] = calculateChecksum(&buffer[2], payload_size + 2) & 0xFF;    // 校验低字节
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size + 2] = HT.Tailframe1;    // 帧尾一
    buffer[UART_PROTOCOL_PAYLOAD_OFFSET + payload_size + 3] = HT.Tailframe2;    // 帧尾二
    return UART_PROTOCOL_FRAME_OVERHEAD + payload_size;
}

static bool __Uart_Protocol_AckFraming(Custom_Frame_HT_T HT,uint8_t* buffer,size_t buffer_size,uint8_t type,const uint8_t* data ,uint8_t payload_size)
{
    return __Uart_Protocol_Framing(HT, buffer, buffer_size, UART_PROTOCOL_ACK_TYPE, NULL, 0) > 0;
}

static bool __Uart_Protocol_TransmitRawData(UART_protocol_t* protocol_instance, const uint8_t* data, uint16_t len)
{
    if (!__Uart_Protocol_IsInitialized(protocol_instance))
    {
        LOG_FATAL("Protocol instance is not initialized. Call Uart_Protocol_Init first.");
        return false;
    }
    if ((data == NULL) && (len > 0))
    {
        LOG_FATAL("Data pointer is NULL while length is %u. Cannot transmit data.", len);
        return false;
    }
    if (!protocol_instance->tx)
    {
        LOG_FATAL("Transmit function pointer is NULL. Cannot transmit data.");
        return false;
    }
    return protocol_instance->tx(data, len);
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
    if (!protocol_instance->tx)
    {
        LOG_FATAL("Transmit function pointer is NULL. Cannot transmit data.");
        return false;
    }
    if (len > UART_PROTOCOL_MAX_PAYLOAD_SIZE)
    {
        LOG_WARN("Data length %u exceeds maximum payload size. Cannot transmit data.", len);
        return false;
    }
    uint8_t frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    bool success = false;
    if (type == UART_PROTOCOL_ACK_TYPE)
    {
        success = __Uart_Protocol_AckFraming(protocol_instance->frame_cfg, frame, sizeof(frame), type, data, len);
    }
    else
    {
        success = __Uart_Protocol_Framing(protocol_instance->frame_cfg, frame, sizeof(frame), type, data, len) > 0;
    }
    if (!success)
    {
        LOG_FATAL("Failed to frame data for transmission. Type: 0x%02X, Length: %u.", type, len);
        if (type != UART_PROTOCOL_ACK_TYPE)
        {
            __Uart_Protocol_StopWaitingForAck(protocol_instance);
        }
        return false;   
    }

    success = protocol_instance->tx(frame, UART_PROTOCOL_FRAME_OVERHEAD + len);

    if (!success)
    {
        LOG_FATAL("Failed to transmit frame. Type: 0x%02X, Length: %u.", type, len);
        if (type != UART_PROTOCOL_ACK_TYPE)
        {
            __Uart_Protocol_StopWaitingForAck(protocol_instance);
        }
        return false;
    }

    if (type != UART_PROTOCOL_ACK_TYPE)
    {
        protocol_instance->ack.pending_type = type;
        __Uart_Protocol_BeginWaitingForAck(protocol_instance);
        (void)__Uart_Protocol_Backup_LastTransmitData(protocol_instance, frame, UART_PROTOCOL_FRAME_OVERHEAD + len);
    }

    return success;
}

static bool __Uart_Protocol_Retry(UART_protocol_t* protocol_instance)
{
    if (protocol_instance->ack.retry_n < protocol_instance->ack.retry_max)
    {
        protocol_instance->ack.retry_n++;
        return __Uart_Protocol_TransmitRawData(protocol_instance, protocol_instance->ack.backup, protocol_instance->ack.backup_len);    // 重传上次发送的数据，假设是ACK帧
    }
    else
    {
        const uint8_t pending_frame_type = protocol_instance->ack.pending_type;
        __Uart_Protocol_StopWaitingForAck(protocol_instance);
        LOG_WARN("Exceeded maximum retry attempts for ACK. Exiting waiting for ACK state.");
        if (protocol_instance->ack.on_timeout)
        {
            protocol_instance->ack.on_timeout(pending_frame_type);    // 调用用户定义的超时处理函数
        }
        return false;
    }
}

static void __Uart_Protocol_parseWatchDog(UART_protocol_t* protocol_instance)
{
    if (!__Uart_Protocol_ReadBitFlag(protocol_instance, isEnableParseWatchDog))
    {
        LOG_DEBUG("Parse watchdog is disabled. Skipping parse watchdog check.");
        return;
    }
    Frame_Process_Type current_state = __Uart_Protocol_ReadCurrentState(protocol_instance);
    if (current_state == UART_protocol_WaitingHeader1)
    {
        protocol_instance->parse_watchdog.last_tick = protocol_instance->parse_watchdog.get_tick();
        return;// 等待新帧,不检查阶段超时
    }
    else if(current_state != protocol_instance->parse_watchdog.last_frame_type)
    {
        protocol_instance->parse_watchdog.last_frame_type = current_state;
        protocol_instance->parse_watchdog.last_tick = protocol_instance->parse_watchdog.get_tick();
        return;// 状态变更,重置计时
    }
    else
    {
        uint32_t current_tick = protocol_instance->parse_watchdog.get_tick();
        uint32_t elapsed = current_tick - protocol_instance->parse_watchdog.last_tick;
        if (elapsed > protocol_instance->parse_watchdog.timeout_ms)
        {
            LOG_DEBUG("Parse stage %d exceeded timeout threshold. Resetting to wait for Headerframe1.", current_state);
            __Uart_Protocol_ResetReceiveState(protocol_instance);
            protocol_instance->parse_watchdog.last_tick = current_tick;    // 更新上次记录的tick
        }
    }
}

static bool __Uart_Protocol_ElapsedTimeExceeded(UART_protocol_t* protocol_instance)
{
    if ((protocol_instance->flags & isEnableAck) == 0 || (protocol_instance->flags & isWatingForAck) == 0 )
    {
        return false;   // ACK机制未启用或当前不等待ACK，无需检查时间
    }
    if (!protocol_instance->ack.get_tick)
    {
        LOG_FATAL("GetTick function pointer is NULL. Cannot check elapsed time.");
        return false;
    }
    uint32_t current_tick = protocol_instance->ack.get_tick();
    uint32_t elapsed = current_tick - protocol_instance->ack.last_tick;
    if (elapsed > protocol_instance->ack.timeout_ms)
    {
        protocol_instance->ack.last_tick = current_tick;    // 更新上次记录的tick
        LOG_DEBUG("Elapsed time %u ms exceeded threshold. Attempting to retry transmission. Try times: %u.", elapsed, protocol_instance->ack.retry_n);   
        return __Uart_Protocol_Retry(protocol_instance);
    }
    return false;
}


/* ========================================================================
 *  Section 6 — 公开 API（发送帧 & 主循环）
 * ======================================================================== */

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
    if (!protocol_instance->tx)
    {
        LOG_FATAL("Transmit function pointer is NULL. Cannot transmit data.");
        return false;
    }
    if (len > UART_PROTOCOL_MAX_PAYLOAD_SIZE)
    {
        LOG_WARN("Data length %u exceeds maximum payload size. Cannot transmit data.", len);
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
    __Uart_Protocol_parseWatchDog(protocol_instance);
    (void)__Uart_Protocol_ElapsedTimeExceeded(protocol_instance);
    return Uart_Protocol_Analysis(protocol_instance);
}


