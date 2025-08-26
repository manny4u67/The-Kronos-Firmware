// ledMation.h - LED indexing helpers
#pragma once
#include <Arduino.h>

class LedMation {
public:
    int ledIndex = 0;
    int ledChosen = 0;
    static constexpr int linearCircleArray[36] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,45,46,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,31,30};

    void setArray(int /*vertical*/, int /*horizontal*/) { /* reserved for future */ }

    int linearCircle (int advance = 0) {
        ledIndex = (ledIndex + advance) % 36;
        if (ledIndex < 0) ledIndex += 36; // ensure positive
        ledChosen = linearCircleArray[ledIndex];
        return ledChosen;
    }
};