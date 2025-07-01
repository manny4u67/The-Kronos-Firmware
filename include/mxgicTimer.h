#include <Arduino.h>

class MxgicTimer {

    private:


    public:

    bool timerReady = 0;
    bool timerRunning = 0; // Timer is currently counting down 
    bool timerPaused = 0; // Timer is not counting down
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
        if (timerRunning == 0){
            currentTime = millis();
            setTime = timeArray[1][index];
            endTime = setTime + millis();
            timerRunning = 1;
            timerPaused = 0;
            return;
        } 
        else {
            return;
        }
            return;
    }

    void pause(){
        setTime = endTime - millis();
        timerRunning = 0;
        timerPaused = 1;
    }

    void resume(){   
        endTime = setTime + millis();
        timerRunning = 1;
        timerPaused = 0;
    }
    void reset(){
        timerReady = 1;
        timerRunning = 0;
        timerPaused = 0;

    }

    unsigned long checkTimeLeftMillis(){
        if (millis() > endTime){
            return 0;
        }
        else{
            int timeLeft = endTime - millis();
            return timeLeft;
        }   
        return 0;
    }   

    unsigned long checkTimeLeftSeconds(){
        if (millis() > endTime){
            return 0;
        }
        else{
            int timeLeft = endTime - millis();
            timeLeft = timeLeft / 1000;
            return timeLeft;
        }   
        return 0;
    }   

        unsigned long checkTimeLeftMinutes(){
        if (millis() > endTime){
            return 0;
        }
        else{
             int timeLeft = endTime - millis();
            timeLeft = timeLeft / 60000;
            return timeLeft;
            
        }   
        return 0;
    } 

    bool timeOver(){
        if (millis() > endTime){
            return 1;
        }
        else {
            return 0;
        }
    }
}; // 