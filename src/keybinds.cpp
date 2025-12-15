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
