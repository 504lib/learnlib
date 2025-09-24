#include "protocol.h"

static uint16_t calculateChecksum(const uint8_t* data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
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

void Receive_Uart_Frame(UART_protocol UART_protocol_structure,uint8_t data)
{
    static enum { WAIT_HEADER1, WAIT_HEADER2, WAIT_TYPE, WAIT_LEN, WAIT_DATA, WAIT_CHECK1, WAIT_CHECK2, WAIT_TAIL1, WAIT_TAIL2 } state = WAIT_HEADER1;
    static uint8_t frame_type = 0;
    static uint8_t frame_len = 0;
    static uint8_t data_buf[8] = {0};
    static uint8_t data_index = 0;
    static uint16_t checksum = 0;
    static uint8_t check1 = 0, check2 = 0;

    switch(state)
    {
        case WAIT_HEADER1:
            if(data == UART_protocol_structure.Headerframe1) state = WAIT_HEADER2;
            break;
        case WAIT_HEADER2:
            if(data == UART_protocol_structure.Headerframe2) state = WAIT_TYPE;
            else state = WAIT_HEADER1;
            break;
        case WAIT_TYPE:
            frame_type = data;
            state = WAIT_LEN;
            break;
        case WAIT_LEN:
            frame_len = data;
            data_index = 0;
            if(frame_len > sizeof(data_buf)) frame_len = sizeof(data_buf); // 防止溢出
            state = frame_len ? WAIT_DATA : WAIT_CHECK1;
            break;
        case WAIT_DATA:
            data_buf[data_index++] = data;
            if(data_index >= frame_len) state = WAIT_CHECK1;
            break;
        case WAIT_CHECK1:
            check1 = data;
            state = WAIT_CHECK2;
            break;
        case WAIT_CHECK2:
            check2 = data;
            state = WAIT_TAIL1;
            break;
        case WAIT_TAIL1:
            if(data == UART_protocol_structure.Tailframe1) state = WAIT_TAIL2;
            else state = WAIT_HEADER1;
            break;
        case WAIT_TAIL2:
            if(data == UART_protocol_structure.Tailframe2)
            {
                // 校验
                uint8_t check_buf[10];
                check_buf[0] = frame_type;
                check_buf[1] = frame_len;
                memcpy(&check_buf[2], data_buf, frame_len);
                uint16_t calc_check = calculateChecksum(check_buf, 2 + frame_len);
                uint16_t recv_check = (check1 << 8) | check2;
                if(calc_check == recv_check)
                {
                    if(frame_type == INT && frame_len == 4)
                    {
                        int32_t value = (data_buf[0] << 24) | (data_buf[1] << 16) | (data_buf[2] << 8) | data_buf[3];
                        // 处理接收到的整数值
                    }
                    else if(frame_type == FLOAT && frame_len == 4)
                    {
                        union {
                            float f;
                            uint8_t b[4];
                        } u;
                        u.b[0] = data_buf[3];
                        u.b[1] = data_buf[2];
                        u.b[2] = data_buf[1];
                        u.b[3] = data_buf[0];
                        float value = u.f;
                        // 处理接收到的浮点值
                    }
                    else if(frame_type == ACK && frame_len == 0)
                    {
                        // 处理接收到的ACK
                        
                    }
                }
                else
                {
                    
                }
            }
            // 无论校验是否通过，都回到初始状态
            state = WAIT_HEADER1;
            break;
    }
}
