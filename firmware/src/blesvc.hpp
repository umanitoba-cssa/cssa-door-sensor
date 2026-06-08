#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

class BLEUARTService {
  private:
    BLEServer *pServer = NULL;
    BLECharacteristic *pTxCharacteristic;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;

  public:
    void tick();
    void setup();
    void onDisconnect();
    void onConnect();
    void send(const char *string);
    int readInt(int timeout);
    String readString();
};

extern BLEUARTService BLESvc;