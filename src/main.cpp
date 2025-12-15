#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <FastLED.h>
#include <AS5600.h>
#include <BleKeyboard.h>
#include <EEPROM.h>
#include "kronosDisplay.h"
#include "mxgicHall.h"
#include "mxgicDebounce.h"
#include "mxgicRotary.h"
#include "ledMation.h"
#include "mxgicTimer.h"
#include "mysecret.h" // where hidden variables can be placed
#include "keybinds.h"
#include "wifi_config.h"
#include "timer_led_meter.h"
//#include "ledMation.h"

// EEPROM PROGRAMMING FOR STATE MEMORY
static constexpr int EEPROM_SIZE = 2;

// BLE Keyboard 
BleKeyboard bleKeyboard("KRONOS V1", "AddeyX", 100);

//I2C 
// ESP32 `Wire.begin(sda, scl)`
// These values preserve the previously-working wiring/behavior.
static constexpr uint8_t SDA0_Pin = 18;   // ESP32 SDA PIN
static constexpr uint8_t SCL0_Pin = 17;   // ESP32 SCL PIN
static constexpr uint8_t BUTTON_PIN = 8;

// OLED Display 
static constexpr uint16_t SCREEN_WIDTH = 128; // OLED width,  in pixels
static constexpr uint16_t SCREEN_HEIGHT = 64; // OLED height, in pixels
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ARGB LEDS
static constexpr uint8_t SCREENARRAY = 48;
static constexpr int NUM_LEDS_SCREENARRAY = 75;
CRGB leds_75[NUM_LEDS_SCREENARRAY];

static constexpr uint8_t BUTTONARRAY = 35;
static constexpr int NUM_LEDS_BUTTONARRAY = 6;
CRGB leds_6[NUM_LEDS_BUTTONARRAY];

static constexpr size_t HALL_BUTTON_COUNT = 6;

// Timer Configurations
unsigned long duration = 0;

// Mxgic Timer Class
MxgicTimer maintimer;

// Delay Control (Function Timer)
unsigned long previousMillis2 = 0UL;
unsigned long currentMillis2 = 0UL; 
static constexpr unsigned long interval2 = 100UL; // Adjust this to change delay time

unsigned long previousMillis3 = 0UL;
unsigned long currentMillis3 = 0UL; 
static constexpr unsigned long interval3 = 500UL; // Adjust this to change delay time

// Hall Effect Buttons Objects
MxgicHall LTBTN , RTBTN, LMBTN, RMBTN, LBBTN, RBBTN;

MxgicHall* hall[HALL_BUTTON_COUNT] = {&LTBTN, &RTBTN, &LMBTN, &RMBTN, &LBBTN, &RBBTN}; // Object array using pointers

// Debounce Objects
MxgicDebounce LTBTND, RTBTND, LMBTND, RMBTND, LBBTND, RBBTND;

MxgicDebounce* debounceHall[HALL_BUTTON_COUNT] = {&LTBTND, &RTBTND, &LMBTND, &RMBTND, &LBBTND, &RBBTND}; // Object array using pointers

MxgicDebounce buttonG;

// Hall Effect Rotary Encoder
MxgicRotary hallKnob;
uint16_t angleHall = 0;

// For enabling Scan
bool scanEnabled = false;

// Calibration Global Variables
bool initializedK = false;

// ARGB animation objects
LedMation mainArray;

// AUDIO LEVEL GRAPH
uint8_t pos = 0;
bool toggle = false;

// ----------------------------
// WiFi Keybind Configuration
// ----------------------------

static void bleTypeString(const String& input);

static constexpr const char* WIFI_AP_SSID = "KRONOS-CONFIG";
static constexpr const char* PREFS_NAMESPACE = "kronos";

static uint8_t g_ledBrightness = 20;

// Which hall sensor is used to enter WiFi config mode at boot.
// Pick one you consider "unused" in your normal workflow.
// 0..5 maps to: LT, RT, LM, RM, LB, RB
static constexpr size_t WIFI_CONFIG_HALL_INDEX = 4; // LB by default
static constexpr uint16_t WIFI_CONFIG_HOLD_MS = 500;

