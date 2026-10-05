#pragma once

#include <Arduino.h>

enum class TrafficLightState : uint8_t {
	Green,
	YellowToRed,
	Red,
	YellowToGreen
};

void trafficLightInit();
void trafficLightUpdate(bool motionDetected);
const char* trafficLightColor();