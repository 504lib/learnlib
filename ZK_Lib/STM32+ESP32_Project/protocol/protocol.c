#include "protocol.h"

// 静态环形缓冲区
static UartFrame Receive_Uart_Frame_Buffer = {0};

typedef struct
{
    INT_Callback int_callback;                      // 整形数据接受的回调函数
    FLOAT_Callback float_callback;                  // 浮点型数据接受的回调函数
    ACK_Callback ack_callback;                      // ack信号接受的回调函数
    PASSENGER_NUM_Callback passenger_callback;      // 乘客数量接受的回调函数
    CLEAR_Callback clear_callback;                  // 清除乘客数量的回调函数
    HX711_WEIGHT_Callback weight_callback;
    CURRENT_USER_Callback current_user_callback;
    MOTOR_READY_Callback motor_ready_callback;
    MOTOR_STOP_Callback motor_stop_callback;
    TARGET_WEIGHT_Callback target_weight_callback;
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
static void handle_PASSENGER(uint8_t value)
{
    if (callbacks.passenger_callback) 
    {
        callbacks.passenger_callback(value);
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
        UART_Protocol_Passenger(UART_protocol_structure,value);
    }
}


/**
 * @brief    处理CLEAR信号
 * @details  通过Receive_Uart_Frame()函数,帧解包为CLEAR类型对数据进行处理的函数
 * @if       未设置CLEAR回调函数，执行echo行为
 * @else     执行用户定义行为
 * @param    value     为帧解包获得的值
 */
static void handle_CLEAR()
{
    if (callbacks.clear_callback) 
    {
        callbacks.clear_callback();
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
        UART_Protocol_Clear(UART_protocol_structure);
    }
}


static void handle_WEIGHT(float weight)
{
    if (callbacks.weight_callback) 
    {
        callbacks.weight_callback(weight);
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
        UART_Protocol_WEIGHT(UART_protocol_structure,weight);
    }
}

static void handle_CURRENT_USER(char* name,Medicine medicine,uint8_t size)
{
    if (callbacks.current_user_callback) 
    {
        callbacks.current_user_callback(name,medicine,size);
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
        UART_Protocol_CURRENT_USER(UART_protocol_structure,name,medicine,size);
    }
}

static void handle_Motor_Stop(void)
{
    if (callbacks.motor_stop_callback) 
    {
        callbacks.motor_stop_callback();
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
        UART_Protocol_MOTOR_STOP(UART_protocol_structure);
    }
}

static void handle_Motor_Ready(void)
{
    if (callbacks.motor_ready_callback) 
    {
        callbacks.motor_ready_callback();
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
        UART_Protocol_MOTOR_READY(UART_protocol_structure);
    }
}


static void handle_Target_Weight(float value)
{
    if (callbacks.target_weight_callback) 
    {
        callbacks.target_weight_callback(value);
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
        UART_Protocol_TARGET_WEIGHT(UART_protocol_structure,value);
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

void set_Current_User(CURRENT_USER_Callback cb)
{
   callbacks.current_user_callback = cb; 
}

void set_Motor_Stop_Callback(MOTOR_STOP_Callback cb)
{
    callbacks.motor_stop_callback = cb;
}

void set_Motor_Ready_Callback(MOTOR_READY_Callback cb)
{
    callbacks.motor_ready_callback = cb;
}

void set_Target_Weight_Callback(TARGET_WEIGHT_Callback cb)
{
    callbacks.target_weight_callback = cb;
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
    LOG_INFO("INT frame has sent,checksum = 0x%04x",check);
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
    LOG_INFO("FLOAT frame has sent,checksum = 0x%04x",check);
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
    LOG_INFO("ACK frame has sent,checksum = 0x%04x",check);
}


/**
 * @brief    发送乘客数量
 * @cite     LOG_DEBUG();calculateChecksum();
 * @param    UART_protocol_structure 帧头帧尾结构体数据
 * @param    value     一字节的数值
 */
void UART_Protocol_Passenger(UART_protocol UART_protocol_structure,uint8_t value)
{

    uint8_t frame[9];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = PASSENGER_NUM;
    frame[3] = 1;

    LOG_DEBUG("type has been placed ... , type = PASSENGER_NUM");
    frame[4] = value;

    LOG_DEBUG("data has been placed ...");
    uint16_t check = calculateChecksum(&frame[2],3);
    frame[5] = (check >> 8) & 0xff;
    frame[6] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[7] = UART_protocol_structure.Tailframe1;
    frame[8] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_INFO("PASSENGER frame has sent,checksum = 0x%04x",check);
}

/**
 * @brief    发送清除顾客数量信号
 * @warning  当前还只是车站自清理，不是http端信号！！！
 * @param    UART_protocol_structure    帧头帧尾结构体数据
 */
void UART_Protocol_Clear(UART_protocol UART_protocol_structure)
{
    uint8_t frame[8];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = CLEAR;
    frame[3] = 0;

    LOG_DEBUG("type has been placed ... , type = CLEAR");
    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[6] = UART_protocol_structure.Tailframe1;
    frame[7] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_INFO("CLEAR frame has sent,checksum = 0x%04x",check);
}

void UART_Protocol_WEIGHT(UART_protocol UART_protocol_structure , float weight)
{
    typedef union
    {
        float value;
        uint8_t value_arr[4];
    }dataunion;

    LOG_DEBUG("untion object has been built ...");
    dataunion data;
    data.value = weight;
    uint8_t frame[12];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = HX711_WEIGHT;
    frame[3] = 4;

    LOG_DEBUG("type has been placed ... , type = HX711_WEIGHT");
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
    // LOG_INFO("WEIGHT frame has sent,checksum = 0x%04x",check);
}

void UART_Protocol_CURRENT_USER(UART_protocol UART_protocol_structure,char* name,Medicine medicine,uint8_t size)
{
    uint16_t frame_size = size + 9;
    uint8_t* frame = (uint8_t*)malloc(frame_size);
    
    if (frame == NULL) 
    {
        return;
    }

    uint16_t index = 0;

    // 帧头
    frame[index++] = UART_protocol_structure.Headerframe1;
    frame[index++] = UART_protocol_structure.Headerframe2;

    // 类型和长度
    frame[index++] = CURRENT_USER;
    frame[index++] = size + 1;

    frame[index++] = medicine;

    // 数据
    for (uint16_t i = 0; i < size; i++) 
    {
        frame[index++] = name[i];
    }

    // 校验和（从类型开始到数据结束）
    uint16_t check = calculateChecksum(&frame[2], size + 2);
    frame[index++] = (check >> 8) & 0xff;
    frame[index++] = check & 0xff;

    // 帧尾
    frame[index++] = UART_protocol_structure.Tailframe1;
    frame[index++] = UART_protocol_structure.Tailframe2;

    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    // 释放内存
    free(frame);
}


void UART_Protocol_MOTOR_STOP(UART_protocol UART_protocol_structure)
{

    uint8_t frame[8];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = MOTOR_STOP;
    frame[3] = 0;

    LOG_DEBUG("type has been placed ... , type = MOTOR_STOP");
    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[6] = UART_protocol_structure.Tailframe1;
    frame[7] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_INFO("MOTOR_STOP frame has sent,checksum = 0x%04x",check);
}

void UART_Protocol_MOTOR_READY(UART_protocol UART_protocol_structure)
{
    uint8_t frame[8];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = MOTOR_READY;
    frame[3] = 0;

    LOG_DEBUG("type has been placed ... , type = MOTOR_READY");
    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check & 0xff;

    LOG_DEBUG("checksum has been placed ... , checksum = 0x%04x",check);
    frame[6] = UART_protocol_structure.Tailframe1;
    frame[7] = UART_protocol_structure.Tailframe2;

    LOG_DEBUG("Tailer has been placed ...");
    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
    LOG_INFO("MOTOR_READY frame has sent,checksum = 0x%04x",check);
}


void UART_Protocol_TARGET_WEIGHT(UART_protocol UART_protocol_structure , float weight)
{
    typedef union
    {
        float value;
        uint8_t value_arr[4];
    }dataunion;

    LOG_DEBUG("untion object has been built ...");
    dataunion data;
    data.value = weight;
    uint8_t frame[12];

    LOG_DEBUG("data frame data variable has been built ...");
    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    LOG_DEBUG("header has been placed ...");
    frame[2] = TARGET_WEIGHT;
    frame[3] = 4;

    LOG_DEBUG("type has been placed ... , type = TARGET_WEIGHT");
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
        LOG_INFO("Frame too short, size=%d", size);
        LOG_INFO("abanddoned data:");
        for (uint8_t i = 0; i < size; i++)
        {
            LOG_INFO("0x%.2x ",data[i]);
        }
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
    if(size != (frame_len + 8))
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
    LOG_INFO("the frame is valid: type=%d, len=%d", frame_type, frame_len);
    // 解析数据
    if(frame_type == INT && frame_len == 4)
    {
        int32_t value = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
        handle_INT(value);
    }
    else if(frame_type == FLOAT && frame_len == 4)
    {
        union {
            float f;
            uint8_t b[4];
        } u;
        u.b[0] = payload[3];
        u.b[1] = payload[2];
        u.b[2] = payload[1];
        u.b[3] = payload[0];
        handle_FLOAT(u.f);
        // HAL_UART_Transmit_DMA(&huart1,data,size);
    }
    else if(frame_type == ACK && frame_len == 0)
    {
        handle_ACK();
    }
    else if (frame_type == PASSENGER_NUM && frame_len == 1)
    {
        uint8_t value = *payload;
        handle_PASSENGER(value); 
    }
    else if (frame_type == CLEAR && frame_len == 0)
    {
        handle_CLEAR();
    }
    else if (frame_type == HX711_WEIGHT && frame_len == 4)
    {
        union {
            float f;
            uint8_t b[4];
        } u;
        u.b[0] = payload[3];
        u.b[1] = payload[2];
        u.b[2] = payload[1];
        u.b[3] = payload[0];
        handle_WEIGHT(u.f);        
    }
    else if (frame_type == CURRENT_USER)
    {
        char buf[32] = {0};
        Medicine medicine = (Medicine)payload[0];
        for (uint8_t i = 0; i < frame_len - 1; i++)
        {
            buf[i] = payload[i + 1];
        }
        handle_CURRENT_USER(buf,medicine,strlen(buf));
                
    }
    else if (frame_type == MOTOR_READY && frame_len == 0)
    {
        handle_Motor_Ready();        
    }
    else if (frame_type == MOTOR_STOP && frame_len == 0)
    {
        handle_Motor_Stop();
    }
    else if(frame_type == TARGET_WEIGHT && frame_len == 4)
    {
        union {
            float f;
            uint8_t b[4];
        } u;
        u.b[0] = payload[3];
        u.b[1] = payload[2];
        u.b[2] = payload[1];
        u.b[3] = payload[0];
        handle_Target_Weight(u.f);
        // HAL_UART_Transmit_DMA(&huart1,data,size);
    }
}

