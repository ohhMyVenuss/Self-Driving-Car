#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
constexpr int XSHUT_PIN = 16;
constexpr int GPIO1_PIN = -1;

VL53L0X sensor;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("VL53L0X test");

  if (XSHUT_PIN >= 0) {
    pinMode(XSHUT_PIN, OUTPUT);
    digitalWrite(XSHUT_PIN, LOW);
    delay(10);
    digitalWrite(XSHUT_PIN, HIGH);
    delay(10);
  }

  if (GPIO1_PIN >= 0) {
    pinMode(GPIO1_PIN, INPUT);
  }

  Wire.begin(SDA_PIN, SCL_PIN);
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("Failed to detect VL53L0X");
    while (true) {
      delay(100);
    }
  }

  sensor.startContinuous(50);
}

void loop() {
  uint16_t range = sensor.readRangeContinuousMillimeters();
  if (sensor.timeoutOccurred()) {
    Serial.println("TIMEOUT");
  } else {
    Serial.print("Distance: ");
    Serial.print(range);
    Serial.println(" mm");
  }
  delay(100);
}