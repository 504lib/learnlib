#ifndef __MQ4_CALCULATE
#define __MQ4_CALCULATE

#include <Arduino.h>


#define RLOAD 10.0               // MQ-4 负载电阻，单位为kΩ，根据实际情况调整
#define RZERO 9.65               // MQ-4 在100ppm甲烷时

float readMQ135Resistance(uint8_t pin);
float calculatePPM(float resistance);


#endif // !__MQ4_CALCULATE