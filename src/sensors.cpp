#include <Arduino.h>
#include <DHT.h>

#include "config.h"
#include "sensors.h"

namespace {
constexpr uint8_t DHT_TYPE = DHT22;
DHT dht(DHT_SENSOR_PIN, DHT_TYPE);
}

void sensorsInit() {
    pinMode(MOTION_SENSOR_PIN, INPUT);
    dht.begin();
}

float sensorsReadTemperature() {
    return dht.readTemperature();
}

float sensorsReadHumidity() {
    return dht.readHumidity();
}

bool sensorsMotionDetected() {
    return digitalRead(MOTION_SENSOR_PIN) == HIGH;
}