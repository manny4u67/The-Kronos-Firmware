#pragma once

#include <Arduino.h>
#include <FastLED.h>

// Physical map for the 75-LED "screen" array is a 5x15 grid.
// See docs/ARRAY.md. Top row IDs: 1 30 31 60 61; Bottom row IDs: 15 16 45 46 75.

enum class TimerLedMode : uint8_t {
  None = 0,
  Setting = 1,
  Running = 2,
  Paused = 3,
};

enum class TimerLedColorStyle : uint8_t {
  White = 0,
  Gradient = 1,
};

// Must be called once after FastLED has been initialized.
void timerLedMeterInit(CRGB* leds, int numLeds);

// Sets how the meter chooses its base color.
// - White: always white (brightness may still vary for fractional fills)
// - Gradient: Running shows green->red gradient; Setting=blue; Paused=yellow
void timerLedMeterSetColorStyle(TimerLedColorStyle style);

void timerLedMeterClear(bool forceShow = false);

// Lit rows represent percent of time LEFT.
void timerLedMeterUpdateFromRemaining(unsigned long remainingMs, unsigned long totalMs, TimerLedMode mode);

// For timer-setting UI: fill relative to 60 minute max.
void timerLedMeterUpdateFromMinutes(int minutesSelected);
