#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <FastLED.h>
#include <AS5600.h>
#include <BleKeyboard.h>
#include <EEPROM.h>
#include "config.h"
#include "kronosDisplay.h"
#include "mxgicHall.h"
#include "mxgicDebounce.h"
#include "mxgicRotary.h"
#include "ledMation.h"
#include "mxgicTimer.h"
#include "DisplayController.h"
#include "mysecret.h" // where hidden variables can be placed
//#include "ledMation.h"

// Provide storage for globals declared 'extern' in headers
Adafruit_ADS1115 ads1; // 0x48
Adafruit_ADS1115 ads2; // 0x49
AS5600 as5600;

// Global display text storage (declared extern in kronosDisplay.h)
String displayText;

// EEPROM PROGRAMMING FOR STATE MEMORY (future use)
#define EEPROM_SIZE 2
int testEEPROM = 150; // currently unused

// BLE Keyboard 
BleKeyboard bleKeyboard("KRONOS V1", "AddeyX", 100);

// I2C pins now sourced from config.h (PIN_SDA0 / PIN_SCL0)

// OLED Display 
#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ARGB LEDS (pins and counts from config.h)
CRGB leds_75[LED_COUNT_SCREEN];
CRGB leds_6[LED_COUNT_BUTTON];

// Timer Configurations
long duration;

// Mxgic Timer Class
MxgicTimer maintimer;

// Delay Control (Function Timer)
unsigned long previousMillis2 = 0UL;
unsigned long currentMillis2 = 0UL; 
const long interval2 = 100UL; //Adjust this to change delay time 

unsigned long previousMillis3 = 0UL;
unsigned long currentMillis3 = 0UL; 
const long interval3 = 500UL; //Adjust this to change delay time

// Hall Effect Buttons Objects
MxgicHall LTBTN , RTBTN, LMBTN, RMBTN, LBBTN, RBBTN;

MxgicHall* hall[] = {&LTBTN, &RTBTN, &LMBTN, &RMBTN, &LBBTN, &RBBTN}; //Object array using pointers

// Debounce Objects
MxgicDebounce LTBTND, RTBTND, LMBTND, RMBTND, LBBTND, RBBTND;

MxgicDebounce* debounceHall[] = {&LTBTND, &RTBTND, &LMBTND, &RMBTND, &LBBTND, &RBBTND}; //Object array using pointers

MxgicDebounce buttonG;

// Hall Effect Rotary Encoder
MxgicRotary hallKnob;
uint16_t angleHall;

// For enabling Scan
bool scanEnabled = 0;

// Calibration Global Variables
bool initializedK = 0;

// ARGB animation objects
LedMation mainArray;
int i;

// AUDIO LEVEL GRAPH
uint8_t pos = 0;
bool toggle = false;

// Task Function
void infiniteScan(void * parameters) {
  for(;;) {
    if(initializedK){
  if(debounceHall[0]->update(LTBTN.checkTrig(0))){
        String input = PhantomPass;

        for (int i = 0; i < input.length(); i++) {
          bleKeyboard.print(input[i]);
          bleKeyboard.releaseAll();
          delay(10);
        }
      }
  else if (debounceHall[1]->update(RTBTN.checkTrig(0))){
        bleKeyboard.press(KEY_LEFT_GUI);
        bleKeyboard.press(KEY_NUM_MINUS);
      }
  else if (debounceHall[2]->update(LMBTN.checkTrig(0))){
        bleKeyboard.press(KEY_LEFT_CTRL);
        bleKeyboard.press('z');
      }
  else if (debounceHall[3]->update(RMBTN.checkTrig(0))){
        bleKeyboard.press(KEY_LEFT_CTRL);
        bleKeyboard.press(KEY_LEFT_SHIFT);
        bleKeyboard.press('z');       
      }
  else if (debounceHall[4]->update(LBBTN.checkTrig(0))){
        String input = MainSolWallet;
        for (int i = 0; i < input.length(); i++) {
          bleKeyboard.print(input[i]);
          bleKeyboard.releaseAll();
          delay(10);
        }
      }
  else if (debounceHall[5]->update(RBBTN.checkTrig(0))){
        bleKeyboard.press(KEY_DELETE);
      }
      bleKeyboard.releaseAll();
      //if (buttonG.debounce(digitalRead(8) == 1)) {
      //  bleKeyboard.print("LOVE YOU!");
      //}
    }
    vTaskDelay(2/portTICK_PERIOD_MS);
  }
}

void ledCycle(CRGB* leds, int numLeds) {
  for(int i = 0; i < numLeds; i++) {
    delay(35);
    leds[i] = CHSV(255, 0, 255);
    FastLED.show();
  }
}

