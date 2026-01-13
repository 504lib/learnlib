#include "protocol.hpp"


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

protocol::protocol(uint8_t header1, uint8_t header2, uint8_t tail1, uint8_t tail2)
    : Headerframe1(header1), Headerframe2(header2), Tailframe1(tail1), Tailframe2(tail2)
{
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, 16, 17);
     // RX1=16, TX1=17
}


void protocol::Send_Uart_Frame(DataPacket_t packet)
{
    switch (packet.type)
    {
    case CmdType::INT:
        {
            if (packet.length !=4)
            {
                LOG_WARN("Invalid INT payload length: %d", packet.length);
                return;
            }
            int32_t int_value = rd_u32_be(&packet.data[0]);
            Send_Uart_Frame(int_value);
        }
        break;
    case CmdType::FLOAT:
        {
            if (packet.length !=4)
            {
                LOG_WARN("Invalid FLOAT payload length: %d", packet.length);
                return;
            }
            float float_value = rd_f32_be(&packet.data[0]);
            Send_Uart_Frame(float_value);
        }
        break;
    case CmdType::ACK:
        Send_Uart_Frame_ACK();
        break;
    case CmdType::PASSENGER_NUM:
        {
            if (packet.length !=2)
            {
                LOG_WARN("Invalid PASSENGER_NUM payload length: %d", packet.length);
                return;
            }
            Rounter router = static_cast<Rounter>(packet.data[0]);
            uint8_t passenger_num = packet.data[1];
            Send_Uart_Frame(router,passenger_num);
        }
        break;
    case CmdType::CLEAR:
        {
            if (packet.length !=1)
            {
                LOG_WARN("Invalid CLEAR payload length: %d", packet.length);
                return;
            }
            Rounter router = static_cast<Rounter>(packet.data[0]);
            Send_Uart_Frame(router);
        }
        break;
    case CmdType::VEHICLE_STATUS:
        {
            if (packet.length !=1)
            {
                LOG_WARN("Invalid VEHICLE_STATUS payload length: %d", packet.length);
                return;
            }
            VehicleStatus status = static_cast<VehicleStatus>(packet.data[0]);
            LOG_INFO("Sending VEHICLE_STATUS: Status=%d\n", static_cast<uint8_t>(status));
            Send_Uart_Frame(status);
        }
        break;
    default:
        break;
    }
}

void protocol::Send_Uart_Frame(int32_t num)
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

    wr_u32_be(&frame[4], static_cast<uint32_t>(num));

    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    frame[10] = Tailframe1;
    frame[11] = Tailframe2;
    Serial1.write(frame,sizeof(frame));
    LOG_DEBUG("Sending INT Frame: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
              frame[6], frame[7], frame[8], frame[9], frame[10], frame[11]);
}

void protocol::Send_Uart_Frame(float num)
{
    uint8_t frame[12];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::FLOAT);
    frame[3] = 4;

    wr_f32_be(&frame[4], num);

    uint16_t check = calculateChecksum(&frame[2],6);
    frame[8] = (check >> 8) & 0xff;
    frame[9] = check & 0xff;

    frame[10] = Tailframe1;
    frame[11] = Tailframe2;

    Serial1.write(frame,sizeof(frame));
    LOG_DEBUG("Sending FLOAT Frame: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
              frame[6], frame[7], frame[8], frame[9], frame[10], frame[11]);
}

uint16_t protocol::calculateChecksum(const uint8_t* data, size_t length)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
    LOG_DEBUG("Calculated Checksum: 0x%04X", sum);
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
    frame[5] = check & 0xff;

    frame[6] = Tailframe1;
    frame[7] = Tailframe2;

    Serial1.write(frame,sizeof(frame));
    LOG_DEBUG("Sending ACK Frame: %02X %02X %02X %02X %02X %02X %02X %02X",
              frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
              frame[6], frame[7]);
}

