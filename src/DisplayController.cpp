#include "DisplayController.h"
#include <Arduino.h>
#include "mxgicHall.h"
#include "mxgicRotary.h"

void DisplayController::showBootBlank(const unsigned char* bmp){
  oled.clearDisplay();
  oled.drawBitmap(0,0,bmp,128,64,WHITE);
  oled.display();
}

void DisplayController::showBootLogo(const unsigned char* bmp){
  oled.clearDisplay();
  oled.drawBitmap(0,0,bmp,128,64,WHITE);
  oled.display();
}

void DisplayController::showSensors(uint16_t angle, MxgicHall* halls[6]){
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0);
  oled.print(F("Sensor Readings:"));
  oled.print(F("Hall Angle: ")); oled.println(angle);
  oled.print(F("LT: ")); oled.print(halls[0]->currentVal); oled.print(F(" RT: ")); oled.println(halls[1]->currentVal);
  oled.print(F("LM: ")); oled.print(halls[2]->currentVal); oled.print(F(" RM: ")); oled.println(halls[3]->currentVal);
  oled.print(F("LB: ")); oled.print(halls[4]->currentVal); oled.print(F(" RB: ")); oled.println(halls[5]->currentVal);
  oled.display();
}

void DisplayController::showCalibrationLive(const String& title, MxgicHall* hall, uint8_t index){
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0,0);
  oled.println(title);
  oled.println(hall->cali());
  oled.display();
}

void DisplayController::showCalibrationSummary(const String& title, MxgicHall* hall, uint8_t index){
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0,0);
  oled.println(title);
  oled.println(String("Max:") + hall->getMax());
  oled.print(String("Min:") + hall->getMin());
  oled.display();
}

void DisplayController::showLayer(const unsigned char* bmp, const char* layerName){
  oled.clearDisplay();
  oled.drawBitmap(0,0,bmp,128,64,WHITE);
  oled.setCursor(0,0);
  oled.setTextSize(1);
  oled.print(layerName);
  oled.display();
}

void DisplayController::showTimerSet(int minutesOption){
  oled.clearDisplay();
  oled.setTextSize(4);
  oled.setCursor(0,0);
  oled.println(String(minutesOption) + F(" M"));
  oled.setTextSize(1);
  oled.println();
  oled.println(F("Confirm       Cancel"));
  oled.display();
}

void DisplayController::showTimerRunning(){
  uint32_t secs = timer.timeLeftSeconds();
  uint32_t mins = secs / 60;
  uint32_t rem  = secs % 60;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0);
  oled.println(F("Time Left:"));
  oled.setTextSize(3);
  oled.print(String(mins));
  oled.setTextSize(1); oled.print(F("M "));
  oled.setTextSize(3); oled.print(String(rem));
  oled.setTextSize(1); oled.print(F("S"));
  oled.println();
  oled.println(F("Pause       Cancel"));
  oled.display();
}

void DisplayController::showTimerOver(){
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0);
  oled.println(String(timer.setTime/60000UL) + F("M Timer Over:"));
  oled.setTextSize(4);
  oled.println(F("REST"));
  oled.setTextSize(1);
  oled.println(F("OK       Go Again"));
  oled.display();
}