void ledClear(CRGB* leds, int numLeds) {
  for(int i = 0; i < numLeds; i++) {
    leds[i] = CRGB::Black;
    FastLED.show();
  }
}

void solidColor(CRGB* leds, int numLeds, CRGB color) {
  for(int i = 0; i < numLeds; i++) {
    leds[i] = color;
    delay(250);
  }
  FastLED.show();
}

void solidColor2(CRGB* leds, int numLeds, CRGB color) {
  for(int i = 0; i < numLeds; i++) {
    leds[i] = color;
    delay(1);
  }
  FastLED.show();
}
void ledFadeUp(CRGB* leds, int numLEDS , CRGB color){
  for (int i = 0; i < numLEDS; i++){
    leds[i] = color;
  }
  FastLED.show();
  FastLED.setBrightness(hallKnob.scanMapAngle());
}
// screenRender deprecated; using DisplayController abstraction now.

// Initialize Calibration for Kronos (Only when EEPROM is 0)
void initializeKronos(){
  int numBtns = 6;
  int startTimer = 0;
  int timerSet = 500;
  bool calibrationComplete = false;
  bool arrayCalibrationComplete = false;
  int currentHall = 0;

  while(currentHall <= (numBtns-1)) {
    calibrationComplete = 0;
    while(!calibrationComplete) {
      while(hall[currentHall]->checkTrig(0) == 0) {
  displayText = "PRESS BTN" + String(currentHall+1);
      }  
      startTimer = millis();
      arrayCalibrationComplete = 0;
      while((hall[currentHall]->checkTrig(0) == 1) && (!arrayCalibrationComplete)) {
        if ((startTimer + (timerSet/2)) > (millis())) {
          displayText = "FULLY PRESS";
        }
        else if ((startTimer + (timerSet/2)+(timerSet / 4)) > millis()) {
          displayText = "CALIBRATING";
        }
        else if ((startTimer + timerSet) < millis()) {
          displayText = "HALL " + String(currentHall+1) + "CHK";
          delay(500);
          arrayCalibrationComplete = 1;
          calibrationComplete = 1;
          currentHall++;
        }
      }
    }
  }
}

bool timerMenuKeyScan(int TimerMenu) {
  if (LTBTN.checkTrig(0)){
        return 1;
      }
      if (RTBTN.checkTrig(0)){
        return 1;
      }
      if((digitalRead(8) == 0)&&(TimerMenu > 1000)){ 
        return 1;
      }
}
bool timerMenu (){
  unsigned long menuEnteredTime = millis();
  unsigned long menuNowTime = 0UL;
  unsigned long menuTimeDiff = 0UL;
  int timePrint;
  initializedK = 0;
  while (true){
    menuNowTime = millis();
    menuTimeDiff = menuNowTime - menuEnteredTime;
    if (maintimer.timerRunning == 1 ) {
  // Timer running display handled in loop via DisplayController
      // Check which buttons has been pressed
      timerMenuKeyScan(menuTimeDiff);
    }
    else if (maintimer.timerRunning == 0){
  timePrint = maintimer.minutesOption(hallKnob.scanMapAngle(12,255,1)); // Set Timer Display Based On Rotary Encoder
      if (LTBTN.checkTrig(0)){ // Check if left button was pressed
  maintimer.start(hallKnob.scanMapAngle(12,255,1)); // Start time based on rotary encoder location
        delay (500);
        return 1;
      }
      else if (RTBTN.checkTrig(0)){
        return 0;
      }
    }
    delay (25);
  }
}

bool timerEnd(){
  DisplayController disp(oled, maintimer);
  disp.showTimerOver();
  solidColor2(leds_75, LED_COUNT_SCREEN , CRGB::Red);
  delay(1000);
  while(true){
  disp.showTimerOver();
    if (LTBTN.checkTrig(0)){ // Check if left button was pressed 
  solidColor2(leds_75, LED_COUNT_SCREEN , CRGB::Black);
      maintimer.reset();
      return 1;
    }
    else if (RTBTN.checkTrig(0)){
  solidColor2(leds_75, LED_COUNT_SCREEN , CRGB::Black);
      maintimer.reset();
      timerMenu();
      return 1;
    }
    delay (25);
  }
}

bool timerPause(){
  while(true){
    
  }
}

