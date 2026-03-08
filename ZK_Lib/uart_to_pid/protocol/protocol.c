#include "protocol.h"



typedef struct {
    CmdType cmd_type;
    uint8_t pram_length;
    FrameHandler handler;
}CmdHandlerEntry;

typedef struct{
    UART_protocol G_UART_protocol_structure;
    FrameTransmit transmit_function;
    CmdHandlerEntry CmdHandlerTable[MAX_CMD_TYPE_HANDLERS];
}G_Protocol_Context;

// 静态环形缓冲区
static UartFrame Receive_Uart_Frame_Buffer = {0};
static G_Protocol_Context g_protocol_context = {0};

// 回调函数实例 + 初始化

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

void UART_Protocol_Init(UART_protocol UART_protocol_structure,FrameTransmit transmit_function)
{
    if (!transmit_function)
    {
        return;
    }
    g_protocol_context.G_UART_protocol_structure = UART_protocol_structure;
    g_protocol_context.transmit_function = transmit_function;
}


void UART_Protocol_Register_Hander(CmdType cmd_type, FrameHandler handler,uint8_t pram_length)
{
    if ((size_t)cmd_type >= MAX_CMD_TYPE_HANDLERS) {
        return;
    }
    if (pram_length >= MAX_PAYLOAD_LEN)
    {
        return;
    }
    g_protocol_context.CmdHandlerTable[cmd_type].pram_length = pram_length;
    g_protocol_context.CmdHandlerTable[cmd_type].cmd_type = cmd_type;
    g_protocol_context.CmdHandlerTable[cmd_type].handler = handler;
}


void UART_Protocol_Transmit(prama_Cmd_packet* cmd_packet)
{
    if (cmd_packet == NULL) return;
    if (!g_protocol_context.transmit_function) return;

    uint8_t len = g_protocol_context.CmdHandlerTable[cmd_packet->cmd_type].pram_length;
    uint8_t frame_buffer[MAX_PAYLOAD_LEN + 8];  // 至少 payload + 8
    uint16_t index = 0;

    // 帧头
    frame_buffer[index++] = g_protocol_context.G_UART_protocol_structure.Headerframe1;
    frame_buffer[index++] = g_protocol_context.G_UART_protocol_structure.Headerframe2;

    // 类型与长度
    frame_buffer[index++] = (uint8_t)cmd_packet->cmd_type;
    frame_buffer[index++] = len;

    // 载荷（发送侧应直接拷贝，不调用接收处理器）
    if (len) 
    {
        memcpy(&frame_buffer[index], cmd_packet->param_value, len);
        index += len;
    }

    // 校验（覆盖 CMD+LEN+PAYLOAD）
    uint16_t check = calculateChecksum(&frame_buffer[2], 2 + len);
    frame_buffer[index++] = (check >> 8) & 0xff;
    frame_buffer[index++] = check & 0xff;

    // 帧尾
    frame_buffer[index++] = g_protocol_context.G_UART_protocol_structure.Tailframe1;
    frame_buffer[index++] = g_protocol_context.G_UART_protocol_structure.Tailframe2;

    // 发送
    g_protocol_context.transmit_function(frame_buffer, index);
}


/**
 * @brief    环形缓冲区初始化
 * @param    rb        环形缓冲区指针
 */
static void RingBuffer_Init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/**
 * @brief    向环形缓冲区放置数据
 * @param    rb        环形缓冲区指针
 * @param    data      一个字节的二进制数据
 */
static void RingBuffer_Put(RingBuffer *rb, uint8_t data)
{
    uint16_t next_head = (rb->head + 1) % sizeof(rb->buffer);   // 指向下一个区域的指针下标
    if (next_head != rb->tail)                                  // 检查缓冲区是否已满
    {
        rb->buffer[rb->head] = data;                            // 放置数据
        rb->head = next_head;                                   // 更改指针下标·
    }
}

/**
 * @brief    从环形缓冲区获取一字节二进制数据
 * @param    rb        环形缓冲区指针
 * @param    data      接受数据的指针
 * @return   true      获取成功
 * @return   false     缓冲区为空，无法获取
 */
