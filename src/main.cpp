#include <Arduino.h>

#include "sensors.h"
#include "traffic_light.h"
#include "access_point_server.h"

void setup() {
    Serial.begin(115200);
    sensorsInit();
    trafficLightInit();
    webServerInit();

    Serial.println("ESP32 Projekt gestartet!");
}

void loop() {
    static float temperatur = NAN;
    static float luftfeuchtigkeit = NAN;
    static uint32_t lastSensorRead = 0;
    static bool sensorReadingsInitialized = false;
    const uint32_t now = millis();

    if (!sensorReadingsInitialized || now - lastSensorRead >= 2500) {
        temperatur = sensorsReadTemperature();
        luftfeuchtigkeit = sensorsReadHumidity();
        lastSensorRead = now;
        sensorReadingsInitialized = true;

        if (isnan(temperatur) || isnan(luftfeuchtigkeit)) {
            Serial.println("Fehler beim Lesen des AM2302!");
        } else {
            Serial.print("Temperatur: ");
            Serial.print(temperatur);
            Serial.println(" C");

            Serial.print("Luftfeuchtigkeit: ");
            Serial.print(luftfeuchtigkeit);
            Serial.println(" %");
        }
    }

    const bool motionDetected = sensorsMotionDetected();
    trafficLightUpdate(motionDetected);
    webServerUpdateStatus(temperatur, luftfeuchtigkeit,
                          !isnan(temperatur) && !isnan(luftfeuchtigkeit),
                          motionDetected, trafficLightColor());
    webServerHandleClient();
    delay(5);
}