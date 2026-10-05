#include <Arduino.h>

#include "config.h"
#include "traffic_light.h"

namespace {
constexpr uint32_t YELLOW_DURATION_MS = 2000;
TrafficLightState state = TrafficLightState::Green;
uint32_t stateStartedAt = 0;

void setTrafficLight(bool red, bool yellow, bool green) {
    digitalWrite(TRAFFIC_LIGHT_RED_PIN, red ? HIGH : LOW);
    digitalWrite(TRAFFIC_LIGHT_YELLOW_PIN, yellow ? HIGH : LOW);
    digitalWrite(TRAFFIC_LIGHT_GREEN_PIN, green ? HIGH : LOW);
}

void changeState(TrafficLightState nextState) {
    state = nextState;
    stateStartedAt = millis();

    switch (state) {
        case TrafficLightState::Green:
            setTrafficLight(false, false, true);
            Serial.println("Ampel: GRUEN");
            break;
        case TrafficLightState::YellowToRed:
        case TrafficLightState::YellowToGreen:
            setTrafficLight(false, true, false);
            break;
        case TrafficLightState::Red:
            setTrafficLight(true, false, false);
            Serial.println("Ampel: ROT");
            break;
    }
}
}

void trafficLightInit() {
    pinMode(TRAFFIC_LIGHT_RED_PIN, OUTPUT);
    pinMode(TRAFFIC_LIGHT_YELLOW_PIN, OUTPUT);
    pinMode(TRAFFIC_LIGHT_GREEN_PIN, OUTPUT);
    setTrafficLight(false, false, true);
    state = TrafficLightState::Green;
    stateStartedAt = millis();
}

void trafficLightUpdate(bool motionDetected) {
    switch (state) {
        case TrafficLightState::Green:
            if (motionDetected) {
                Serial.println("BEWEGUNG ERKANNT!");
                changeState(TrafficLightState::YellowToRed);
            }
            break;
        case TrafficLightState::YellowToRed:
            if (millis() - stateStartedAt >= YELLOW_DURATION_MS) {
                changeState(TrafficLightState::Red);
            }
            break;
        case TrafficLightState::Red:
            if (!motionDetected) {
                Serial.println("Keine Bewegung mehr");
                changeState(TrafficLightState::YellowToGreen);
            }
            break;
        case TrafficLightState::YellowToGreen:
            if (millis() - stateStartedAt >= YELLOW_DURATION_MS) {
                changeState(TrafficLightState::Green);
            }
            break;
    }
}

const char* trafficLightColor() {
    switch (state) {
        case TrafficLightState::YellowToRed:
        case TrafficLightState::YellowToGreen:
            return "yellow";
        case TrafficLightState::Red:
            return "red";
        case TrafficLightState::Green:
        default:
            return "green";
    }
}