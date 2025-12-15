#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <FastLED.h>

#include "mxgicHall.h"
#include "mxgicRotary.h"

struct DiagnosticsContext {
  // Sensors
  MxgicHall** hall = nullptr;
  size_t hallCount = 0;
  MxgicRotary* knob = nullptr;

  // Buttons / pins
  uint8_t physicalButtonPin = 0;

  // LEDs
  CRGB* ledsScreen = nullptr;
  int ledsScreenCount = 0;
  CRGB* ledsButtons = nullptr;
  int ledsButtonsCount = 0;

  // Preferences namespace (for showing current settings)
  const char* prefsNamespace = nullptr;
};

// Registers /diag, /api/diag, and LED-test endpoints onto an existing WebServer.
void diagnosticsWebRegisterRoutes(WebServer& server, const DiagnosticsContext& ctx);