static bool RingBuffer_Get(RingBuffer *rb, uint8_t *data)
{
    if (rb->head == rb->tail) // 检查缓冲区是否为空
    {
        return false;// 缓冲区为空
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % sizeof(rb->buffer);
    return true; // 成功读取一个字节
}

/**
 * @brief    获取环形缓冲期指针
 * @return   UartFrame* 环形缓冲区指针
 */
UartFrame* Get_Uart_Frame_Buffer(void)
{
    UartFrame* frame = &Receive_Uart_Frame_Buffer;
    frame->Size = 0; 
    RingBuffer_Init(&frame->ring_buffer);
    return &Receive_Uart_Frame_Buffer;
}

/**
 * @brief    放置指定字节长度的二进制数据
 * @cite     RingBuffer_Put()
 * @param    frame     环形缓冲区的指针
 * @param    data      放置数据的缓冲区数组
 * @param    size      指定大小
 */
void Uart_Buffer_Put_frame(UartFrame* frame,uint8_t* data,uint8_t size)
{
    frame->Size = size;
    for(uint16_t i = 0; i < size; i++)
    {
        RingBuffer_Put(&frame->ring_buffer,data[i]);
    }
}

/**
 * @brief    获取目前缓冲区存放的数据
 * @warning  注意缓冲区大小问题，请确保size大小的数据能完美的放在data数组内
 * @param    frame     环形缓冲区的指针
 * @param    data      用于接受数据的缓冲区指针
 */
void Uart_Buffer_Get_frame(UartFrame* frame,uint8_t* data)
{
    uint8_t temp;
    uint8_t count = 0;
    while(RingBuffer_Get(&frame->ring_buffer,&temp) && count < frame->Size)
    {
        data[count++] = temp;
    }
}



/**
 * @brief    处理数据帧函数
 * @details  处理规定帧头帧尾的数据帧
 * @param    UART_protocol_structure    帧头帧尾结构体数据
 * @param    data      存储数据帧的缓冲区指针
 * @param    size      数据帧大小
 */
void Receive_Uart_Frame(UART_protocol UART_protocol_structure, uint8_t* data,uint16_t size)
{
    // 最小帧长度检查
    if(size < 8)
    {
        LOG_DEBUG("Frame too short, size=%d", size);
    }
    // 检查头
    if(data[0] != UART_protocol_structure.Headerframe1 ||
       data[1] != UART_protocol_structure.Headerframe2)
    {
        LOG_DEBUG("Header mismatch: 0x%02X 0x%02X", data[0], data[1]);
        return;
    }
    LOG_DEBUG("Header matched.");
    uint8_t frame_type = data[2];
    uint8_t frame_len  = data[3];

    // 检查长度是否合理
    if(size != (frame_len + 8) || frame_len > MAX_PAYLOAD_LEN)
    {
        LOG_DEBUG("Length mismatch: expected %d, got %d", frame_len + 8, size);
        return;
    } 
    LOG_DEBUG("length matched.");
    // 数据区
    uint8_t *payload = &data[4];

    // 校验
    uint16_t recv_check = (data[frame_len + 4] << 8) | data[frame_len + 5];
    uint16_t calc_check = calculateChecksum(&data[2], 2 + frame_len);
    if(recv_check != calc_check)
    {
        LOG_DEBUG("Checksum error: expected 0x%04X, got 0x%04X", calc_check, recv_check);
        return;
    } 
    LOG_DEBUG("checksum matched.");
    // 检查尾
    if(data[frame_len + 6] != UART_protocol_structure.Tailframe1 ||
       data[frame_len + 7] != UART_protocol_structure.Tailframe2)
    {
        LOG_DEBUG("Tail mismatch: 0x%02X 0x%02X", data[frame_len + 6], data[frame_len + 7]);
        return;
    }
    LOG_DEBUG("tail matched.");
    LOG_DEBUG("the frame is valid: type=%d, len=%d", frame_type, frame_len);
    // 解析数据
    if(frame_type == g_protocol_context.CmdHandlerTable[frame_type].cmd_type && 
       frame_len == g_protocol_context.CmdHandlerTable[frame_type].pram_length)
    {
        g_protocol_context.CmdHandlerTable[frame_type].handler(payload);
        return;
    }
    else
    {
        LOG_DEBUG("Unknown frame type: %d or invalid length: %d", frame_type, frame_len);
    }
}

