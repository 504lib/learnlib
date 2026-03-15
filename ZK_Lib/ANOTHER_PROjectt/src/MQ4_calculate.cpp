#include "../lib/MQ4_calculate.hpp"
#include <math.h>


float readMQ135Resistance(uint8_t pin) 
{
  int adc_value = analogRead(pin);
  float voltage = adc_value * (3.3 / 4095.0);

  // 电压接近 0 或接近满量程时，计算电阻会失真甚至除零，直接判无效。
  if (voltage <= 0.001f || voltage >= 3.299f) {
    return NAN;
  }

  return (3.3 - voltage) / voltage * RLOAD;
}

float calculatePPM(float resistance) {
  if (!isfinite(resistance) || resistance <= 0.0f) {
    return NAN;
  }
  return 116.6020682 * pow((resistance / RZERO), -2.769034857);
}