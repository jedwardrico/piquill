// Pin assignments for the display, as raw ESP32-S3 GPIO numbers.
//
// Taken from the Unexpected Maker FeatherS3[D] pinout sheet. The number printed
// on the board next to a header pin is the GPIO number (e.g. "7" = GPIO7).
// Do NOT use the Arduino names (D5, D10, A0...): those are not GPIO numbers
// (D10 is GPIO3, a strapping pin). SCK/MOSI are GPIO36/GPIO35, the board's
// default SPI pins, so the driver's SPI.begin() picks them up.
//
// Avoid: GPIO0 and GPIO3 (strapping), GPIO19/20 (USB), GPIO43/44 (UART0, left
// free for debug), GPIO8/9 (battery fuel gauge I2C), GPIO39/40/41/13/4/34
// (used on the board: LDO2 enable, RGB LED, antenna select, LED, light sensor,
// 5 V detect).
#pragma once
#include <Arduino.h>

#define EPD_CS_PIN   1   // header "1"
#define EPD_DC_PIN   7   // header "7"
#define EPD_RST_PIN  10  // header "10"
#define EPD_BUSY_PIN 11  // header "11"

// Enable pin for the 5 V boost converter feeding the keyboard (header "6").
#define BOOST_EN_PIN 6
