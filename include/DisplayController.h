#pragma once
#include <Adafruit_SSD1306.h>
#include "mxgicTimer.h"
#include "kronosDisplay.h"
#include "config.h"

// Forward declarations
class MxgicHall;
class MxgicRotary;

enum class ScreenId : uint8_t {
  BootBlank = 1,
  BootLogo  = 2,
  Sensor    = 3,
  CalLive   = 4,
  CalSummary= 5,
  Layer     = 6,
  TimerSet  = 7,
  TimerRun  = 9,
  TimerOver = 10
};

class DisplayController {
public:
  DisplayController(Adafruit_SSD1306& d, MxgicTimer& t): oled(d), timer(t) {}

  void showBootBlank(const unsigned char* bmp);
  void showBootLogo(const unsigned char* bmp);
  void showSensors(uint16_t angle, MxgicHall* halls[6]);
  void showCalibrationLive(const String& title, MxgicHall* hall, uint8_t index);
  void showCalibrationSummary(const String& title, MxgicHall* hall, uint8_t index);
  void showLayer(const unsigned char* bmp, const char* layerName);
  void showTimerSet(int minutesOption);
  void showTimerRunning();
  void showTimerOver();

private:
  Adafruit_SSD1306& oled;
  MxgicTimer& timer;
};