void protocol::Send_Uart_Frame(Rounter rounter,uint8_t value)
{
    uint8_t frame[10];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::PASSENGER_NUM);
    frame[3] = 2;
    frame[4] = static_cast<uint8_t>(rounter);
    frame[5] = value;

    uint16_t check = calculateChecksum(&frame[2],4);
    frame[6] = (check >> 8) & 0xff;
    frame[7] = check & 0xff;

    frame[8] = Tailframe1;
    frame[9] = Tailframe2;

    Serial1.write(frame,sizeof(frame));
    LOG_DEBUG("Sending PASSENGER_NUM Frame: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
              frame[6], frame[7], frame[8], frame[9]);
}

void protocol::Send_Uart_Frame(Rounter rounter)
{
    uint8_t frame[9];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::CLEAR);
    frame[3] = 1;
    frame[4] = static_cast<uint8_t>(rounter);

    uint16_t check = calculateChecksum(&frame[2],3);
    frame[5] = (check >> 8) & 0xff;
    frame[6] = check & 0xff;

    frame[7] = Tailframe1;
    frame[8] = Tailframe2;

    Serial1.write(frame,sizeof(frame));
    LOG_DEBUG("Sending CLEAR Frame: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
              frame[6], frame[7], frame[8]);
}

void protocol::Send_Uart_Frame(VehicleStatus status)
{
    uint8_t frame[9];

    frame[0] = Headerframe1;
    frame[1] = Headerframe2;

    frame[2] = static_cast<uint8_t>(CmdType::VEHICLE_STATUS);
    frame[3] = 1;
    frame[4] = static_cast<uint8_t>(status);

    uint16_t check = calculateChecksum(&frame[2],3);
    frame[5] = (check >> 8) & 0xff;
    frame[6] = check & 0xff;

    frame[7] = Tailframe1;
    frame[8] = Tailframe2;

    Serial1.write(frame,sizeof(frame));

    LOG_DEBUG("Sending VEHICLE_STATUS Frame: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
              frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
              frame[6], frame[7], frame[8]);
}

void protocol::Register_Hander(CmdType cmd, CmdHandler handler)
{
    if (handler == nullptr || static_cast<uint8_t>(cmd) >= MAX_CMD_TYPE_NUM)
    {
        LOG_INFO("Invalid handler or command type");
        return;
    }
    handlers[static_cast<uint8_t>(cmd)] = handler;
}

void protocol::Receive_Uart_Frame(uint8_t data)
{
    LOG_DEBUG("Received byte: %02X", data);
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
            if(data == Headerframe1)
            {
                state = WAIT_HEADER2;
                LOG_DEBUG("Header1 matched");
            }
            break;
        case WAIT_HEADER2:
            if(data == Headerframe2) 
            {
                state = WAIT_TYPE;
                LOG_DEBUG("Header2 matched");
            }
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
            if(data == Tailframe1) 
            {
                state = WAIT_TAIL2;
                LOG_DEBUG("Tail1 matched");
            }
            else state = WAIT_HEADER1;
            break;
        case WAIT_TAIL2:
            if(data == Tailframe2)
            {
                // 校验
                uint8_t check_buf[10];
                check_buf[0] = frame_type;
                check_buf[1] = frame_len;
                memcpy(&check_buf[2], data_buf, frame_len);
                uint16_t calc_check = calculateChecksum(check_buf, 2 + frame_len);
                uint16_t recv_check = (check1 << 8) | check2;
                LOG_DEBUG("Calculated Checksum: 0x%04X, Received Checksum: 0x%04X", calc_check, recv_check);
                if(calc_check == recv_check)
                {
                    if (frame_type < MAX_CMD_TYPE_NUM && handlers[frame_type] != nullptr)
                    {
                        handlers[frame_type](static_cast<CmdType>(frame_type), data_buf, frame_len);
                    }
                    else
                    {
                        LOG_INFO("Unknown frame type or invalid length: Type=%d, Length=%d", frame_type, frame_len);
                        // Serial.println("Unknown frame type or invalid length");
                    }
                }
                else
                {
                    LOG_INFO("Checksum error: Calculated 0x%04X, Received 0x%04X", calc_check, recv_check);
                    // Serial.println("Checksum error");
                }
            }
            // 无论校验是否通过，都回到初始状态
            state = WAIT_HEADER1;
            break;
    }
}
protocol::~protocol()
{
}
