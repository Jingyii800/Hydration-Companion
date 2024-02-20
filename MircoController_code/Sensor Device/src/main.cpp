#include <Arduino.h>
#include <HX711.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
HX711 scale;

// Buzzer pin
const int BUZZER_PIN = 2;

// BLE UUIDs
#define SERVICE_UUID                    "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define WEIGHT_CHARACTERISTIC_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define FIND_CUP_CHARACTERISTIC_UUID    "abc12345-6def-7890-1234-56789abcdef0"

BLEServer *pServer = nullptr;
BLECharacteristic *pWeightCharacteristic = nullptr;
BLECharacteristic *pFindCupCharacteristic = nullptr;

float lastWeight = 0; // Store the last weight to detect changes

class FindCupCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string value = pCharacteristic->getValue();

    if (value == "find") { // Check if the command is to find the cup
      digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
      delay(1000); // Buzz for 1 second
      digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f); // Adjust to your scale
  scale.tare(); // Reset the scale to 0

  BLEDevice::init("Weight Sensor");
  pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pWeightCharacteristic = pService->createCharacteristic(
                            WEIGHT_CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                          );
  pWeightCharacteristic->addDescriptor(new BLE2902());

  pFindCupCharacteristic = pService->createCharacteristic(
                             FIND_CUP_CHARACTERISTIC_UUID,
                             BLECharacteristic::PROPERTY_WRITE
                           );
  pFindCupCharacteristic->setCallbacks(new FindCupCallback());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  static float previousWeight = 0; // Store the previous weight for comparison
  float weight = scale.get_units(5); // Take a 5 reading average to smooth the data

  if (previousWeight == 0) {
    previousWeight = weight; // Initialize with the first reading
  }

  // Check if the weight has decreased
  if (weight < previousWeight) {
    float weightChange = previousWeight - weight; // Calculate the decrease

    // Only send the data if the change is significant to avoid noise
    if (weightChange >= 0.05) {
      Serial.print("Water intake detected: ");
      Serial.println(weightChange, 2);
      char tempBuff[20];
      dtostrf(weightChange, 1, 2, tempBuff); // Convert float to char array
      pWeightCharacteristic->setValue(tempBuff); // Set the weight value
      pWeightCharacteristic->notify(); // Notify any connected clients
    }
  }

  previousWeight = weight; // Update the last weight for the next loop iteration
  delay(1000); // Delay between readings                 
}



