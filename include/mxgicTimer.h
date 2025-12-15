#include <Arduino.h>

class MxgicTimer {

    private:


    public:

    bool timerReady = 0;
    bool timerRunning = 0; // Timer is currently counting down 
    bool timerPaused = 0; // Timer is not counting down
    unsigned long setMinutes = 0UL;
    unsigned long totalDurationMs = 0UL;
    unsigned long setTime = 0UL;
    unsigned long endTime = 0UL;
    unsigned long currentTime = 0UL;
    unsigned long timeArray [2][12] = {
    {1UL, 10UL, 15UL, 20UL, 25UL, 30UL, 35UL, 40UL, 45UL, 50UL, 55UL, 60UL},
    {60000UL, 600000UL, 900000UL, 1200000UL, 1500000UL, 1800000UL, 2100000UL, 2400000UL, 2700000UL, 3000000UL, 3300000UL, 3600000UL}
    };

    
    int setCheck(int index){
        index = index * 1;
        index = constrain(index , 0 , 11);
        return timeArray[0][index];
    }

    void start(int index){
        if (timerRunning != 0) {
            return;
        }

        index = constrain(index, 0, 11);
        currentTime = millis();
        setMinutes = timeArray[0][index];
        totalDurationMs = timeArray[1][index];
        setTime = totalDurationMs;
        endTime = millis() + totalDurationMs;
        timerRunning = 1;
        timerPaused = 0;
    }

    void pause(){
        if (!timerRunning) {
            return;
        }
        setTime = checkTimeLeftMillis();
        timerRunning = 0;
        timerPaused = 1;
    }

    void resume(){   
        if (!timerPaused) {
            return;
        }
        endTime = millis() + setTime;
        timerRunning = 1;
        timerPaused = 0;
    }

    void cancel(){
        reset();
    }
    void reset(){
        timerReady = 1;
        timerRunning = 0;
        timerPaused = 0;
        setMinutes = 0UL;
        totalDurationMs = 0UL;
        setTime = 0UL;
        endTime = 0UL;
        currentTime = 0UL;

    }

    unsigned long checkTimeLeftMillis(){
        if (timerPaused) {
            return setTime;
        }
        if (!timerRunning) {
            return 0;
        }
        if (millis() >= endTime){
            return 0;
        }
        return (unsigned long)(endTime - millis());
    }   

    unsigned long checkTimeLeftSeconds(){
        const unsigned long ms = checkTimeLeftMillis();
        return ms / 1000UL;
    }   

    unsigned long checkTimeLeftMinutes(){
        const unsigned long ms = checkTimeLeftMillis();
        return ms / 60000UL;
    }

    bool timeOver(){
        if (!timerRunning) {
            return 0;
        }
        return millis() >= endTime;
    }
}; // 