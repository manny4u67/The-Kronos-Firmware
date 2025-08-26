// mxgicHall.h - Hall sensor abstraction around two ADS1115 devices
// Refactored: removed unreachable code, added bounds checks, const-correctness

#pragma once
#include <Arduino.h>
#include <Adafruit_ADS1X15.h>

// External ADS1115 instances (declared in main translation unit)
extern Adafruit_ADS1115 ads1; // addr 0x48
extern Adafruit_ADS1115 ads2; // addr 0x49

class MxgicHall {
private:
  uint16_t minVal = 0xFFFF; // start high so first sample lowers it
  uint16_t maxVal = 0;      // start low so first sample raises it

public:
  static constexpr uint16_t precisionTable[6] = {128, 256, 512, 1024, 2048, 4096};

  uint16_t currentVal = 0;
  uint8_t precision = 1;   // index into precisionTable
  uint16_t trigPoint = 0;   // mid threshold after precision set
  uint8_t adcSel = 1;       // 1 or 2 (which ADS1115)
  uint8_t adcCh = 0;        // 0..3 channel
  bool calibrated = false;

  void setChannel(uint8_t adcSelect, uint8_t adcChannel) {
    adcSel = (adcSelect < 1) ? 1 : (adcSelect > 2 ? 2 : adcSelect);
    adcCh = adcChannel & 0x03; // limit to 0..3
  }

  uint16_t rawRead() {
    Adafruit_ADS1115 *dev = (adcSel == 1) ? &ads1 : &ads2;
    currentVal = dev->readADC_SingleEnded(adcCh);
    return currentVal;
  }

  uint16_t cali() {
    rawRead();
    if (currentVal < minVal) minVal = currentVal;
    if (currentVal > maxVal) maxVal = currentVal;
    return currentVal;
  }

  uint16_t caliRead() {
    // Avoid divide-by-zero if not yet calibrated
    if (minVal >= maxVal) {
      cali(); // take at least one sample
      return 0;
    }
    uint16_t mapMax = precisionTable[precision];
    rawRead();
    uint16_t clamped = constrain(currentVal, minVal, maxVal);
    currentVal = map(clamped, minVal, maxVal, 0, mapMax);
    return currentVal;
  }

  void setPrecision(uint8_t p = 4) {
    if (p > 5) p = 5;
    precision = p;
    trigPoint = precisionTable[precision] / 2;
  }

  bool checkTrig(uint8_t option) {
    switch(option) {
      case 0: return cali() > 11000; // raw trigger threshold
      case 1: return caliRead() > trigPoint;
      default: return false;
    }
  }

  uint16_t getMin() const { return minVal; }
  uint16_t getMax() const { return maxVal; }
};