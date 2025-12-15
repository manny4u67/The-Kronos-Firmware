#pragma once

#include <Arduino.h>

// Shared action prefix used across modules.
static constexpr const char* ACTION_TYPE_PREFIX = "TYPE:";

// Loads keybind strings from Preferences (NVS). If keys are missing, fills and stores defaults.
void keybindsLoadFromPrefs(const char* prefsNamespace, String* actions, size_t actionCount);

// Saves keybind strings to Preferences (NVS).
void keybindsSaveToPrefs(const char* prefsNamespace, const String* actions, size_t actionCount);

// Returns the form field / prefs key for a button index (e.g. "btn0").
String keybindsKeyForButton(size_t idx);

// Timer meter LED color style.
// 0 = white, 1 = gradient
uint8_t keybindsLoadMeterStyleFromPrefs(const char* prefsNamespace);
void keybindsSaveMeterStyleToPrefs(const char* prefsNamespace, uint8_t meterStyle);

// Global LED brightness used for FastLED.setBrightness().
// Range: 1..255 (0 is treated as off; we clamp to 1).
uint8_t keybindsLoadLedBrightnessFromPrefs(const char* prefsNamespace);
void keybindsSaveLedBrightnessToPrefs(const char* prefsNamespace, uint8_t brightness);
