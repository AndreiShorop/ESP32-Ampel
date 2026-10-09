#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "access_point_server.h"

namespace {
WebServer server(80);
bool filesystemReady = false;
bool serverStarted = false;
float temperatureC = NAN;
float humidityPercent = NAN;
bool readingsValid = false;
bool motionDetected = false;
const char* currentTrafficLight = "green";

void handleIndex() {
    if (!filesystemReady) {
        server.send(503, "text/plain; charset=utf-8",
                    "Webdateien fehlen. Bitte LittleFS-Abbild hochladen.");
        return;
    }

    File page = LittleFS.open("/index.html", "r");
    if (!page) {
        server.send(404, "text/plain; charset=utf-8",
                    "index.html fehlt im LittleFS-Dateisystem.");
        return;
    }

    server.sendHeader("Cache-Control", "no-store");
    server.streamFile(page, "text/html; charset=utf-8");
    page.close();
}

void handleStatus() {
    String json;
    json.reserve(192);
    json = "{\"temperatureC\":";
    json += readingsValid ? String(temperatureC, 1) : "null";
    json += ",\"humidityPercent\":";
    json += readingsValid ? String(humidityPercent, 1) : "null";
    json += ",\"readingsValid\":";
    json += readingsValid ? "true" : "false";
    json += ",\"motionDetected\":";
    json += motionDetected ? "true" : "false";
    json += ",\"trafficLight\":\"";
    json += currentTrafficLight;
    json += "\",\"uptimeSeconds\":";
    json += String(millis() / 1000UL);
    json += ",\"ip\":\"";
    json += WiFi.localIP().toString();
    json += "\"}";

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json; charset=utf-8", json);
}
}

void webServerInit() {
    filesystemReady = LittleFS.begin(false);
    if (!filesystemReady) {
        Serial.println("LittleFS nicht bereit; Webdateien nicht verfuegbar.");
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Verbinde mit WLAN");
    const uint32_t connectionStartedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - connectionStartedAt < 30000UL) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WLAN-Verbindung fehlgeschlagen. SSID und Passwort pruefen.");
        return;
    }

    server.on("/", HTTP_GET, handleIndex);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.onNotFound([]() {
        server.send(404, "text/plain; charset=utf-8", "Nicht gefunden");
    });
    server.begin();
    serverStarted = true;

    Serial.print("Verbunden mit WLAN: ");
    Serial.println(WiFi.SSID());
    Serial.print("Webseite: http://");
    Serial.println(WiFi.localIP());
}

void webServerHandleClient() {
    if (serverStarted) {
        server.handleClient();
    }
}

void webServerUpdateStatus(float newTemperatureC, float newHumidityPercent,
                           bool newReadingsValid, bool newMotionDetected,
                           const char* newTrafficLight) {
    temperatureC = newTemperatureC;
    humidityPercent = newHumidityPercent;
    readingsValid = newReadingsValid;
    motionDetected = newMotionDetected;
    currentTrafficLight = newTrafficLight;
}