// Pin assignments for the display. Proposed names from the Adafruit ESP32-S3
// Feather pinout; verify against your board (see docs/WIRING.md). SCK and MOSI
// use the board's default SPI pins.
#pragma once
#include <Arduino.h>

#define EPD_CS_PIN   D10
#define EPD_DC_PIN   D9
#define EPD_RST_PIN  D6
#define EPD_BUSY_PIN D5
