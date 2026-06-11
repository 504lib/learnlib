#include "distance_sensor.h"
#include <string.h>

// 状态机函数指针
typedef bool (*distance_action_t)(distance_sensor *sensor, uint8_t byte);


/**
 * @brief Initializes the distance sensor structure.
 * @param sensor Pointer to the distance sensor structure.
 * @return true if initialization is successful, false otherwise.
 */
bool distance_sensor_init(distance_sensor *sensor)
{
    sensor->distance_data.raw_distance = 0;
    sensor->frame_state = DISTANCE_FRAME_WAITING_HEADER1;
    memset(sensor->distance_data.distance_bytes, 0, sizeof(sensor->distance_data.distance_bytes));
    return true;
}

/**
 * @brief  传感器指针的防御性校验，确保传入的传感器实例有效
 * @param  sensor  指向距离传感器实例的指针
 * @return 校验通过返回 true，指针为空时返回 false
 * @throws 当 sensor 为空指针时记录 FATAL 级别日志
 */
static bool defense_check(distance_sensor *sensor)
{
    if (!sensor)
    {
        LOG_FATAL("defense_check: Invalid sensor pointer");
        return false;
    }
    return true;
}

/**
 * @brief 完整帧接收完成后的距离值读取处理函数
 * @param sensor 距离传感器实例指针，不得为 NULL
 * @return 测量到的原始距离值（单位由协议定义）；若 sensor 为空指针则返回 0xFFFF 表示无效数据
 * @throws 当 sensor 为 NULL 时通过 defense_check 记录错误日志并返回错误码
 */
static uint16_t distance_handler_whole_frame(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return 0xFFFF; // Return 0xFFFF or an appropriate error code
    }
    return sensor->distance_data.raw_distance;
}

static bool distance_analysis_waiting_header1_action(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    if (byte == 0x01)
    {
        sensor->frame_state = DISTANCE_FRAME_WAITING_HEADER2;
        return true;
    }
    LOG_DEBUG("distance_analysis_waiting_header1_action: Invalid header byte: 0x%02X", byte);
    return false;
}

static bool distance_analysis_waiting_header2_action(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    if (byte == 0x03)
    {
        sensor->frame_state = DISTANCE_FRAME_WAITING_LENGTH;
        return true;
    }
    LOG_DEBUG("distance_analysis_waiting_header2_action: Invalid header byte: 0x%02X", byte);
    sensor->frame_state = DISTANCE_FRAME_WAITING_HEADER1; // Reset to waiting for the first header
    return false;
}

static bool distance_analysis_waiting_length_action(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    if (byte == 0x02)// only support 2 bytes payload (distance data)
    {
        sensor->frame_state = DISTANCE_FRAME_WAITING_PAYLOAD1;
        return true;
    }
    LOG_DEBUG("distance_analysis_waiting_length_action: Invalid length byte: 0x%02X", byte);
    sensor->frame_state = DISTANCE_FRAME_WAITING_HEADER1; // Reset to waiting for the first header
    return false;
}

static bool distance_analysis_waiting_payload1_action(distance_sensor *sensor, uint8_t byte)
{
    // Little-endian format: the first byte is the low byte, and the second byte is the high byte
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    sensor->distance_data.distance_bytes[1] = byte;// Store the first byte in the high byte position
    sensor->frame_state = DISTANCE_FRAME_WAITING_PAYLOAD2;
    return true;
}

static bool distance_analysis_waiting_payload2_action(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    sensor->distance_data.distance_bytes[0] = byte;// Store the second byte in the low byte position
    sensor->frame_state = DISTANCE_FRAME_WAITING_CHECKSUM1;
    return true;
}

static bool distance_analysis_waiting_checksum1_action(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    LOG_DEBUG("nothing,because not support crc checksum");
    sensor->frame_state = DISTANCE_FRAME_WAITING_CHECKSUM2;
    return true;
}

static bool distance_analysis_waiting_checksum2_action(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    LOG_DEBUG("nothing,because not support crc checksum");
    sensor->frame_state = DISTANCE_FRAME_WAITING_HEADER1; // Reset to waiting for the first header
    return true;
}


distance_action_t distance_action[DISTANCE_FRAME_WAITING_CHECKSUM2 + 1] = {
    distance_analysis_waiting_header1_action,
    distance_analysis_waiting_header2_action,
    distance_analysis_waiting_length_action,
    distance_analysis_waiting_payload1_action,
    distance_analysis_waiting_payload2_action,
    distance_analysis_waiting_checksum1_action,
    distance_analysis_waiting_checksum2_action
};

bool distance_sensor_process_byte(distance_sensor *sensor, uint8_t byte)
{
    if (!defense_check(sensor))
    {
        return false; // Return false or an appropriate error code
    }
    if (sensor->frame_state > DISTANCE_FRAME_WAITING_CHECKSUM2)
    {
        LOG_FATAL("distance_sensor_process_byte: Invalid frame state: %d", sensor->frame_state);
        sensor->frame_state = DISTANCE_FRAME_WAITING_HEADER1; // Reset to waiting for the first header
        return false;
    }
    return distance_action[sensor->frame_state](sensor, byte);
}

uint16_t distance_sensor_get_distance(const distance_sensor *sensor)
{
    if (!defense_check((distance_sensor *)sensor))
    {
        return 0xFFFF; // Return 0xFFFF or an appropriate error code
    }
	if(sensor->distance_data.raw_distance == 0xFFFF)
	{
		return DISTANCE_SENSOR_FOR_MAX_DISTANCE;
	}
    return sensor->distance_data.raw_distance;
}

