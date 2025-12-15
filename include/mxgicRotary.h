#pragma once

#include <Arduino.h>
#include <AS5600.h>
// AS5600 Hall Effect Sensor
extern AS5600 as5600;

class MxgicRotary {
    private:
        int precision = 3;
        int precisionMap[6] = {128, 255, 512, 1024, 2048, 4096};

    public:
        int currentAngle;
        int previousAngle;
        uint16_t myMapValue ;
        uint16_t myMapValue2 ;
        bool rotationCheckActive;

    uint16_t readRawAngle() {
        int angle = as5600.rawAngle();
        return angle;
    }

    void setPrecision(int prec) {
        precision = prec;
        return;
    }
    uint16_t readCaliAngle () {
        int angle = as5600.rawAngle();
        angle = map(angle, 0, 4096, 0, precisionMap[precision]);
        return angle;
    }

    // This function maps the raw angle measured by the AS5600 to a range specified through function parameters
    uint16_t scanMapAngle(int myMap = 100 ,int myMap2 = 255 , int selectMap =1) {
        int angleRaw = as5600.rawAngle();
        myMapValue = map(angleRaw, 0, 4096, 0, myMap);
        myMapValue2 = map(angleRaw, 0, 4096, 0, myMap2);
        
        switch (selectMap) {
            case 1:
                return myMapValue;
                break;
            
            case 2:
                return myMapValue2;
                break;
        }
        return 0;
    }

    void startRotationCheck() {
        currentAngle = as5600.rawAngle();
        previousAngle = currentAngle;
    }
    int checkRotation() {
        currentAngle = as5600.rawAngle();
        if (currentAngle == previousAngle) {
            return 0;
        }
        else if (currentAngle > (previousAngle + 100)){
            previousAngle = currentAngle;
            return 1;
        }
        else if (currentAngle < (previousAngle - 100)){
            previousAngle = currentAngle;
            return 2;
        }
        return 0;
    }
};