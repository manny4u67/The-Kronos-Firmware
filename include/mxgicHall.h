#include <Arduino.h>
#include <Adafruit_ADS1X15.h>

// ADS1115 
Adafruit_ADS1115 ads1;  // First ADS1115 at 0x49
Adafruit_ADS1115 ads2;  // Second ADS1115 at 0x48

  #if __has_include (<AS5600.h>)
    const bool USE_AS5600 = 1;
  #else
    const bool USE_AS5600 = 0;
  #endif

class MxgicHall {
  private:
    unsigned int minVal=64000; // Store the min value
    unsigned int maxVal=0; // Store the max value
    //int calibrationtime; // Variable to store the calibration time
    
  public:
    unsigned int precisionTable[6] = {128, 256, 512, 1024, 2048, 4096};
    unsigned int currentVal; // Store the current value
    int precision = 1; // Set Hall Pecision 1 = 128 2 = 256 
    int trigPoint = 0;
    int adcSel;
    int adcCh;
    bool calibrated = 0;
    
    void setChannel (int adcSelect , int adcChannel) {
      adcSel = adcSelect;
      adcCh = adcChannel;
    }

    unsigned int rawRead() {
      switch (adcSel) {
        case 1:
          switch(adcCh) {
            case 0:
              currentVal = ads1.readADC_SingleEnded(0);
              break;
            case 1:
              currentVal = ads1.readADC_SingleEnded(1);
              break;
            case 2:
              currentVal = ads1.readADC_SingleEnded(2);
              break;
            case 3:
              currentVal = ads1.readADC_SingleEnded(3);
              break;
          }
          break;
        case 2:
          switch (adcCh) {
            case 0:
              currentVal = ads2.readADC_SingleEnded(0);
              break;
            case 1:
              currentVal = ads2.readADC_SingleEnded(1);
              break;
            case 2:
              currentVal = ads2.readADC_SingleEnded(2);
              break;
            case 3:
              currentVal = ads2.readADC_SingleEnded(3);
              break;
          }
          break;
      }
      return currentVal;
    }
    
    int cali() {
        currentVal = rawRead();
        if (currentVal < minVal) {
          minVal = currentVal;
        }
        if (currentVal > maxVal) {
          maxVal = currentVal;
        }
        return currentVal;
        Serial.println(currentVal);
    }
    
    int caliRead() {
      int mapMax;
      mapMax = precisionTable[precision];
      currentVal = rawRead(); // -10 to get min closer to 0 when mapped
      currentVal = constrain(currentVal, minVal, maxVal);
      currentVal = map (currentVal, minVal, maxVal, 0, mapMax);
      return currentVal;
    }
    
    void setPrecision(int precision = 4) {
      trigPoint = (precisionTable[precision]) / 2;
    }
    bool checkTrig(int option) {
      switch (option) {
        case 0:
          return cali() > 11000;
          break;
        case 1:
          return caliRead() > trigPoint;
          break;
        default:
          return false;
          break;
      }
    }
    
    unsigned int getMin() {
      return minVal;
    }
    
    unsigned int getMax() {
      return maxVal;
    }
};