static uint8_t applyBrightnessCap(uint8_t requested) {
  // Treat the web-configured brightness as a global cap.
  // If other parts of the firmware want dynamic brightness (e.g. knob-based),
  // they can request a value and we'll scale it into the capped range.
  //
  // - requested: 0..255 (FastLED brightness)
  // - g_ledBrightness: 1..255 (cap)
  const uint16_t scaled = (uint16_t)requested * (uint16_t)g_ledBrightness / 255U;
  const uint8_t out = (scaled < 1U) ? 1U : (uint8_t)scaled;
  FastLED.setBrightness(out);
  return out;
}

static bool hallHeldForMs(MxgicHall& hallBtn, uint16_t holdMs) {
  const uint32_t start = millis();
  while ((uint32_t)(millis() - start) < holdMs) {
    if (hallBtn.checkTrig(0) == 0) {
      return false;
    }
    delay(1);
  }
  return true;
}

String hallActions[HALL_BUTTON_COUNT];

static bool pressKeyToken(const String& tokenUpper) {
  if (tokenUpper.length() == 0) {
    return false;
  }

  if (tokenUpper == F("CTRL") || tokenUpper == F("CONTROL")) { bleKeyboard.press(KEY_LEFT_CTRL); return true; }
  if (tokenUpper == F("SHIFT")) { bleKeyboard.press(KEY_LEFT_SHIFT); return true; }
  if (tokenUpper == F("ALT")) { bleKeyboard.press(KEY_LEFT_ALT); return true; }
  if (tokenUpper == F("GUI") || tokenUpper == F("WIN") || tokenUpper == F("CMD")) { bleKeyboard.press(KEY_LEFT_GUI); return true; }

  if (tokenUpper == F("ENTER") || tokenUpper == F("RETURN")) { bleKeyboard.press(KEY_RETURN); return true; }
  if (tokenUpper == F("TAB")) { bleKeyboard.press(KEY_TAB); return true; }
  if (tokenUpper == F("ESC") || tokenUpper == F("ESCAPE")) { bleKeyboard.press(KEY_ESC); return true; }
  if (tokenUpper == F("BACKSPACE") || tokenUpper == F("BKSP")) { bleKeyboard.press(KEY_BACKSPACE); return true; }
  if (tokenUpper == F("DELETE") || tokenUpper == F("DEL")) { bleKeyboard.press(KEY_DELETE); return true; }
  if (tokenUpper == F("SPACE")) { bleKeyboard.press(' '); return true; }

  if (tokenUpper == F("UP")) { bleKeyboard.press(KEY_UP_ARROW); return true; }
  if (tokenUpper == F("DOWN")) { bleKeyboard.press(KEY_DOWN_ARROW); return true; }
  if (tokenUpper == F("LEFT")) { bleKeyboard.press(KEY_LEFT_ARROW); return true; }
  if (tokenUpper == F("RIGHT")) { bleKeyboard.press(KEY_RIGHT_ARROW); return true; }

  if (tokenUpper == F("NUM_MINUS")) { bleKeyboard.press(KEY_NUM_MINUS); return true; }
  if (tokenUpper == F("NUM_PLUS")) { bleKeyboard.press(KEY_NUM_PLUS); return true; }
  if (tokenUpper == F("NUM_ENTER")) { bleKeyboard.press(KEY_NUM_ENTER); return true; }

  if (tokenUpper.length() == 2 && tokenUpper[0] == 'F' && isDigit(tokenUpper[1])) {
    const int fnum = tokenUpper[1] - '0';
    switch (fnum) {
      case 1: bleKeyboard.press(KEY_F1); return true;
      case 2: bleKeyboard.press(KEY_F2); return true;
      case 3: bleKeyboard.press(KEY_F3); return true;
      case 4: bleKeyboard.press(KEY_F4); return true;
      case 5: bleKeyboard.press(KEY_F5); return true;
      case 6: bleKeyboard.press(KEY_F6); return true;
      case 7: bleKeyboard.press(KEY_F7); return true;
      case 8: bleKeyboard.press(KEY_F8); return true;
      case 9: bleKeyboard.press(KEY_F9); return true;
      default: break;
    }
  }
  if (tokenUpper.length() == 3 && tokenUpper[0] == 'F' && tokenUpper[1] == '1' && isDigit(tokenUpper[2])) {
    const int fnum = 10 + (tokenUpper[2] - '0');
    switch (fnum) {
      case 10: bleKeyboard.press(KEY_F10); return true;
      case 11: bleKeyboard.press(KEY_F11); return true;
      case 12: bleKeyboard.press(KEY_F12); return true;
      default: break;
    }
  }

  if (tokenUpper.length() == 1) {
    char c = tokenUpper[0];
    if (c >= 'A' && c <= 'Z') {
      c = (char)(c - 'A' + 'a');
    }
    bleKeyboard.press(c);
    return true;
  }

  return false;
}

