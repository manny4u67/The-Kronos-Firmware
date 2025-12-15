#include <Arduino.h>
#include <Preferences.h>

#include "keybinds.h"
#include "mysecret.h"

static String defaultActionForButton(size_t idx) {
  switch (idx) {
    case 0: return String(ACTION_TYPE_PREFIX) + PhantomPass;
    case 1: return F("GUI+NUM_MINUS");
    case 2: return F("CTRL+Z");
    case 3: return F("CTRL+SHIFT+Z");
    case 4: return String(ACTION_TYPE_PREFIX) + MainSolWallet;
    case 5: return F("DELETE");
    default: return String();
  }
}

String keybindsKeyForButton(size_t idx) {
  return String("btn") + String((int)idx);
}

static const char* keybindsKeyForMeterStyle() {
  return "meterStyle";
}

static const char* keybindsKeyForLedBrightness() {
  return "ledBrightness";
}

static uint8_t clampBrightness(int value) {
  if (value < 1) return 1;
  if (value > 255) return 255;
  return (uint8_t)value;
}

uint8_t keybindsLoadMeterStyleFromPrefs(const char* prefsNamespace) {
  Preferences prefs;
  if (!prefs.begin(prefsNamespace, false)) {
    Serial.println(F("[prefs] begin() failed; meterStyle=0"));
    return 0;
  }

  const char* key = keybindsKeyForMeterStyle();
  if (!prefs.isKey(key)) {
    prefs.putUChar(key, 0);
    prefs.end();
    Serial.println(F("[prefs] initialized meterStyle=0"));
    return 0;
  }

  const uint8_t style = prefs.getUChar(key, 0);
  prefs.end();
  return style;
}

void keybindsSaveMeterStyleToPrefs(const char* prefsNamespace, uint8_t meterStyle) {
  Preferences prefs;
  if (!prefs.begin(prefsNamespace, false)) {
    Serial.println(F("[prefs] begin() failed; meterStyle not saved"));
    return;
  }

  prefs.putUChar(keybindsKeyForMeterStyle(), meterStyle);
  prefs.end();
  Serial.print(F("[prefs] saved meterStyle="));
  Serial.println((int)meterStyle);
}

uint8_t keybindsLoadLedBrightnessFromPrefs(const char* prefsNamespace) {
  Preferences prefs;
  if (!prefs.begin(prefsNamespace, false)) {
    Serial.println(F("[prefs] begin() failed; ledBrightness=20"));
    return 20;
  }

  const char* key = keybindsKeyForLedBrightness();
  if (!prefs.isKey(key)) {
    prefs.putUChar(key, 20);
    prefs.end();
    Serial.println(F("[prefs] initialized ledBrightness=20"));
    return 20;
  }

  const uint8_t b = prefs.getUChar(key, 20);
  prefs.end();
  return clampBrightness((int)b);
}

void keybindsSaveLedBrightnessToPrefs(const char* prefsNamespace, uint8_t brightness) {
  Preferences prefs;
  if (!prefs.begin(prefsNamespace, false)) {
    Serial.println(F("[prefs] begin() failed; ledBrightness not saved"));
    return;
  }

  const uint8_t b = clampBrightness((int)brightness);
  prefs.putUChar(keybindsKeyForLedBrightness(), b);
  prefs.end();
  Serial.print(F("[prefs] saved ledBrightness="));
  Serial.println((int)b);
}

void keybindsLoadFromPrefs(const char* prefsNamespace, String* actions, size_t actionCount) {
  if (actions == nullptr || actionCount == 0) {
    return;
  }

  Preferences prefs;
  if (!prefs.begin(prefsNamespace, false)) {
    for (size_t i = 0; i < actionCount; i++) {
      actions[i] = defaultActionForButton(i);
    }
    Serial.println(F("[prefs] begin() failed; using defaults"));
    return;
  }

  bool wroteAnyDefaults = false;
  for (size_t i = 0; i < actionCount; i++) {
    const String key = keybindsKeyForButton(i);

    // Only auto-fill defaults when the key does not exist.
    // This preserves intentionally-empty strings.
    if (!prefs.isKey(key.c_str())) {
      actions[i] = defaultActionForButton(i);
      prefs.putString(key.c_str(), actions[i]);
      wroteAnyDefaults = true;
      continue;
    }

    actions[i] = prefs.getString(key.c_str(), "");
  }

  prefs.end();

  if (wroteAnyDefaults) {
    Serial.println(F("[prefs] initialized missing keys with defaults"));
  }
  for (size_t i = 0; i < actionCount; i++) {
    Serial.print(F("[prefs] btn"));
    Serial.print((int)i);
    Serial.print(F("="));
    Serial.println(actions[i]);
  }
}

void keybindsSaveToPrefs(const char* prefsNamespace, const String* actions, size_t actionCount) {
  if (actions == nullptr || actionCount == 0) {
    return;
  }

  Preferences prefs;
  if (!prefs.begin(prefsNamespace, false)) {
    Serial.println(F("[prefs] begin() failed; not saving"));
    return;
  }

  for (size_t i = 0; i < actionCount; i++) {
    const String key = keybindsKeyForButton(i);
    prefs.putString(key.c_str(), actions[i]);
  }

  prefs.end();
  Serial.println(F("[prefs] saved keybinds"));
}
