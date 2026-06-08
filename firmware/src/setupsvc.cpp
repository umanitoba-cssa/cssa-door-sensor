#include "setupsvc.hpp"
#include "configsvc.hpp"
#include "ledsvc.hpp"
#include "serialutils.cpp"
#include "wifisvc.hpp"
#include <Arduino.h>

SetupService::SetupService() {}

void SetupService::start(print_fn print, read_int_fn intReader,
                         read_str_fn strReader) {
    this->print = print;
    this->intReader = intReader;
    this->strReader = strReader;
    menu();
}

void SetupService::menu() {
    char buffer[256];
    while (true) {
        LEDSvc.set(COLOR_ORANGE);
        ConfigData data = ConfigSvc.load();

        snprintf(buffer, sizeof(buffer), "Current configuration:\n");
        this->print(buffer);
        snprintf(buffer, sizeof(buffer), "Wifi SSID: %s\n", data.ssid);
        this->print(buffer);
        if (data.wifimode == 2) {
            snprintf(buffer, sizeof(buffer), "Wifi user: %s\n", data.userid);
            this->print(buffer);
        }
        snprintf(buffer, sizeof(buffer), "Webhook: %s\n\n", data.webhook);
        this->print(buffer);

        snprintf(buffer, sizeof(buffer), "Select option:\n");
        this->print(buffer);
        snprintf(buffer, sizeof(buffer), "1. Configure Wifi\n");
        this->print(buffer);
        snprintf(buffer, sizeof(buffer), "2. Set Webhook\n");
        this->print(buffer);
        snprintf(buffer, sizeof(buffer), "3. Clear configuration\n");
        this->print(buffer);
        snprintf(buffer, sizeof(buffer), "0. Resume boot\n");
        this->print(buffer);
        int option = intReader(10000);

        switch (option) {
        case 1:
            wifi();
            break;
        case 2:
            webhook();
            break;
        case 3:
            clear();
            break;
        case 0:
        case -1:
            return;
        default:
            snprintf(buffer, sizeof(buffer), "Invalid option\n");
            this->print(buffer);
            break;
        }
    }
}

void SetupService::wifi() {
    char buffer[256];

    String ssid;
    String userId;
    String password;
    bool result;
    snprintf(buffer, sizeof(buffer), "Wifi network type:\n");
    this->print(buffer);
    snprintf(buffer, sizeof(buffer), "1. Standard\n");
    this->print(buffer);
    snprintf(buffer, sizeof(buffer),
             "2. WPA2 Enterprise (User ID and Password)\n");
    this->print(buffer);

    int option = this->intReader(10000);

    switch (option) {
    case 1:
        snprintf(buffer, sizeof(buffer), "SSID: ");
        this->print(buffer);
        ssid = this->strReader(false); // SerialUtils::readString();
        snprintf(buffer, sizeof(buffer), "Password: ");
        this->print(buffer);
        password = this->strReader(true); // SerialUtils::readString(true);
        snprintf(buffer, sizeof(buffer), "%s/%s\n", ssid, password);
        this->print(buffer);
        result = WifiSvc.connTestStandard(ssid, password);
        if (result) {
            WifiSvc.saveStandard(ssid, password);
        }
        break;
    case 2:
        snprintf(buffer, sizeof(buffer), "SSID: ");
        this->print(buffer);
        ssid = this->strReader(false); // SerialUtils::readString();
        snprintf(buffer, sizeof(buffer), "User ID: ");
        this->print(buffer);
        userId = this->strReader(false); // SerialUtils::readString();
        snprintf(buffer, sizeof(buffer), "Password: ");
        this->print(buffer);
        password = this->strReader(true); // SerialUtils::readString(true);
        result = WifiSvc.connTestEnterprise(ssid, userId, password);
        if (result) {
            WifiSvc.saveEnterprise(ssid, userId, password);
        }
        break;
    default:
        snprintf(buffer, sizeof(buffer), "Invalid option");
        this->print(buffer);
        break;
    }
}

void SetupService::webhook() {
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "Webhook: ");
    this->print(buffer);
    String webhook = this->strReader(false);
    ConfigSvc.saveWebhookUrl(webhook);
}

void SetupService::clear() {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Clearing configuration");
    this->print(buffer);
    ConfigSvc.clear();
}