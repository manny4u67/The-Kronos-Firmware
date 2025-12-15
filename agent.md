# Agent Notes (The Kronos Firmware)

This file is a working map of the repo to make future coding changes faster and safer. It captures the current architecture, module responsibilities, and a few “gotchas” to watch for when refactoring.

## Build + Environment
- **Tooling**: PlatformIO (Arduino framework)
- **Target**: `esp32-s3-devkitc-1`
- **Config**: `platformio.ini`
- **Key build flags**:
  - `-DARDUINO_USB_CDC_ON_BOOT=1`
  - `-DCORE_DEBUG_LEVEL=1`
- **Libraries** (PlatformIO `lib_deps`):
  - Adafruit SSD1306 (OLED)
  - Adafruit ADS1X15 (ADS1115 ADCs)
  - FastLED (LED strips) — pinned to **3.5.0** due to ESP32-S3 toolchain incompatibilities in newer releases
  - AS5600 (magnetic angle sensor)
  - ESP32 BLE Keyboard (HID)

### Build commands
- Build: `platformio run`
- Upload: `platformio run -t upload`

## Hardware + IO Map (as implemented)
### Pins
- **I2C**: defined in `src/main.cpp`
  - `SDA0_Pin = 17`
  - `SCL0_Pin = 18`
  - Note: `Wire.begin(SCL0_Pin, SDA0_Pin)` is currently called (verify parameter order for ESP32 Arduino: commonly `Wire.begin(sda, scl)`)
- **Button**: GPIO `8` as `INPUT_PULLUP`
- **WS2812B LEDs**:
  - “Screen array” data pin: `48`, length `75`
  - “Button array” data pin: `35`, length `6`

### Sensors
- **ADS1115 x2**
  - Declared globally in `include/mxgicHall.h` as `ads1` and `ads2`
  - Initialized in `setup()` with `ads1.begin(0x48)` and `ads2.begin(0x49)`
- **AS5600**
  - Declared globally in `include/mxgicRotary.h` as `as5600`
  - Initialized in `setup()` with `as5600.begin()`

## Repo Map
### `src/main.cpp` (top-level firmware)
Contains almost all application logic:
- Global device objects (OLED, BLE keyboard, LED buffers)
- Task: `infiniteScan()` (FreeRTOS task)
- LED helper functions: `ledCycle`, `ledClear`, `solidColor`, `solidColor2`, `ledFadeUp`
- UI: `screenRender(screen, timer, misc, optText)`
- Calibration: `initializeKronos()`
- Timer UI: `timerMenuKeyScan`, `timerMenu`, `timerEnd` (and stub `timerPause`)
- LED “audio level graph” animation: `audioLevelGraph()`
- Arduino lifecycle: `setup()` and `loop()`

### `include/kronosDisplay.h`
- Provides `displayText` (global `String`)
- Stores OLED bitmap assets in PROGMEM: `myBootLogo`, `myLogo`

### `include/mxgicHall.h`
- Declares **global** ADC objects:
  - `Adafruit_ADS1115 ads1;`
  - `Adafruit_ADS1115 ads2;`
- Defines class `MxgicHall`:
  - `setChannel(adcSelect, adcChannel)` selects ADS chip (1/2) and channel (0..3)
  - `rawRead()` reads ADS channel and stores `currentVal`
  - `cali()` updates min/max during calibration sampling
  - `caliRead()` constrains to min/max then maps into a “precision table” range
  - `checkTrig(option)`
    - `option=0`: currently uses a hard threshold `cali() > 11000`
    - `option=1`: uses calibrated mapping vs `trigPoint`

### `include/mxgicRotary.h`
- Declares **global** `AS5600 as5600;`
- Defines class `MxgicRotary`:
  - `rawAngle()` passthrough
  - `scanMapAngle(myMap, myMap2, selectMap)` maps raw 0..4096 to configured ranges
  - `checkRotation()` returns 0 (none), 1 (forward), 2 (reverse) based on delta threshold