static void executeConfiguredAction(const String& action) {
  if (action.startsWith(ACTION_TYPE_PREFIX)) {
    bleTypeString(action.substring(strlen(ACTION_TYPE_PREFIX)));
    return;
  }

  int start = 0;
  while (start < action.length()) {
    int plusPos = action.indexOf('+', start);
    if (plusPos == -1) {
      plusPos = action.length();
    }
    String token = action.substring(start, plusPos);
    token.trim();
    token.toUpperCase();
    (void)pressKeyToken(token);
    start = plusPos + 1;
  }
}



static void bleTypeString(const String& input) {
  for (int idx = 0; idx < input.length(); idx++) {
    bleKeyboard.print(input[idx]);
    bleKeyboard.releaseAll();
    delay(10);
  }
}

// Task Function
void infiniteScan(void * parameters) {
  for(;;) {
    if(initializedK){
      if (debounceHall[0]->debounce(LTBTN.checkTrig(0) != 0)) {
        executeConfiguredAction(hallActions[0]);
      }
      else if (debounceHall[1]->debounce(RTBTN.checkTrig(0) != 0)) {
        executeConfiguredAction(hallActions[1]);
      }
      else if (debounceHall[2]->debounce(LMBTN.checkTrig(0) != 0)) {
        executeConfiguredAction(hallActions[2]);
      }
      else if (debounceHall[3]->debounce(RMBTN.checkTrig(0) != 0)) {
        executeConfiguredAction(hallActions[3]);
      }
      else if (debounceHall[4]->debounce(LBBTN.checkTrig(0) != 0)) {
        executeConfiguredAction(hallActions[4]);
      }
      else if (debounceHall[5]->debounce(RBBTN.checkTrig(0) != 0)) {
        executeConfiguredAction(hallActions[5]);
      }
      bleKeyboard.releaseAll();
      //if (buttonG.debounce(digitalRead(BUTTON_PIN) == HIGH)) {
      //  bleKeyboard.print("LOVE YOU!");
      //}
    }
    vTaskDelay(pdMS_TO_TICKS(1));
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
  applyBrightnessCap((uint8_t)hallKnob.scanMapAngle());
}

enum class ScreenId : int {
  Unused = 0,
  BootBlank = 1,
  BootWithLogo = 2,
  SensorReadings = 3,
  CalibrationPrompt = 4,
  CalibrationMinMax = 5,
  Layer1 = 6,
  TimerSet = 7,
  TimerSetAck = 8,
  TimerLeft = 9,
  TimerOver = 10,
};

static void renderUnusedScreen() {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.print("hi");
}

static void renderBootBlank() {
  oled.clearDisplay(); // clear display
  oled.setTextSize(1);         // set text size
  oled.setTextColor(WHITE);    // set text color
  oled.drawBitmap(0, 0, myBootLogo, 128, 64, WHITE);
  oled.display();
  delay(1000);
}

static void renderBootWithLogo() {
  oled.clearDisplay(); // clear display
  oled.setTextSize(1);         // set text size
  oled.setTextColor(WHITE);    // set text color
  oled.drawBitmap(0, 0, myLogo, 128, 64, WHITE);
  //oled.print(F("EEPROM SAYS!: "));
  //oled.println(EEPROM.read(0));
  oled.display();
  delay(1000);
}

static void renderSensorReadings() {
  currentMillis3 = millis();
  if (currentMillis3 - previousMillis3 >= interval3) {
    oled.clearDisplay();
    oled.setTextSize(1);         // set text size
    oled.setCursor(0, 0);
    oled.print(F("Sensor Readings:")); //Using oled.print(F()) due to it not taking up RAM
    oled.print(F("Hall Angle: ")); oled.println(angleHall);
    oled.print(F("LT: "));oled.print(hall[0]->currentVal);oled.print(F(" RT: "));oled.println(hall[1]->currentVal);
    oled.print(F("LM: "));oled.print(hall[2]->currentVal);oled.print(F(" RM: "));oled.println(hall[3]->currentVal);
    oled.print(F("LB: "));oled.print(hall[4]->currentVal);oled.print(F(" RB: "));oled.println(hall[5]->currentVal);
    oled.print(F("Button: ")); oled.println(digitalRead(BUTTON_PIN)); oled.print(F(" ")); oled.print(duration); oled.println(F(" us"));
    oled.print(hall[0]->precision);
    oled.display();
    previousMillis3 = currentMillis3;
  }
}

static void renderCalibrationPrompt(int misc) {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.println(displayText);
  oled.println(hall[misc]->cali());
  oled.display();
}

static void renderCalibrationMinMax(int misc) {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.println(displayText);
  oled.println("Max :"+ String(hall[misc]->getMax())); oled.print("Min :"+ String(hall[misc]->getMin()));
  oled.display();
}

static void renderLayer1() {
  currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 >= interval2) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.drawBitmap(0, 0, myLogo, 128, 64, WHITE);
    oled.setCursor(0, 0);
    oled.print("Layer 1");
    oled.display();
    previousMillis3 = currentMillis3;
  }
}

