#include <Arduino.h>
#include <EEPROM.h>

#include "configsvc.hpp"
#include "constants/pins.hpp"
#include "ledsvc.hpp"
#include "setupsvc.hpp"
#include "wifisvc.hpp"
#include "doorstatesvc.hpp"
#include "webhooksvc.hpp"

#include <WiFi.h>
#include <ESP32OTAPull.h>
#include <HTTPClient.h>

#define VERSION "1.0.0"

#define TICK_DELAY 10

#define AUTO 0
#define CLOSED 1
#define OPEN 2

const char* updateURL = "http://iphere:8000/update.json";

LEDService LEDSvc;
WifiService WifiSvc;
ConfigManager ConfigSvc;
SetupService SetupSvc;
DoorStateService DoorStateSvc;
WebhookService WebhookSvc;

ESP32OTAPull ota;

unsigned long lastCheck = 0;

bool getDoorState();
void startBackgroundThread();
void asyncTick(void *parameter);

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing!");

    LEDSvc.setBrightness(0.3);
    LEDSvc.set(COLOR_BLUE);

    startBackgroundThread();

    SetupSvc.start();

    WifiSvc.connect();

    Serial.println("Waiting 10s for initial data collection...");
    delay(10000);
    
    Serial.println("Performing webhook init...");
    WebhookSvc.init(getDoorState());

    Serial.print("Firmware Version: ");
    Serial.println(VERSION);

    ota.SetCallback([](int offset, int total) {
    Serial.printf("Downloading: %d / %d bytes\n",
                  offset, total);
});

}

void checkOTA() {

    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (millis() - lastCheck < 30000) {
        return;
    }

    lastCheck = millis();

    Serial.println("Checking for updates...");

    Serial.print("Current Version: ");
    Serial.println(VERSION);

    int result = ota.CheckForOTAUpdate(
        updateURL,
        VERSION,
        ESP32OTAPull::DONT_DO_UPDATE
    );

    Serial.print("Server Version: ");
    Serial.println(ota.GetVersion());

    Serial.print("Check Result: ");
    Serial.println(result);

    if (result == ESP32OTAPull::UPDATE_AVAILABLE) {

        Serial.println("New firmware detected!");
        Serial.println("Starting download...");

        result = ota.CheckForOTAUpdate(
            updateURL,
            VERSION,
            ESP32OTAPull::UPDATE_AND_BOOT
        );

        Serial.print("Installation Result: ");
        Serial.println(result);
    }
}

void loop() {
    Serial.printf("Loop - %.2fs\n", (millis() / 1000.0));

    bool doorState = getDoorState();
    byte switchState = DoorStateSvc.getSwitchState();

    if (doorState && switchState == AUTO) {
        LEDSvc.set(COLOR_GREEN);
    } else if (!doorState && switchState == AUTO) {
        LEDSvc.set(COLOR_RED);
    } else if (doorState && switchState == OPEN) {
        LEDSvc.set(COLOR_YELLOW_GREEN);
    } else if (!doorState && switchState == CLOSED) {
        LEDSvc.set(COLOR_DARK_ORANGE);
    }

    WebhookSvc.trySendMessage(doorState);
    checkOTA();

    delay(1000);
}

bool getDoorState() {
    bool doorState = DoorStateSvc.getSensorState();
    String doorStateString = DoorStateSvc.getStateString();
    Serial.println(doorStateString);

    return doorState;
}

void startBackgroundThread() {
    Serial.println("Starting background thread");
    xTaskCreatePinnedToCore(asyncTick, "Background Thread", 10000, NULL, 1,
                            NULL, 0);
}

void asyncTick(void *parameter) {
    for (;;) {
        LEDSvc.tick();
        DoorStateSvc.tick();
        delay(TICK_DELAY);
    }
}