### `include/mxgicTimer.h`
- Defines class `MxgicTimer`:
  - Uses `millis()` to compute `endTime` and remaining time
  - Stores a preset table (12 entries) from 1..60 minutes
  - Key methods: `start(index)`, `pause()`, `resume()`, `reset()`, `timeOver()`

### `include/mxgicDebounce.h`
- Defines class `MxgicDebounce` with a shift-register style debouncer.

### `include/ledMation.h`
- Defines `LedMation` with a fixed `linearCircleArray` mapping and helper `linearCircle(advance)`.

### `include/mysecret.h`
- Included by `src/main.cpp`.
- Appears to contain sensitive strings used by BLE keyboard macros (e.g., passphrase / wallet-like strings).
- Treat as **secret material**:
  - Avoid printing in logs
  - Avoid committing to public repos
  - Consider adding to `.gitignore` if it’s not already

## Runtime Model (How the firmware behaves)
### Startup (`setup()`)
1. Initialize serial, EEPROM, BLE keyboard.
2. Configure GPIO 8 input.
3. Assign ADS channels to 6 hall “button” sensors (`MxgicHall` instances).
4. Initialize I2C + OLED.
5. Initialize both ADS1115 chips.
6. Initialize AS5600.
7. Configure FastLED and run a boot LED cycle.
8. (Currently) forces initialization/calibration via `LTBTN` or always sets `initializedK=1` and enables scan.
9. Clears LEDs.
10. Runs `audioLevelGraph()` (this is a blocking loop until LT button is pressed).
11. Starts a FreeRTOS task `infiniteScan()`.

### Main loop (`loop()`)
- Measures loop duration with `micros()`.
- If GPIO 8 is **not pressed**:
  - If timer is running and over → `timerEnd()`
  - Else show debug sensor screen (`screenRender(3)`)
- If GPIO 8 **is pressed**:
  - Enter `timerMenu()`

### Background task (`infiniteScan()`)
- Runs continuously (`for(;;)`), gated by `initializedK`.
- Checks 6 hall “buttons” and issues BLE keypress sequences.

## Things to Watch (for future changes)
These are not necessarily “bugs,” but they matter for safe refactors.

1. **Globals defined in headers**
   - `ads1`, `ads2`, `as5600`, and `displayText` are defined in headers, not declared `extern`.
   - This is safe only as long as those headers are effectively included in exactly one translation unit.
   - If you later add more `.cpp` files and include these headers, you’ll likely hit multiple-definition linker errors.

2. **Secret handling**
   - BLE macros type out strings from `mysecret.h`.
   - Avoid echoing those values in any debugging output or docs.

3. **Wire pin ordering**
   - Double-check ESP32 Arduino signature for `Wire.begin(...)`. If it is `Wire.begin(sda, scl)`, the current call swaps pins.

4. **Debounce implementation**
   - `MxgicDebounce::debounce()` uses a `static` local `state`, meaning **all instances share one debounce state**.
   - If per-button debouncing is desired, `state` should be an instance member (not a local static).

5. **Timer menu scan helper**
   - `timerMenuKeyScan(int)` is declared `bool` but does not return on all paths.

6. **Calibration + thresholds**
   - `MxgicHall::checkTrig(0)` uses a fixed threshold (`> 11000`) while calibration routines also exist.
   - If you intend fully calibrated behavior, consider converging on the calibrated trigger path (`option=1`).

## Where to Extend
- **New OLED screens**: add new `case` branches in `screenRender()`.
- **New BLE shortcuts**: extend the `infiniteScan()` button mapping.
- **New LED animations**: add functions like `audioLevelGraph()` and decide whether they should block.
- **Refactor direction** (if desired): move large subsystems out of `src/main.cpp` into dedicated modules, but first convert header-defined globals to `extern` + a single `.cpp` definition.

## FastLED Compatibility Note
If FastLED is upgraded and the build breaks in `.pio/libdeps/.../FastLED/src/fl/rbtree.h`, revert to the known-good pin:
- `fastled/FastLED@3.5.0`

(See `docs/FASTLED_COMPILATION_FIX.md` for details.)