void audioLevelGraph() {
  while(1){
    leds_75[pos] = CHSV(150, 255, 255);
    // Blur the entire strip
  blur1d(leds_75, LED_COUNT_SCREEN, 172);
  fadeToBlackBy(leds_75, LED_COUNT_SCREEN, 4);
    FastLED.show();
    // Move the position of the dot
    if (toggle) {
  pos = (pos + 1) % LED_COUNT_SCREEN;
    }
    toggle = !toggle;
    delay(20);
    if (LTBTN.checkTrig(0)){
        return; // Exit the function if left button is pressed
    }
  }  // Add a bright pixel that moves
}
void setup() {
  // Initialize Serial Communication
  Serial.begin(115200);

  // Initialize EEPROM with predefine size
  EEPROM.begin(EEPROM_SIZE);

  // Initializing BLE Keyboard
  bleKeyboard.begin();

  // Initializing Button
  pinMode(8, INPUT_PULLUP);

  // Initializing Hall Objects
  LTBTN.setChannel(1,0);
  RTBTN.setChannel(2,0);
  LMBTN.setChannel(1,1);
  RMBTN.setChannel(2,1);
  LBBTN.setChannel(1,2);
  RBBTN.setChannel(2,2);

  // Initialize I2C
  Wire.begin(PIN_SCL0, PIN_SDA0 );

  // Initialize OLED Display
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // Initialize ADS1115 Modules
  if (!ads1.begin(0x48)) {
    Serial.println("Failed to initialize ADS1115 at 0x48");
    for(;;);
  }

  if (!ads2.begin(0x49)) {
    Serial.println("Failed to initialize ADS1115 at 0x49");
    for(;;);
  }

  // Initialize AS5600 Hall Effect Sensor
  as5600.begin();

  // Initialize LED Strips
  FastLED.addLeds<WS2812B, PIN_SCREENARRAY, GRB>(leds_75, LED_COUNT_SCREEN);
  FastLED.addLeds<WS2812B, PIN_BUTTONARRAY, GRB>(leds_6, LED_COUNT_BUTTON);
  FastLED.setBrightness(25);

  // LED Strip Animation
  ledCycle(leds_75, LED_COUNT_SCREEN);
  DisplayController disp(oled, maintimer);
  disp.showBootBlank(myBootLogo);
  delay(500);
  disp.showBootLogo(myLogo);
  delay(500);

  // Set Precision variables for all 6 hall effect sensors
  int setPrecisionTemp = 3;
  for(int i= 1; i< 6; i++) { // 6 Hall Effect Sensors
    hall[i]->setPrecision(setPrecisionTemp);
  }

  // Set Precision variable for hallKnob
  hallKnob.setPrecision(5);

  /* 
  FIX EEPROM MEMORY ROUTINE , IT IS WHT IS CAUSING THE CRASHES
 



  // Sets EEPROM to 0, for use after first programming
  if (LBBTN.checkTrig(0)) {
    EEPROM.write(0,0); // Write to EEPROM (FLASH)
    EEPROM.commit();  // Actually Write to EEPROM (FLASH)
  }

  // Checks for prior initialization
  if (RBBTN.checkTrig(0)) {
    initializeKronos();
    initializedK = 1;
    scanEnabled = 1;
    EEPROM.write(0,initializedK); // Write to EEPROM (FLASH)
    EEPROM.commit();  // Actually Write to EEPROM (FLASH)
  }

  initializedK = EEPROM.read(0);

  if(!initializedK){ //Run Initialization Calibration Code
    initializeKronos();
    initializedK = 1;
    scanEnabled = 1;
    EEPROM.write(0,initializedK); // Write to EEPROM (FLASH)
    EEPROM.commit();  // Actually Write to EEPROM (FLASH)
  }
  else {
    scanEnabled = 1;
    Serial.println("Already Initialized");
  }

   */
  if (LTBTN.checkTrig(0)){
        initializeKronos();
  }
  initializedK = 1;
  scanEnabled = 1;

  // Clear Display
  ledClear(leds_75, LED_COUNT_SCREEN);
  FastLED.show();

  audioLevelGraph(); // Run Audio Level Graph
  //Create
  xTaskCreate(
    infiniteScan, // function name
    "infiniteScan", // task name
    4096, // stack size
    NULL, // task parameters
    1, // task priority
    NULL // task handle
  );
}


void loop() {
  long start = micros(); 
  static DisplayController disp(oled, maintimer);
  if(digitalRead(8) == 1) { // if button is not pressed
    if (maintimer.timeOver() && (maintimer.timerRunning == 1) ){
      timerEnd();
    } else {
      currentMillis3 = millis();
      if (currentMillis3 - previousMillis3 >= interval3) {
        angleHall = hallKnob.readRawAngle();
        disp.showSensors(angleHall, hall);
        previousMillis3 = currentMillis3;
      }
    }
  }
  else { // if button is pressed
    timerMenu();
    delay(200);
    initializedK = 1; // re-initializing Kronos to allow for infinite scan
  }
  duration = micros() - start;

  delay(50);
}

