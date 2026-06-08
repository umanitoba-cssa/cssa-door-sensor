#include <Arduino.h>
#include <EEPROM.h>

#include "blesvc.hpp"
#include "configsvc.hpp"
#include "constants/pins.hpp"
#include "doorstatesvc.hpp"
#include "ledsvc.hpp"
#include "serialutils.cpp"
#include "setupsvc.hpp"
#include "webhooksvc.hpp"
#include "wifisvc.hpp"

#define TICK_DELAY 10

#define AUTO 0
#define CLOSED 1
#define OPEN 2

LEDService LEDSvc;
WifiService WifiSvc;
ConfigManager ConfigSvc;
SetupService SetupSvc;
DoorStateService DoorStateSvc;
WebhookService WebhookSvc;
BLEUARTService BLESvc;

bool getDoorState();
void startBackgroundThread();
void asyncTick(void *parameter);

void printCallback(const char *string) { Serial.print(string); }

int readIntCallback(int timeout) { return SerialUtils::readInt(timeout); }

String readStrCallback(bool hideInput) {
    return SerialUtils::readString(hideInput);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing!");

    LEDSvc.setBrightness(0.3);
    LEDSvc.set(COLOR_BLUE);

    startBackgroundThread();

    SetupSvc.start(printCallback, readIntCallback, readStrCallback);

    BLESvc.setup();

    WifiSvc.connect();

    Serial.println("Waiting 10s for initial data collection...");
    delay(10000);

    Serial.println("Performing webhook init...");
    WebhookSvc.init(getDoorState());
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
        BLESvc.tick();
        delay(TICK_DELAY);
    }
}