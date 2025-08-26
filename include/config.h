#pragma once
// Central configuration & constants

// I2C Pins
constexpr int PIN_SDA0 = 17; // ESP32 SDA
constexpr int PIN_SCL0 = 18; // ESP32 SCL

// LED Pins / Counts
constexpr int PIN_SCREENARRAY  = 48;
constexpr int LED_COUNT_SCREEN = 75;
constexpr int PIN_BUTTONARRAY  = 35;
constexpr int LED_COUNT_BUTTON = 6;

// Button (legacy physical)
constexpr int PIN_PHYSICAL_BUTTON = 8; // INPUT_PULLUP

// Timings
constexpr unsigned long INTERVAL_SENSOR_MS = 500; // sensor display refresh
constexpr unsigned long INTERVAL_LOGO_MS   = 100; // logo refresh

// Task stack sizes / priorities
constexpr uint16_t STACK_TASK_INPUT   = 4096;
constexpr UBaseType_t PRIO_TASK_INPUT = 1;

// Debounce / rotation
constexpr uint16_t ROTATION_HYSTERESIS = 100;

// Hall trigger threshold (raw fallback)
constexpr int HALL_RAW_TRIGGER = 11000;

// Misc
constexpr const char* BLE_DEVICE_NAME = "KRONOS V1";
constexpr const char* BLE_DEVICE_MFG  = "AddeyX";
