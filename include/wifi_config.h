#pragma once

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#include "diagnostics_web.h"

// Starts the WiFi AP + HTTP portal and blocks forever until the device reboots.
// - ssid: AP SSID (open AP)
// - prefsNamespace: Preferences namespace used for keybind storage
// - oled: display used for the "WiFi Config Mode" screen
// - actions: array of action strings (btn0..btnN)
// - actionCount: number of actions
void startWifiConfigPortal(const char* ssid,
                           const char* prefsNamespace,
                           Adafruit_SSD1306& oled,
                           String* actions,
                           size_t actionCount,
                           const DiagnosticsContext* diagCtx = nullptr);
