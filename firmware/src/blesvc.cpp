/*
    Based on Neil Kolban example for IDF:
   https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/
// https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_uart/BLE_uart.ino

#include "blesvc.hpp"

#include <Arduino.h>

#include "setupsvc.hpp"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "f97b5d7f-1291-4490-af66-e804f9a3a750" // UART service UUID
#define CHARACTERISTIC_UUID_RX "d42b21fc-126b-42ad-92f3-6165489fa9eb"
#define CHARACTERISTIC_UUID_TX "5fc27ec6-7aef-491e-9918-e2b15e777657"

std::string rxBuffer;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) { BLESvc.onConnect(); };

    void onDisconnect(BLEServer *pServer) { BLESvc.onDisconnect(); }
};

void ble_send_callback(const char *string) { BLESvc.send(string); }
int ble_read_int_callback(int timeout) { return BLESvc.readInt(timeout); }
String ble_read_str_callback(bool hideInput) { return BLESvc.readString(); }

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            rxBuffer = rxValue;
            // Serial.print("BLE Received Value: ");
            // ble_send_callback("Bongus\n");
            // SetupSvc.start(ble_send_callback);
            // for (int i = 0; i < rxValue.length(); i++) {
            //     Serial.print(rxValue[i]);
            // }
            // for (int i = 0; i < rxValue.length(); i++) {
            //     Serial.printf("%d\n", rxValue[i]);
            // }
            // if (rxValue.compare("Webhook\n") == 0) {
            //     Serial.println("Returning BLE message...");
            //     BLESvc.send("Chongus");
            // }
        }
    }
};

void BLEUARTService::setup() {
    // Create the BLE Device
    BLEDevice::init("CSSA Lounge Sensor");

    // Create the BLE Server
    this->pServer = BLEDevice::createServer();
    this->pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    this->pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

    this->pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    this->pServer->getAdvertising()->start();
    Serial.println("BLE initialized...");
}

void BLEUARTService::tick() {
    // disconnecting
    if (!this->deviceConnected && this->oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        this->pServer->startAdvertising(); // restart advertising
        Serial.println("BLE advertising started...");
        this->oldDeviceConnected = this->deviceConnected;
    }
    // connecting
    if (this->deviceConnected && !this->oldDeviceConnected) {
        // do stuff here on connecting
        this->oldDeviceConnected = this->deviceConnected;
        Serial.println("New Bluetooth device connected! Sending info....");
        delay(1000);
        SetupSvc.start(ble_send_callback, ble_read_int_callback,
                       ble_read_str_callback);
    }
}

void BLEUARTService::onDisconnect() { this->deviceConnected = false; }

void BLEUARTService::onConnect() { this->deviceConnected = true; }

void BLEUARTService::send(const char *string) {
    // char buffer[50];
    // va_list args;
    // va_start(args, format);
    // snprintf(buffer, sizeof(buffer), format, args);
    // Serial.println("Print function generated: ");
    // Serial.println(buffer);

    // char buffer[5];
    // strcpy(buffer, "test");
    // int value = 40;
    this->pTxCharacteristic->setValue(string);
    this->pTxCharacteristic->notify();
}

int BLEUARTService::readInt(int timeout) {
    int timeoutRemaining = timeout;
    while (this->deviceConnected && rxBuffer.empty() && timeoutRemaining > 0) {
        delay(100);
        timeoutRemaining -= 100;
    }

    Serial.print("Read message from BLE: ");
    Serial.println(rxBuffer.c_str());
    int result =
        atoi(rxBuffer.c_str()); // strtoimax(rxBuffer.c_str(), nullptr, 10);
    rxBuffer.clear();
    return result;
}

String BLEUARTService::readString() {
    // TODO: Device disconnections won't ever process because this is in the
    // same thread as the disconnects (tick starts the setup service)
    while (this->deviceConnected && rxBuffer.empty()) {
        delay(100);
    }

    Serial.print("Read message from BLE: ");
    Serial.println(rxBuffer.c_str());
    // TODO: Will this stay allocated? Maybe not, could be a problem
    String result = String(rxBuffer.c_str());
    return result;
}