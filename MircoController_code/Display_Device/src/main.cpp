#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AccelStepper.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define motorPin1  16
#define motorPin2  17
#define motorPin3  18
#define motorPin4  19
AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);

const char* service_uuid_str = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const char* weight_char_uuid_str = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const char* find_cup_char_uuid_str = "abc12345-6def-7890-1234-56789abcdef0";

BLEUUID serviceUUID(service_uuid_str);
BLEUUID weightCharUUID(weight_char_uuid_str);
BLEUUID findCupCharUUID(find_cup_char_uuid_str);

BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteWeightCharacteristic = nullptr;
BLERemoteCharacteristic* pRemoteFindCupCharacteristic = nullptr;
bool connected = false;

void connectToSensorDevice() {
  Serial.println("Starting BLE scan...");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

   BLEDevice::init("Display Device");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(serviceUUID);

  // Create characteristics
  BLECharacteristic *pWeightCharacteristic = pService->createCharacteristic(weightCharUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  BLECharacteristic *pFindCupCharacteristic = pService->createCharacteristic(findCupCharUUID, BLECharacteristic::PROPERTY_WRITE);
  connectToSensorDevice();
}

void loop() {
  if (!connected) {
    connectToSensorDevice(); // Attempt to reconnect
  }
  delay(1000); // Delay to reduce power consumption
}

void updateDisplay(float weight) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print("Weight: ");
    display.print(weight);
    display.println(" g");
    display.display();
}

void updateStepper(float weight) {
    // Convert weight to stepper steps
    // This is just a placeholder; you'll need to calibrate this
    long steps = (long)(weight * 10); // Example conversion
    stepper.move(steps);
    while(stepper.distanceToGo() != 0) {
        stepper.run();
    }
}

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (isNotify) {
        float weight = atof((char*)pData);
        Serial.print("Weight Update: ");
        Serial.println(weight);
        updateDisplay(weight);
        updateStepper(weight);
    }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID)) {
            BLEDevice::getScan()->stop();
            Serial.println("Sensor device found. Connecting...");
            pClient = BLEDevice::createClient();
            pClient->connect(&advertisedDevice);
            BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
            if (pRemoteService != nullptr) {
                pRemoteWeightCharacteristic = pRemoteService->getCharacteristic(weightCharUUID);
                pRemoteFindCupCharacteristic = pRemoteService->getCharacteristic(findCupCharUUID);
                if (pRemoteWeightCharacteristic != nullptr && pRemoteFindCupCharacteristic != nullptr) {
                    pRemoteWeightCharacteristic->registerForNotify(notifyCallback);
                    connected = true;
                    Serial.println("Connected and subscribed for notifications.");
                }
            }
        }
    }
};