static void renderTimerSetScreen(int timer) {
  oled.clearDisplay();
  oled.setTextSize(4);
  oled.setCursor(0, 0);
  oled.println(String(timer) + F(" M"));
  oled.setTextSize(1);
  oled.println();
  oled.println();
  oled.println();
  oled.println(F("Confirm       Cancel"));
  oled.display();
}

static void renderTimerLeftScreen() {
  const int totalSeconds = (int)maintimer.checkTimeLeftSeconds();
  const int minutesRender = totalSeconds / 60;
  const int secondsRender = totalSeconds % 60;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0);
  oled.setTextSize(1);
  oled.println(F("Time Left: "));
  oled.println();
  oled.setTextSize(3);
  oled.print(String(minutesRender)  );
  oled.setTextSize(1);
  oled.print(F("M "));
  oled.setTextSize(3);
  oled.print(String(secondsRender));
  oled.setTextSize(1);
  oled.print(F("S"));
  oled.setTextSize(1);
  oled.println();
  oled.println();
  oled.println();
  oled.println();
  oled.println();
  if (maintimer.timerPaused) {
    oled.println(F("Resume      Cancel"));
  } else {
    oled.println(F("Pause       Cancel"));
  }
  oled.display();
}

static void renderTimerOverScreen() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0);
  oled.println(String(maintimer.setMinutes) + F("M Timer Over:"));
  oled.setTextSize(4);
  oled.println(F("REST"));
  oled.setTextSize(1);
  oled.println();
  oled.println();
  oled.println();
  oled.println(F("OK       Go Again"));
  oled.display();
}

