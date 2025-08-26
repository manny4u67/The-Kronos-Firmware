// mxgicTimer.h - simple multi-duration countdown timer helper
#pragma once
#include <Arduino.h>

class MxgicTimer {
public:
    bool timerReady = false;
    bool timerRunning = false;
    bool timerPaused = false;
    unsigned long setTime = 0UL;   // total duration (ms)
    unsigned long endTime = 0UL;   // millis() when timer should end

    static constexpr unsigned long minutesLut[12] = {1,10,15,20,25,30,35,40,45,50,55,60};
    static constexpr unsigned long millisLut[12] = {
        60000UL, 600000UL, 900000UL, 1200000UL, 1500000UL, 1800000UL,
        2100000UL, 2400000UL, 2700000UL, 3000000UL, 3300000UL, 3600000UL
    };

    uint8_t clampIndex(int idx) const { return (idx < 0) ? 0 : (idx > 11 ? 11 : idx); }

    unsigned long minutesOption(int idx) const { return minutesLut[clampIndex(idx)]; }

    void start(int idx) {
        if (timerRunning) return;
        uint8_t i = clampIndex(idx);
        setTime = millisLut[i];
        endTime = millis() + setTime;
        timerRunning = true;
        timerPaused  = false;
        timerReady   = true;
    }

    void pause() {
        if (!timerRunning) return;
        setTime = timeLeftMillis();
        timerRunning = false;
        timerPaused = true;
    }

    void resume() {
        if (!timerPaused) return;
        endTime = millis() + setTime;
        timerRunning = true;
        timerPaused = false;
    }

    void reset() {
        timerRunning = false;
        timerPaused = false;
        setTime = 0;
        endTime = 0;
        timerReady = false;
    }

    unsigned long timeLeftMillis() const {
        if (!timerRunning && !timerPaused) return 0;
        long diff = (long)endTime - (long)millis();
        if (diff < 0) return 0;
        return (unsigned long)diff;
    }
    unsigned long timeLeftSeconds() const { return timeLeftMillis() / 1000UL; }
    unsigned long timeLeftMinutes() const { return timeLeftMillis() / 60000UL; }

    bool timeOver() const { return timeLeftMillis() == 0 && (timerRunning || timerPaused); }
};