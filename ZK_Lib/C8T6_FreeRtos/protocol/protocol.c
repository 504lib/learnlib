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

// static CmdHandlerEntry CmdHandlerTable[MAX_CMD_TYPE_HANDLERS] = {0};


// static UART_protocol G_UART_protocol_structure = {
//     .Headerframe1 = 0xAA,
//     .Headerframe2 = 0x55,
//     .Tailframe1 = 0x0D,
//     .Tailframe2 = 0x0A
// };

// static 

static G_Protocol_Context g_protocol_context = {0};


typedef struct
{
    INT_Callback int_callback;                      // 整形数据接受的回调函数
    FLOAT_Callback float_callback;                  // 浮点型数据接受的回调函数
    ACK_Callback ack_callback;                      // ack信号接受的回调函数
    PASSENGER_NUM_Callback passenger_callback;      // 乘客数量接受的回调函数
    CLEAR_Callback clear_callback;                  // 清除乘客数量的回调函数
    VehicleStatusCallback vehiclestatus_callback;  // 车辆状态接受的回调函数
}callback_functions;

// 回调函数实例 + 初始化
callback_functions callbacks = {
    .ack_callback = NULL,           
    .float_callback = NULL,
    .int_callback = NULL,
    .passenger_callback = NULL,
    .clear_callback = NULL
};

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
 * @brief   大端序（网络序）辅助函数
 * @note    协议在线路上统一采用大端序，MCU 本机为小端。
 *          使用这些函数可保持收发一致、移植性更好。
 */
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
 * @brief    处理INT数据
 * @details  通过Receive_Uart_Frame()函数,帧解包为INT类型对数据进行处理的函数
 * @if       未设置INT回调函数，执行echo行为
 * @else     执行用户定义行为
 * @param    value     为帧解包获得的值
 */
static void handle_INT(int32_t value)
{
    if (callbacks.int_callback) 
    {
        callbacks.int_callback(value);
        return;
    }
    else
    {
        UART_protocol UART_protocol_structure = {
        .Headerframe1 = 0xAA,
        .Headerframe2 = 0x55,
        .Tailframe1 = 0x0D,
        .Tailframe2 = 0x0A
        };
        UART_Protocol_INT(UART_protocol_structure,value);
    }
}


/**
 * @brief    处理INT数据
 * @details  通过Receive_Uart_Frame()函数,帧解包为FLOAT类型对数据进行处理的函数
 * @if       未设置FLOAT回调函数，执行echo行为
 * @else     执行用户定义行为
 * @param    value     为帧解包获得的值
 */
static void handle_FLOAT(float value)
{
    LOG_INFO("handle_FLOAT called with value: %f", value);
    if (callbacks.float_callback) 
    {
        callbacks.float_callback(value);
        return;
    }
    else
    {
        UART_protocol UART_protocol_structure = {
        .Headerframe1 = 0xAA,
        .Headerframe2 = 0x55,
        .Tailframe1 = 0x0D,
        .Tailframe2 = 0x0A
        };
        UART_Protocol_FLOAT(UART_protocol_structure,value);
    }
}


/**
 * @brief    处理ACK信号
 * @details  通过Receive_Uart_Frame()函数,帧解包为ACK类型对信号进行处理的函数
 * @if       未设置ACK回调函数，执行echo行为
 * @else     执行用户定义行为
 */
static void handle_ACK(void)
{
    if (callbacks.ack_callback) 
    {
        callbacks.ack_callback();
        return;
    }
    else
    {
        UART_protocol UART_protocol_structure = {
        .Headerframe1 = 0xAA,
        .Headerframe2 = 0x55,
        .Tailframe1 = 0x0D,
        .Tailframe2 = 0x0A
        };
        UART_Protocol_ACK(UART_protocol_structure);
    }
}


/**
 * @brief    处理PASSENGES数据
 * @details  通过Receive_Uart_Frame()函数,帧解包为PASSENGES类型对数据进行处理的函数
 * @if       未设置PASSENGES回调函数，执行echo行为
 * @else     执行用户定义行为
 * @param    value     为帧解包获得的值
 */
static void handle_PASSENGER(Rounter rounter,uint8_t value)
{
    if (callbacks.passenger_callback) 
    {
        callbacks.passenger_callback(rounter,value);
        return;
    }
    else
    {
        UART_protocol UART_protocol_structure = {
        .Headerframe1 = 0xAA,
        .Headerframe2 = 0x55,
        .Tailframe1 = 0x0D,
        .Tailframe2 = 0x0A
        };
        UART_Protocol_Passenger(UART_protocol_structure,rounter,value);
    }
}