void screenRender(ScreenId screen, int timer, int misc = 0, const String& optText = "NULL") {
  (void)optText;
  switch (screen) {
    case ScreenId::Unused:
      renderUnusedScreen();
    break;

    case ScreenId::BootBlank:
      renderBootBlank();
    break;

    case ScreenId::BootWithLogo:
      renderBootWithLogo();
    break;

    case ScreenId::SensorReadings:
      renderSensorReadings();
    break;

    case ScreenId::CalibrationPrompt:
      renderCalibrationPrompt(misc);
    break;

    case ScreenId::CalibrationMinMax:
      renderCalibrationMinMax(misc);
    break;

    case ScreenId::Layer1:
      renderLayer1();
    break;

    case ScreenId::TimerSet:
      renderTimerSetScreen(timer);
    break;

    case ScreenId::TimerSetAck:

    break;

    case ScreenId::TimerLeft:
      renderTimerLeftScreen();
    break;

    case ScreenId::TimerOver:
      renderTimerOverScreen();
    break;

    default:
      // Intentionally no-op for unknown screens
    break;
  }
}

// Initialize Calibration for Kronos (Only when EEPROM is 0)
void initializeKronos(){
  const int numBtns = (int)HALL_BUTTON_COUNT;
  unsigned long startTimer = 0UL;
  const unsigned long timerSet = 500UL;
  bool calibrationComplete = false;
  bool arrayCalibrationComplete = false;
  int currentHall = 0;

  while (currentHall < numBtns) {
    calibrationComplete = false;
    while(!calibrationComplete) {
      while(hall[currentHall]->checkTrig(0) == 0) {
        displayText = "PRESS BTN" + String(currentHall+1);
        screenRender(ScreenId::CalibrationPrompt, 0, currentHall);
        // Prevent watchdog resets during long calibration waits
        vTaskDelay(pdMS_TO_TICKS(1));
      }  
      startTimer = millis();
      arrayCalibrationComplete = false;
      // IMPORTANT: check the completion flag first to avoid evaluating hall[currentHall]
      // after currentHall has been incremented (prevents out-of-bounds on the last button).
      while (!arrayCalibrationComplete && (hall[currentHall]->checkTrig(0) == 1)) {
        if ((startTimer + (timerSet/2)) > (millis())) {
          displayText = "FULLY PRESS";
          screenRender(ScreenId::CalibrationPrompt, 0, currentHall);
        }
        else if ((startTimer + (timerSet/2)+(timerSet / 4)) > millis()) {
          displayText = "CALIBRATING";
          screenRender(ScreenId::CalibrationPrompt, 0, currentHall);
        }
        else if ((startTimer + timerSet) < millis()) {
          displayText = "HALL " + String(currentHall+1) + "CHK";
          screenRender(ScreenId::CalibrationMinMax, 0, currentHall);
          delay(500);
          arrayCalibrationComplete = true;
          calibrationComplete = true;
          currentHall++;
        }

        // Yield while we poll/refresh during calibration
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    }
  }
}

bool timerMenuKeyScan(unsigned long timerMenuMs) {
  if (LTBTN.checkTrig(0) != 0) {
    return true;
  }
  if (RTBTN.checkTrig(0) != 0) {
    return true;
  }
  if ((digitalRead(BUTTON_PIN) == LOW) && (timerMenuMs > 1000UL)) {
    return true;
  }

  return false;
}
bool timerMenu (){
  unsigned long menuEnteredTime = millis();
  unsigned long menuNowTime = 0UL;
  unsigned long menuTimeDiff = 0UL;
  int timePrint = 0;
  bool prevLT = false;
  bool prevRT = false;
  initializedK = false;
  for(;;){
    menuNowTime = millis();
    menuTimeDiff = menuNowTime - menuEnteredTime;

    const bool curLT = (LTBTN.checkTrig(0) != 0);
    const bool curRT = (RTBTN.checkTrig(0) != 0);
    const bool ltPressedEdge = curLT && !prevLT;
    const bool rtPressedEdge = curRT && !prevRT;
    prevLT = curLT;
    prevRT = curRT;

    if (maintimer.timerRunning == 1 ) {
      // Show time left
      screenRender(ScreenId::TimerLeft, 0, 0);

      timerLedMeterUpdateFromRemaining(
        maintimer.checkTimeLeftMillis(),
        maintimer.totalDurationMs,
        TimerLedMode::Running
      );

      // Pause / Cancel
      if (ltPressedEdge) {
        maintimer.pause();
      } else if (rtPressedEdge) {
        maintimer.cancel();
        timerLedMeterClear();
        return false;
      }

      // Allow exiting the menu by holding the physical button.
      if ((digitalRead(BUTTON_PIN) == LOW) && (menuTimeDiff > 1000UL)) {
        return true;
      }
    }
    else if (maintimer.timerPaused == 1) {
      // Paused: show the same countdown screen, but with Resume/Cancel
      screenRender(ScreenId::TimerLeft, 0, 0);

      timerLedMeterUpdateFromRemaining(
        maintimer.checkTimeLeftMillis(),
        maintimer.totalDurationMs,
        TimerLedMode::Paused
      );

      if (ltPressedEdge) {
        maintimer.resume();
      } else if (rtPressedEdge) {
        maintimer.cancel();
        timerLedMeterClear();
        return false;
      }
    }
    else if (maintimer.timerRunning == 0){
      const int knobValue = hallKnob.scanMapAngle(12,255,1);
      timePrint = maintimer.setCheck(knobValue); // Set Timer Display Based On Rotary Encoder
      screenRender(ScreenId::TimerSet, timePrint, 0); // Render display timer time currently

      // Fill LEDs based on time being set (relative to 60 minutes max)
      timerLedMeterUpdateFromMinutes(timePrint);

      if (ltPressedEdge){ // Confirm
        maintimer.start(knobValue); // Start time based on rotary encoder location
        timerLedMeterUpdateFromRemaining(
          maintimer.checkTimeLeftMillis(),
          maintimer.totalDurationMs,
          TimerLedMode::Running
        );
        screenRender(ScreenId::TimerSetAck, timePrint, 0, "Timer Set!");
        delay (500);
        return true;
      }
      else if (rtPressedEdge){ // Cancel
        timerLedMeterClear();
        return false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool timerEnd(){
  screenRender(ScreenId::TimerOver, 0, 0, "Timer Set!");
  solidColor2(leds_75, NUM_LEDS_SCREENARRAY , CRGB::Red);
  delay(1000);
  for(;;){
    screenRender(ScreenId::TimerOver, 0, 0, "Timer Set!");
    if (LTBTN.checkTrig(0)){ // Check if left button was pressed 
      solidColor2(leds_75, NUM_LEDS_SCREENARRAY , CRGB::Black);
      maintimer.reset();
      timerLedMeterClear(true);
      return true;
    }
    else if (RTBTN.checkTrig(0)){
      solidColor2(leds_75, NUM_LEDS_SCREENARRAY , CRGB::Black);
      maintimer.reset();
      timerLedMeterClear(true);
      timerMenu();
      return true;
    }
    delay (25);
  }
}

bool timerPause(){
  for(;;){
    vTaskDelay(pdMS_TO_TICKS(25));
  }

  return false;
}

void audioLevelGraph() {
  for(;;){
    leds_75[pos] = CHSV(150, 255, 255);
    // Blur the entire strip
    blur1d(leds_75, NUM_LEDS_SCREENARRAY, 172);
    fadeToBlackBy(leds_75, NUM_LEDS_SCREENARRAY, 4);
    FastLED.show();
    // Move the position of the dot
    if (toggle) {
      pos = (pos + 1) % NUM_LEDS_SCREENARRAY;
    }
    toggle = !toggle;
    vTaskDelay(pdMS_TO_TICKS(20));
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  bool enterWifiConfig = false;

  // Initializing Hall Objects
  LTBTN.setChannel(1,0);
  RTBTN.setChannel(2,0);
  LMBTN.setChannel(1,1);
  RMBTN.setChannel(2,1);
  LBBTN.setChannel(1,2);
  RBBTN.setChannel(2,2);

  // Initialize I2C
  Wire.begin(SDA0_Pin, SCL0_Pin);
  // Faster I2C improves ADS1115 sampling latency (button response)
  Wire.setClock(400000);

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

  // Maximize sample rate to reduce per-read blocking time.
  // Default rates are much slower and can make button presses feel laggy.
  ads1.setDataRate(RATE_ADS1115_860SPS);

  if (!ads2.begin(0x49)) {
    Serial.println("Failed to initialize ADS1115 at 0x49");
    for(;;);
  }

  ads2.setDataRate(RATE_ADS1115_860SPS);

  // Hold the selected hall "button" at boot to enter WiFi configuration mode.
  // ADS1115 must be initialized before hallBtn.checkTrig().
  if (WIFI_CONFIG_HALL_INDEX < HALL_BUTTON_COUNT) {
    enterWifiConfig = hallHeldForMs(*hall[WIFI_CONFIG_HALL_INDEX], WIFI_CONFIG_HOLD_MS);
  }

  // If requested, enter WiFi config mode now that the screen + ADS are initialized.
  if (enterWifiConfig) {
    startWifiConfigPortal(WIFI_AP_SSID, PREFS_NAMESPACE, oled, hallActions, HALL_BUTTON_COUNT);
  }

  // Load configured keybinds (defaults preserved on first boot)
  keybindsLoadFromPrefs(PREFS_NAMESPACE, hallActions, HALL_BUTTON_COUNT);

  // Load and apply timer meter color style (0=white, 1=gradient)
  {
    const uint8_t meterStyle = keybindsLoadMeterStyleFromPrefs(PREFS_NAMESPACE);
    timerLedMeterSetColorStyle(meterStyle == 1 ? TimerLedColorStyle::Gradient : TimerLedColorStyle::White);
  }

  // Load global LED brightness (applied after FastLED init)
  g_ledBrightness = keybindsLoadLedBrightnessFromPrefs(PREFS_NAMESPACE);

  // Initialize AS5600 Hall Effect Sensor
  as5600.begin();

  // Initialize LED Strips
  FastLED.addLeds<WS2812B, SCREENARRAY, GRB>(leds_75, NUM_LEDS_SCREENARRAY);
  FastLED.addLeds<WS2812B, BUTTONARRAY, GRB>(leds_6, NUM_LEDS_BUTTONARRAY);
  applyBrightnessCap(255);

  timerLedMeterInit(leds_75, NUM_LEDS_SCREENARRAY);

  // LED Strip Animation
  ledCycle(leds_75, NUM_LEDS_SCREENARRAY);
  screenRender(ScreenId::BootBlank, 0, 0);
  screenRender(ScreenId::BootWithLogo, 0, 0);

  // Set Precision variables for all 6 hall effect sensors
  int setPrecisionTemp = 3;
  for(size_t idx = 1; idx < HALL_BUTTON_COUNT; idx++) { // 6 Hall Effect Sensors
    hall[idx]->setPrecision(setPrecisionTemp);
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
    initializedK = true;
    scanEnabled = true;

  // Clear Display
  ledClear(leds_75, NUM_LEDS_SCREENARRAY);
  FastLED.show();

  // Start the scan task now (previously this never started because audioLevelGraph() blocked).
  // Pin it to the Arduino core for more consistent latency.
  xTaskCreatePinnedToCore(
    infiniteScan,
    "infiniteScan",
    4096,
    NULL,
    2,
    NULL,
    ARDUINO_RUNNING_CORE
  );
}


void loop() {
  const unsigned long start = micros();

  // If the timer is active, keep the LED bar updated even outside the menu.
  if (maintimer.timerRunning) {
    timerLedMeterUpdateFromRemaining(
      maintimer.checkTimeLeftMillis(),
      maintimer.totalDurationMs,
      TimerLedMode::Running
    );
  }

  if(digitalRead(BUTTON_PIN) == HIGH) { // if button is not pressed
    if (maintimer.timeOver() && (maintimer.timerRunning == 1) ){ // if timer is over
      timerEnd(); 
    }
    else{ 
      screenRender(ScreenId::SensorReadings, 0, 0);
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
  else { // if button is pressed
    timerMenu();
    vTaskDelay(pdMS_TO_TICKS(50));
    initializedK = true; // re-initializing Kronos to allow for infinite scan
  }
  duration = micros() - start;

  vTaskDelay(pdMS_TO_TICKS(10));
}

