#pragma once

void webServerInit();
void webServerHandleClient();
void webServerUpdateStatus(float temperatureC, float humidityPercent,
                           bool readingsValid, bool motionDetected,
                           const char* trafficLight);