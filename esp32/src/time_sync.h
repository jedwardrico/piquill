// Sets the clock from the internet (NTP) over Wi-Fi, then turns Wi-Fi off.
// UNTESTED on hardware: written from memory of the Arduino-ESP32 3.x WiFi/time API.
#pragma once

#include <Arduino.h>

#include "settings.h"

// Applies the timezone, and, if the clock is not already set and Wi-Fi is
// configured, joins the network and syncs. Blocks for up to timeoutMs. The
// password is never printed. Returns true if the clock is valid afterwards.
// Do this before starting BLE: the two share the radio.
bool syncTime(const Settings &settings, uint32_t timeoutMs = 15000);

// True once the clock holds a real date (after 2023).
bool clockIsSet();
