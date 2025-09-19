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

void UART_Protocol_ACK(UART_protocol UART_protocol_structure)
{
    uint8_t frame[8];

    frame[0] = UART_protocol_structure.Headerframe1;
    frame[1] = UART_protocol_structure.Headerframe2;

    frame[2] = ACK;
    frame[3] = 0;

    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check && 0xff;

    frame[6] = UART_protocol_structure.Tailframe1;
    frame[7] = UART_protocol_structure.Tailframe2;

    HAL_UART_Transmit(&huart1,frame,sizeof(frame),HAL_MAX_DELAY);
}


