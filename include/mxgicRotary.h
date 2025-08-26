#include <Arduino.h>
#include <AS5600.h>
// AS5600 Hall Effect Sensor
AS5600 as5600;

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
        // mxgicRotary.h - AS5600 helper for mapped angle + coarse direction detection
        #pragma once
        #include <Arduino.h>
        #include <AS5600.h>

        extern AS5600 as5600; // declared in main

        class MxgicRotary {
        private:
            uint8_t precision = 3; // index into precisionMap
            static constexpr uint16_t precisionMap[6] = {128, 255, 512, 1024, 2048, 4096};
        public:
            uint16_t currentAngle = 0;
            uint16_t previousAngle = 0;
            uint16_t myMapValue = 0;
            uint16_t myMapValue2 = 0;

            uint16_t readRawAngle() const { return as5600.rawAngle(); }

            void setPrecision(uint8_t p) { if (p > 5) p = 5; precision = p; }

            uint16_t readCaliAngle() {
                uint16_t angle = as5600.rawAngle();
                return map(angle, 0, 4096, 0, precisionMap[precision]);
            }

            // Map the raw angle into up to two arbitrary ranges; select which to return
            uint16_t scanMapAngle(uint16_t map1Max = 100, uint16_t map2Max = 255, uint8_t selectMap = 1) {
                uint16_t angleRaw = as5600.rawAngle();
                myMapValue  = map(angleRaw, 0, 4096, 0, map1Max);
                myMapValue2 = map(angleRaw, 0, 4096, 0, map2Max);
                return (selectMap == 2) ? myMapValue2 : myMapValue;
            }

            void startRotationCheck() { previousAngle = currentAngle = as5600.rawAngle(); }

            // Return 0=no movement, 1=forward, 2=reverse when delta exceeds hysteresis
            uint8_t checkRotation(uint16_t hysteresis = 100) {
                currentAngle = as5600.rawAngle();
                if (currentAngle > previousAngle + hysteresis) { previousAngle = currentAngle; return 1; }
                if (currentAngle + hysteresis < previousAngle) { previousAngle = currentAngle; return 2; }
                return 0;
            }
        };