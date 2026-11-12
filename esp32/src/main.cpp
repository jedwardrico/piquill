// PiQuill ESP32 firmware: placeholder that only proves the toolchain works.
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("PiQuill ESP32 starting");
}

void loop() {
  delay(1000);
}
