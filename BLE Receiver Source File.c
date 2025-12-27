// Dispensser part
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>

#define SCAN_TIME 5 // BLE Scan duration in seconds
#define SER_BUF_SIZE 1024
#define BAURDRATE 115200
#define TxPin 17
#define RxPin 16

HardwareSerial  MySerial(0);
BLEScan* pBLEScan;
Servo myservo, myservo1;

int pos0 = 0;
int pos1 = 90;
int pos2 = 180;
int currentRSSI = 0;

// Callback function to handle detected BLE devices
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.println("------------------------------------------------");
        Serial.print("Device Address: ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());
        
        if (advertisedDevice.haveRSSI()) {
            Serial.print("Signal Strength (RSSI): ");
            currentRSSI = advertisedDevice.getRSSI();
            Serial.print(currentRSSI);
            Serial.println(" dBm");

// Send RSSI value to arduino uno via UART
            MySerial.print("RSSI: ");
            MySerial.print(advertisedDevice.getRSSI());
        }

        if (advertisedDevice.haveServiceUUID()) {
            Serial.print("Service UUID: ");
            Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
        }

        if (advertisedDevice.haveManufacturerData()) {
          String manufractureDataString = advertisedDevice.getManufacturerData();
            std::string manufacturerData = std::string(manufractureDataString.c_str());
            Serial.print("Manufacturer Data: ");
            Serial.println(manufacturerData.c_str());

            // Checking if the device is an iBeacon
            if (manufacturerData.length() > 25) {
                int major = (manufacturerData[20] << 8) | manufacturerData[21];
                int minor = (manufacturerData[22] << 8) | manufacturerData[23];

                Serial.print("Major: ");
                Serial.println(major);
                Serial.print("Minor: ");
                Serial.println(minor);
            }
        }
    }
};
// Servo angle
void my_angle(int angle)
{
  switch(angle)
  {
    case 0: 
    myservo.write(0);
    delay(1000);
    closeLid();
    break;
    
    case 90:
    myservo.write(90);
    delay(1000);
    closeLid();
    break;

    case 180:
    myservo.write(180);
    delay(1000);
    closeLid();
    break;

    default:
    myservo.write(0);
    delay(1000);
    closeLid();
    break;
  }
}

void closeLid()
{
  myservo1.write(0);
  delay(200);
}

void backPos()
{
  myservo.write(0);
  myservo1.write(0);
  delay(1000);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Scanner...");

    // Intilize Serial Port (UART)
    MySerial.setTxBufferSize(SER_BUF_SIZE);

    // 
    MySerial.begin(BAURDRATE, SERIAL_8N1, TxPin, RxPin);

    // Initialize BLE
    BLEDevice::init("ESP32_BLE_Tracker");
    pBLEScan = BLEDevice::getScan(); // Create New scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); // Active scanning for better results
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    Serial.println("Scanning for BLE Beacons...");

    // Servo Pills dispenser 
    myservo.attach(3);
    myservo1.attach(8);
    myservo.write(0);
    myservo1.write(0);
    delay(1000);
}

void loop() {
  // BLE data 
    pBLEScan->start(SCAN_TIME, false);
    delay(2000); // Wait before next scan

    if (currentRSSI >= 0 && currentRSSI <= 10)
    {
      my_angle(pos0);
      delay(1000);
      my_angle(pos1);
      delay(1000);
      my_angle(pos2);
      delay(1000);
    }
}
