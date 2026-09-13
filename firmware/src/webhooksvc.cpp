#include "webhooksvc.hpp"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "configsvc.hpp"
#include "constants/strings.hpp"

#define STATUS_CHANGE_DELAY_SECONDS 10

bool debouncedDoorState = false;
bool lastDoorState = false;
unsigned long lastStatusChange = 0;

WebhookService::WebhookService() {}

void WebhookService::init(bool initialState) {
    sendMessage(Strings::STARTUP_MSG);
    debouncedDoorState = initialState;
    lastDoorState = initialState;
    sendDoorMessage(initialState);
}

void WebhookService::trySendMessage(bool doorState) {
    unsigned long timestamp = millis();
    if (doorState != lastDoorState) {
        lastDoorState = doorState;
        lastStatusChange = timestamp;
    }

    bool delayExceeded =
        timestamp - lastStatusChange > STATUS_CHANGE_DELAY_SECONDS * 1000;

    if (delayExceeded && lastDoorState != debouncedDoorState) {
        debouncedDoorState = lastDoorState;
        sendDoorMessage(debouncedDoorState);
    }
}

void WebhookService::sendDoorMessage(bool doorState) {
    sendMessage(doorState ? Strings::DOOR_OPEN_MSG : Strings::DOOR_CLOSED_MSG);
}

void WebhookService::sendMessage(String message) {
    DynamicJsonDocument doc(128);
    doc["content"] = message;

    String json;
    serializeJson(doc, json);

    String url = ConfigSvc.getWebhookUrl();
    url.trim(); // Remove any accidental trailing spaces or newlines

    // Set DNS in case local router is unreliable
    IPAddress primaryDNS(1, 1, 1, 1);
    IPAddress secondaryDNS(8, 8, 8, 8);
    WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), primaryDNS,
                secondaryDNS);

    WiFiClientSecure client;
    client.setInsecure(); // Skip TLS certificate verification

    HTTPClient http;
    http.setTimeout(15000);

    if (http.begin(client, url)) {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("User-Agent", "ESP32-DoorSensor");

        int httpCode = http.POST(json);
        Serial.println("Message sent, response code: " + String(httpCode));

        if (httpCode < 0) {
            Serial.printf("HTTP Error: %s\n",
                          http.errorToString(httpCode).c_str());
        }

        http.end();
    } else {
        Serial.println("HTTP begin failed to parse URL.");
    }
}