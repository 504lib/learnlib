#include "../lib/MQ4_calculate.hpp"


float readMQ135Resistance(uint8_t pin) 
{
  int adc_value = analogRead(pin);
  float voltage = adc_value * (3.3 / 4095.0);
  return (3.3 - voltage) / voltage * RLOAD;
}

float calculatePPM(float resistance) {
  return 116.6020682 * pow((resistance / RZERO), -2.769034857);
}