/**
 * @brief    处理CLEAR信号
 * @details  通过Receive_Uart_Frame()函数,帧解包为CLEAR类型对数据进行处理的函数
 * @if       未设置CLEAR回调函数，执行echo行为
 * @else     执行用户定义行为
 * @param    value     为帧解包获得的值
 */
static void handle_CLEAR(Rounter rounter)
{
    if (callbacks.clear_callback) 
    {
        callbacks.clear_callback(rounter);
        return;
    }
    else
    {
        UART_protocol UART_protocol_structure = {
        .Headerframe1 = 0xAA,
        .Headerframe2 = 0x55,
        .Tailframe1 = 0x0D,
        .Tailframe2 = 0x0A
        };
        UART_Protocol_Clear(UART_protocol_structure,rounter);
    }
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
 * @brief    Set the INT Callback object:用于设置INT数据处理的回调函数
 * @param    cb        INT回调函数
 */
void set_INT_Callback(INT_Callback cb)
{
    callbacks.int_callback = cb;
}

/**
 * @brief    Set the FLOAT Callback object:用于设置FLOAT数据处理的回调函数
 * @param    cb        FLOAT回调函数
 */
void set_FLOAT_Callback(FLOAT_Callback cb)
{
    callbacks.float_callback = cb;
}

/**
 * @brief    Set the ACK Callback object:用于设置ACK信号处理的回调函数
 * @param    cb        ACK回调函数
 */
void set_ACK_Callback(ACK_Callback cb)
{
    callbacks.ack_callback = cb;
}

/**
 * @brief    Set the PASSENGER Callback object:用于设置PASSENGER数据处理的回调函数
 * @param    cb        PASSENGER回调函数
 */
void set_PASSENGER_Callback(PASSENGER_NUM_Callback cb)
{
    callbacks.passenger_callback = cb;
}

/**
 * @brief    Set the Clear Callback object:用于设置CLEAR数据处理的回调函数
 * @param    cb        CLEAR回调函数
 */
void set_Clear_Callback(CLEAR_Callback cb)
{
    callbacks.clear_callback = cb;
}

void set_VehicleStatus_Callback(VehicleStatusCallback cb)
{
    callbacks.vehiclestatus_callback = cb;
}

/**
 * @brief    发送整形数据
 * @cite     LOG_DEBUG();calculateChecksum();
 * @param    UART_protocol_structure 帧头帧尾结构体数据
 * @param    value     四字节的整形值
 */
void UART_Protocol_INT(UART_protocol UART_protocol_structure,int32_t value)
{
    typedef union
    {
        int value;
        uint8_t value_arr[4];
    }dataunion;

    LOG_DEBUG("untion object has been built ...");

    dataunion data;
    data.value = value;
    uint8_t frame[12];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = INT;
    frame[3] = 4;

    LOG_DEBUG("type has been placed ... , type = INT");
    frame[4] = data.value_arr[3];
    frame[5] = data.value_arr[2];
    frame[6] = data.value_arr[1];
    frame[7] = data.value_arr[0];

    LOG_DEBUG("data has been placed ...");
    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[10] = UART_protocol_structure.Tailframe1;
    frame[11] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_DEBUG("INT frame has sent,checksum = 0x%04x",check);
}

/**
 * @brief    发送浮点型数据
 * @cite     LOG_DEBUG();calculateChecksum();
 * @param    UART_protocol_structure 帧头帧尾结构体数据
 * @param    value     四字节的浮点数值
 */
void UART_Protocol_FLOAT(UART_protocol UART_protocol_structure,float value)
{
    typedef union
    {
        float value;
        uint8_t value_arr[4];
    }dataunion;

    LOG_DEBUG("untion object has been built ...");
    dataunion data;
    data.value = value;
    uint8_t frame[12];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = FLOAT;
    frame[3] = 4;

    LOG_DEBUG("type has been placed ... , type = FLOAT");
    frame[4] = data.value_arr[3];
    frame[5] = data.value_arr[2];
    frame[6] = data.value_arr[1];
    frame[7] = data.value_arr[0];

    LOG_DEBUG("data has been placed ...");
    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[10] = UART_protocol_structure.Tailframe1;
    frame[11] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_DEBUG("FLOAT frame has sent,checksum = 0x%04x",check);
}

/**
 * @brief    发送ACK信号
 * @cite     LOG_DEBUG();calculateChecksum();
 * @param    UART_protocol_structure 帧头帧尾结构体数据
 */
void UART_Protocol_ACK(UART_protocol UART_protocol_structure)
{
    uint8_t frame[8];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = ACK;
    frame[3] = 0;

    LOG_DEBUG("type has been placed ... , type = ACK");
    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[6] = UART_protocol_structure.Tailframe1;
    frame[7] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_DEBUG("ACK frame has sent,checksum = 0x%04x",check);
}


/**
 * @brief    发送乘客数量
 * @cite     LOG_DEBUG();calculateChecksum();
 * @param    UART_protocol_structure 帧头帧尾结构体数据
 * @param    value     一字节的数值
 */
void UART_Protocol_Passenger(UART_protocol UART_protocol_structure,Rounter router,uint8_t value)
{

    uint8_t frame[10];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = PASSENGER_NUM;
    frame[3] = 2;

    LOG_DEBUG("Type has been placed ... , type = PASSENGER_NUM",router);
    frame[4] = (uint8_t)router;

    LOG_DEBUG("Rounter has been placed ... , type = %d",router);
    frame[5] = value;

    LOG_DEBUG("data has been placed ...");
    uint16_t check = calculateChecksum(&frame[2],4);
    frame[6] = (check >> 8) & 0xff;
    frame[7] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[8] = UART_protocol_structure.Tailframe1;
    frame[9] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_DEBUG("PASSENGER frame has sent,checksum = 0x%04x",check);
}

/**
 * @brief    发送清除顾客数量信号
 * @warning  当前还只是车站自清理，不是http端信号！！！
 * @param    UART_protocol_structure    帧头帧尾结构体数据
 */
void UART_Protocol_Clear(UART_protocol UART_protocol_structure,Rounter router)
{
    uint8_t frame[9];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = CLEAR;
    frame[3] = 1;

    LOG_DEBUG("type has been placed ... , type = CLEAR");

    frame[4] = (uint8_t)router;
    LOG_DEBUG("Rounter has been placed ... , type = %d",router);

    uint16_t check = calculateChecksum(&frame[2],3);
    frame[5] = (check >> 8) & 0xff;
    frame[6] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[7] = UART_protocol_structure.Tailframe1;
    frame[8] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_DEBUG("CLEAR frame has sent,checksum = 0x%04x",check);
}

/**
 * @brief    发送车辆状态信号
 * 
 * @param    UART_protocol_structurefunction of param
 * @param    status    function of param
 */
void UART_Protocol_VehicleStatus(UART_protocol UART_protocol_structure,VehicleStatus status)
{
    uint8_t frame[9];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = VEHICLE_STATUS;
    frame[3] = 1;

    LOG_DEBUG("type has been placed ... , type = VEHICLE_STATUS");

    frame[4] = (uint8_t)status;
    LOG_DEBUG("VehicleStatus has been placed ... , status = %d",status);

    uint16_t check = calculateChecksum(&frame[2],3);
    frame[5] = (check >> 8) & 0xff;
    frame[6] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[7] = UART_protocol_structure.Tailframe1;
    frame[8] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_DEBUG("VEHICLE_STATUS frame has sent,checksum = 0x%04x",check);
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
    // else if(frame_type == FLOAT && frame_len == 4)
    // {
    //     int32_t value = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    //     handle_INT(value);
    //     UART_Protocol_ACK(UART_protocol_structure);
    // }
    // else if(frame_type == FLOAT && frame_len == 4)
    // {
    //     union {
    //         float f;
    //         uint8_t b[4];
    //     } u;
    //     u.b[0] = payload[3];
    //     u.b[1] = payload[2];
    //     u.b[2] = payload[1];
    //     u.b[3] = payload[0];
    //     handle_FLOAT(u.f);
    //     UART_Protocol_ACK(UART_protocol_structure);
    // }
    // else if(frame_type == ACK && frame_len == 0)
    // {
    //     handle_ACK();
    // }
    // else if (frame_type == PASSENGER_NUM && frame_len == 2)
    // {
    //     Rounter router = (uint8_t)*payload;
    //     uint8_t passenger = *(payload + 1);
    //     handle_PASSENGER(router,passenger); 
    //     UART_Protocol_ACK(UART_protocol_structure);
    // }
    // else if (frame_type == CLEAR && frame_len == 1)
    // {
    //     Rounter router = (uint8_t)*payload;
    //     handle_CLEAR(router);
    //     UART_Protocol_ACK(UART_protocol_structure);
    // }
    // else if (frame_type == VEHICLE_STATUS && frame_len == 1)
    // {
    //     VehicleStatus status = (uint8_t)*payload;
    //     if(callbacks.vehiclestatus_callback)
    //     {
    //         callbacks.vehiclestatus_callback(status);
    //     }
    //     UART_Protocol_ACK(UART_protocol_structure);
    // }
    else
    {
        LOG_DEBUG("Unknown frame type: %d or invalid length: %d", frame_type, frame_len);
    }
}

