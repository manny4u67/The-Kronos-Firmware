# WiFi Keybinds: Module Split + Secrets Linker Fix

## Why this change
`src/main.cpp` had grown to include:
- WiFi AP config portal UI + HTTP routes
- NVS (Preferences) load/save logic
- default keybind initialization

To make the firmware easier to maintain, those concerns were extracted into focused modules.

## What changed

### 1) New keybind persistence module
Added:
- [include/keybinds.h](../include/keybinds.h)
- [src/keybinds.cpp](../src/keybinds.cpp)

Responsibilities:
- Load keybind strings from Preferences (NVS)
- Save keybind strings to Preferences (NVS)
- Initialize missing keys with defaults on first boot
- Provide consistent field/key naming (`btn0..btn5`)

Public API:
- `keybindsLoadFromPrefs(prefsNamespace, actions, actionCount)`
- `keybindsSaveToPrefs(prefsNamespace, actions, actionCount)`
- `keybindsKeyForButton(idx)`

Notes:
- Uses `Preferences::isKey()` so an intentionally empty string is not treated as “missing”.
- Uses `prefs.end()` to close the NVS handle after read/write.

### 2) New WiFi config portal module
Added:
- [include/wifi_config.h](../include/wifi_config.h)
- [src/wifi_config.cpp](../src/wifi_config.cpp)

Responsibilities:
- Start AP mode (`KRONOS-CONFIG`)
- Serve `GET /` (HTML form) and `POST /save`
- Render OLED “WiFi Config Mode” screen with SSID + IP
- Save updated keybinds and reboot

Public API:
- `startWifiConfigPortal(ssid, prefsNamespace, oled, actions, actionCount)`

### 3) `main.cpp` cleaned up to use modules
Updated:
- [src/main.cpp](../src/main.cpp)

Changes:
- Removed inline Preferences/WebServer/WiFi portal + persistence helper functions.
- Added includes for the new modules.
- Calls:
  - `startWifiConfigPortal(...)` when the boot button is held
  - `keybindsLoadFromPrefs(...)` during normal boot

This reduces the size/complexity of `main.cpp` and isolates the config portal logic.

## Secrets linker issue (and fix)
After splitting into multiple `.cpp` files, the build failed with:
- `multiple definition of 'PhantomPass'`
- `multiple definition of 'MainSolWallet'`

Root cause:
- [include/mysecret.h](../include/mysecret.h) originally *defined* global `String` variables in a header.
- When included by more than one `.cpp`, the linker sees multiple definitions.

Fix applied:
- Updated [include/mysecret.h](../include/mysecret.h):
  - added `#pragma once`
  - changed secrets to `static const String ...` so they have internal linkage

This makes `mysecret.h` safe to include from multiple translation units.

## Behavior
No intended behavior change:
- The WiFi portal still runs when holding the physical boot button.
- Keybinds still persist via Preferences (NVS).
- OLED still shows WiFi config mode screen.

## Follow-ups (optional)
- Extract BLE action parsing/execution (`executeConfiguredAction`, token parsing) into its own module (e.g. `actions.cpp`).
- Add an ADC sampling cache task to reduce ADS1115 blocking reads during scanning.
