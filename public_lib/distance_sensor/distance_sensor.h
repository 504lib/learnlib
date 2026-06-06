#ifndef __DISTANCE_SENSOR_H__
#define __DISTANCE_SENSOR_H__

#include <stdint.h>
#include <stdbool.h>
#include "../Log/Log.h"

#ifndef DISTANCE_SENSOR_FOR_MAX_DISTANCE
#define DISTANCE_SENSOR_FOR_MAX_DISTANCE  500
#endif


typedef enum
{
    DISTANCE_FRAME_WAITING_HEADER1 = 0,
    DISTANCE_FRAME_WAITING_HEADER2,
    DISTANCE_FRAME_WAITING_LENGTH,
    DISTANCE_FRAME_WAITING_PAYLOAD1,
    DISTANCE_FRAME_WAITING_PAYLOAD2,
    DISTANCE_FRAME_WAITING_CHECKSUM1,
    DISTANCE_FRAME_WAITING_CHECKSUM2
} distance_frame_state_t;

typedef struct
{
    union
    {
        uint16_t raw_distance;
        uint8_t distance_bytes[2];
    }distance_data;
    distance_frame_state_t frame_state;
} distance_sensor;

bool distance_sensor_process_byte(distance_sensor *sensor, uint8_t byte);
uint16_t distance_sensor_get_distance(const distance_sensor *sensor);


#endif