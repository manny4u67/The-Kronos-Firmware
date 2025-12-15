# WiFi Keybind Configuration (AP Mode)

## Overview
This firmware supports configuring the 6 hall-button keybind actions over WiFi.

- The ESP32 starts a WiFi Access Point (AP) only when you hold the physical button on `BUTTON_PIN` during boot.
- You connect to the AP, open a web page, edit keybinds, then **Save & Reboot**.
- Keybinds are stored in ESP32 **NVS (Preferences)** so they persist across resets/flashes.

## How to Enter Config Mode
1. Power off the device.
2. Hold the physical button connected to `BUTTON_PIN` (GPIO 8).
3. Power on / reset the device while still holding.
4. Watch Serial at `115200` for the AP address.

The firmware will print something like:

- `Config AP started. Connect to KRONOS-CONFIG then open http://192.168.4.1`

## How to Configure
1. Connect your phone/laptop to the WiFi network:
   - SSID: `KRONOS-CONFIG`
2. Open the config page:
   - `http://192.168.4.1/`
3. Edit each button action and click **Save & Reboot**.

## Action Syntax
Each button action is a single string.

### 1) Type text
Prefix with `TYPE:`

- Example: `TYPE:hello world`

### 2) Key combos
Write keys separated by `+`.

Examples:
- `CTRL+Z`
- `CTRL+SHIFT+Z`
- `GUI+NUM_MINUS`
- `DELETE`

Supported tokens include:
- Modifiers: `CTRL`, `SHIFT`, `ALT`, `GUI` (also accepts `WIN`, `CMD`)
- Common keys: `ENTER`, `TAB`, `ESC`, `BACKSPACE`, `DELETE`, `SPACE`
- Arrows: `UP`, `DOWN`, `LEFT`, `RIGHT`
- Numpad: `NUM_MINUS`, `NUM_PLUS`, `NUM_ENTER`
- Function keys: `F1`..`F12`
- Single characters: `A`..`Z`, digits, and most punctuation (use a single character token)

## Defaults
On first boot (or if nothing is stored yet), defaults match the previous hardcoded behavior:
- Button 1: `TYPE:` + `PhantomPass`
- Button 2: `GUI+NUM_MINUS`
- Button 3: `CTRL+Z`
- Button 4: `CTRL+SHIFT+Z`
- Button 5: `TYPE:` + `MainSolWallet`
- Button 6: `DELETE`

## Security Note
AP mode is currently open (no password). Treat it as a local configuration interface.
If you want a password and/or a time-limited config window, we can add that next.
