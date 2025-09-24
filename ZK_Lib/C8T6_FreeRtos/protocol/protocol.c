#include "protocol.h"

typedef struct
{
    INT_Callback int_callback;
    FLOAT_Callback float_callback;
    ACK_Callback ack_callback;
}callback_functions;

callback_functions callbacks = {0};

static uint16_t calculateChecksum(const uint8_t* data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

static void handle_INT(int32_t value)
{
    if (callbacks.int_callback) 
    {
        callbacks.int_callback(value);
        return;
    }
    else
    {
        char buffer[16];
        sprintf(buffer, "INT: %ld", value);
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
        // HAL_UART_Transmit(&huart1, (uint8_t *)&value, sizeof(value), HAL_MAX_DELAY);
    }
}

static void handle_FLOAT(float value)
{
    if (callbacks.int_callback) 
    {
        callbacks.int_callback(value);
        return;
    }
    else
    {
        char buffer[16];
        sprintf(buffer, "FLOAT: %.2f", value);
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
        // HAL_UART_Transmit(&huart1, (uint8_t *)&value, sizeof(value), HAL_MAX_DELAY);
    }
}

static void handle_ACK(void)
{
    if (callbacks.ack_callback) 
    {
        callbacks.ack_callback();
        return;
    }
    else
    {
        char buffer[16];
        sprintf(buffer, "ACK");
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
    }
}

void set_INT_Callback(INT_Callback cb)
{
    callbacks.int_callback = cb;
}

void set_FLOAT_Callback(FLOAT_Callback cb)
{
    callbacks.float_callback = cb;
}

void set_ACK_Callback(ACK_Callback cb)
{
    callbacks.ack_callback = cb;
}


void UART_Protocol_INT(UART_protocol UART_protocol_structure,int32_t value)
{
    typedef union
    {
        int value;
        uint8_t value_arr[4];
    }dataunion;

    dataunion data;
    data.value = value;
    uint8_t frame[12];

    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    frame[2] = INT;
    frame[3] = 4;

    frame[4] = data.value_arr[3];
    frame[5] = data.value_arr[2];
    frame[6] = data.value_arr[1];
    frame[7] = data.value_arr[0];

    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    frame[10] = UART_protocol_structure.Tailframe1;
    frame[11] = UART_protocol_structure.Tailframe2;

    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
}

void UART_Protocol_FLOAT(UART_protocol UART_protocol_structure,float value)
{
    typedef union
    {
        float value;
        uint8_t value_arr[4];
    }dataunion;

    dataunion data;
    data.value = value;
    uint8_t frame[12];

    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    frame[2] = FLOAT;
    frame[3] = 4;

    frame[4] = data.value_arr[3];
    frame[5] = data.value_arr[2];
    frame[6] = data.value_arr[1];
    frame[7] = data.value_arr[0];

    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    frame[10] = UART_protocol_structure.Tailframe1;
    frame[11] = UART_protocol_structure.Tailframe2;

    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
}

void UART_Protocol_ACK(UART_protocol UART_protocol_structure)
{
    uint8_t frame[8];

    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    frame[2] = ACK;
    frame[3] = 0;

    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check & 0xff;

    frame[6] = UART_protocol_structure.Tailframe1;
    frame[7] = UART_protocol_structure.Tailframe2;

    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
}

void Receive_Uart_Frame(UART_protocol UART_protocol_structure, UartFrame frame)
{
    // 最小帧长度检查
    if(frame.len < 8) return; // 最小帧长度（头2+类型1+长度1+校验2+尾2）

    uint8_t *data = frame.data;

    // 检查头
    if(data[0] != UART_protocol_structure.Headerframe1 ||
       data[1] != UART_protocol_structure.Headerframe2)
        return;

    uint8_t frame_type = data[2];
    uint8_t frame_len  = data[3];

    // 检查长度是否合理
    if(frame.len != (frame_len + 8)) return;

    // 数据区
    uint8_t *payload = &data[4];

    // 校验
    uint16_t recv_check = (data[frame_len + 4] << 8) | data[frame_len + 5];
    uint16_t calc_check = calculateChecksum(&data[2], 2 + frame_len);
    if(recv_check != calc_check) return;

    // 检查尾
    if(data[frame_len + 6] != UART_protocol_structure.Tailframe1 ||
       data[frame_len + 7] != UART_protocol_structure.Tailframe2)
        return;

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
    }
    else if(frame_type == ACK && frame_len == 0)
    {
        handle_ACK();
    }
    // 其他类型可扩展
}

