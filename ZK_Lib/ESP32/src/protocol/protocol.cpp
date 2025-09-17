#include "protocol.hpp"

protocol::protocol(uint8_t header1, uint8_t header2, uint8_t tail1, uint8_t tail2)
    : Headerframe1(header1), Headerframe2(header2), Tailframe1(tail1), Tailframe2(tail2)
{
    Serial.begin(115200);
}

void protocol::Send_Uart_Frame(int num)
{
    union dataunion
    {
        int value;
        uint8_t value_arr[4];
    };

    dataunion data;
    data.value = num;
    uint8_t frame[12];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::INT);
    frame[3] = 4;

    frame[4] = data.value_arr[3];
    frame[5] = data.value_arr[2];
    frame[6] = data.value_arr[1];
    frame[7] = data.value_arr[0];

    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    frame[10] = Tailframe1;
    frame[11] = Tailframe2;

    Serial.write(frame,sizeof(frame));
}

void protocol::Send_Uart_Frame(float num)
{
    union dataunion
    {
        float value;
        uint8_t value_arr[4];
    };

    dataunion data;
    data.value = num;
    uint8_t frame[12];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::FLOAT);
    frame[3] = 4;

    frame[4] = data.value_arr[3];
    frame[5] = data.value_arr[2];
    frame[6] = data.value_arr[1];
    frame[7] = data.value_arr[0];

    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    frame[10] = Tailframe1;
    frame[11] = Tailframe2;

    Serial.write(frame,sizeof(frame));

}

uint16_t protocol::calculateChecksum(const uint8_t* data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

void protocol::Send_Uart_Frame_ACK()
{
    uint8_t frame[8];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::ACK);
    frame[3] = 0;

    uint16_t check = calculateChecksum(&frame[2],2);
    frame[4] = (check >> 8) & 0xff;
    frame[5] = check && 0xff;

    frame[6] = Tailframe1;
    frame[7] = Tailframe2;

    Serial.write(frame,sizeof(frame));
}


protocol::~protocol()
{
}
