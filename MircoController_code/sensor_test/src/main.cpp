#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 5;
const int LOADCELL_SCK_PIN = 2;

HX711 scale;

// Define the number of readings to include in the moving average
const int numReadings = 5;
float readings[numReadings]; // the readings from the load cell
int readIndex = 0; // the index of the current reading
float total = 0; // the running total
float average = 0; // the average

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f);
  scale.tare();
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

void loop() {
  if (scale.is_ready()) {
    total = total - readings[readIndex];
    readings[readIndex] = scale.get_units();
    total = total + readings[readIndex];
    readIndex = readIndex + 1;

    if (readIndex >= numReadings) {
      readIndex = 0;
    }

    average = total / numReadings;
    
    Serial.print("Average weight: ");
    Serial.print(average, 2);
    Serial.println(" g");
  } else {
    Serial.println("HX711 not found.");
  }

  delay(500);
}

