#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEBeacon.h>
#include <Wire.h>

#define DEVICE_NAME "BLE_Tracker"
#define SERVICE_UUID "60489361-3439-4319-81a9-f50b938ab2bd"
#define CHARACTERISTIC_UUID "8a0caf07-be2c-43cb-83e6-b8cdb314a851"

// RTC Pill Time Schedule (Hour, Minute)
const int pillTimes[4][2] = {
  {8, 0},  // Morning 8:00 AM
  {12, 30}, // Afternoon 12:30 PM
  {19, 20},  // Evening 7:20 PM
  {21, 30}   // Night 9:30 PM
};

char dayofTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool bleEnabled = false;

BLEBeacon myBeacon;

// BLE Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Wire.begin();
    DS1307_Write(0, 12, 30, 1, 1, 3, 25);  // Initialize RTC

    // Initialize BLE
    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    BLEBeacon oBeacon = BLEBeacon();
    pServer->setCallbacks(new MyServerCallbacks());
    oBeacon.setSignalPower(-59);

    // Create BLE Service & Characteristic
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    pService->start();
}

// RTC Helper Functions
uint8_t decimal_to_bdc(char value) {
    uint8_t msb, lsb, hex;
    msb = value / 10;
    lsb = value % 10;
    hex = ((msb << 4) + lsb);
    return hex;
}

void DS1307_Write(char _second, char _minute, char _hour, char _day, char _date, char _month, char _year) {
    Wire.beginTransmission(0x68);
    Wire.write(0x00);
    Wire.write(decimal_to_bdc(_second));
    Wire.write(decimal_to_bdc(_minute));
    Wire.write(decimal_to_bdc(_hour));
    Wire.write(decimal_to_bdc(_day));
    Wire.write(decimal_to_bdc(_date));
    Wire.write(decimal_to_bdc(_month));
    Wire.write(decimal_to_bdc(_year));
    Wire.endTransmission();
}

uint8_t DS1307_Read(char addr) {
    char data;
    Wire.beginTransmission(0x68);
    Wire.write(addr);
    Wire.endTransmission();
    delay(5);

    Wire.requestFrom(0x68, 1);
    if (Wire.available()) {
        data = Wire.read();
        data = ((data >> 4) * 10 + (data & 0x0F));
    }
    return data;
}

void readTime(int &hour, int &minute) {
    hour = DS1307_Read(0x02);
    minute = DS1307_Read(0x01);
}

bool isPillTime(int currentHour, int currentMinute) {
    for (int i = 0; i < 4; i++) {
        if (currentHour == pillTimes[i][0] && currentMinute == pillTimes[i][1]) {
            return true;
        }
        else 
        {
          delay(100);
        }
    }
    return false;
}

void loop() {
    int currentHour, currentMinute;
    readTime(currentHour, currentMinute);

    bool shouldBroadcast = isPillTime(currentHour, currentMinute);

    if (shouldBroadcast && !bleEnabled) {
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        Serial.println("BLE Tracking Started at Pill Time!");
        bleEnabled = true;
    } else if (!shouldBroadcast && bleEnabled) {
        BLEDevice::stopAdvertising();
        Serial.println("BLE Tracking Stopped");
        bleEnabled = false;
    }

    if (deviceConnected && bleEnabled) {
        String trackingMessage = "BLE Tracker ID: 1234";
        Serial.println("Transmitting: " + trackingMessage);
        pCharacteristic->setValue(trackingMessage.c_str());
        pCharacteristic->notify();
    }

    delay(1000);  // Update every second
}
