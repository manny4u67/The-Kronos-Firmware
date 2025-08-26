# Refactor Notes (August 2025)

This document summarizes the structural and API changes introduced during the initial refactor pass on the `refactor` branch.

## Overview Goals
- Improve header hygiene (guards, extern globals)
- Encapsulate per-instance state (debounce, hall sensors)
- Reduce duplication / unreachable code
- Clarify APIs (naming, const correctness, bounds checking)
- Prepare for further modularization (display, menus, events)

## File-by-File Changes

### `include/kronosDisplay.h`
- Added `#pragma once`.
- Converted global `String displayText` to `extern` declaration to avoid multiple definition issues; definition moved to `main.cpp`.
- Added brief comment header.

### `include/mxgicDebounce.h`
- Added `#pragma once` and documentation comment.
- Replaced previous class that used a **static** local shift register (shared across all instances) with per-instance `_state` member.
- Renamed `debounce()` to `update()`; added `reset()`.
- Simplified logic: returns true on stable rising edge pattern; improved clarity.

### `include/mxgicHall.h`
- Added `#pragma once` and documentation comment.
- Removed direct instantiation of ADS1115 objects; now declares them `extern` (defined in `main.cpp`).
- Converted magic constants to `constexpr` tables.
- Changed internal min/max types to `uint16_t`; initialized for first-sample update.
- Added bounds checking for precision & ADC channel; simplified raw read logic.
- Removed unreachable `Serial.println` after return.
- Added defensive check in `caliRead()` to avoid mapping when not calibrated (min==max).
- Added const correctness for getters.

### `include/mxgicRotary.h`
- Added `#pragma once` and `extern AS5600 as5600` instead of owning global.
- Made precision map `constexpr`.
- Added bounds checks in `setPrecision()`.
- Consolidated mapping logic; removed dead `break` paths after returns.
- Added hysteresis parameter to `checkRotation()` (default 100) for clarity.

### `include/ledMation.h`
- Added `#pragma once`.
- Converted array to `static constexpr` to place in flash / ROM.
- Added safety ensuring positive modulo result.
- Stubbed `setArray()` for future extension.

### `include/mxgicTimer.h`
- Full rewrite for clarity:
  - Added `#pragma once`.
  - Consolidated repeated time-left computations (`timeLeftMillis/Seconds/Minutes`).
  - Added `constexpr` LUTs for minutes and corresponding milliseconds.
  - Added `clampIndex()` helper.
  - Simplified state flags and start/pause/resume/reset semantics.
  - `timeOver()` now derives from `timeLeftMillis()` result.

### `src/main.cpp`
- Added global definitions for `ads1`, `ads2`, `as5600`, and `displayText` to satisfy new extern declarations.
- Updated includes order / comments.
- Replaced calls to deprecated `debounce()` with `update()`.
- Adjusted timer rendering to use new `maintimer.timeLeftSeconds()` & `minutesOption()`.
- Corrected timer over screen to show minutes by dividing `setTime`.
- Minor comment & clarity improvements (e.g., EEPROM block noted as future use, removed superfluous returns).

### Other Headers (`mxgicSin.h`, etc.)
- (No functional refactor yet; candidate for removal or implementation in future pass.)

## Behavioral Changes
- Debouncing now isolated per button; previously all hall buttons shared a single static shift state—this could have caused missed or phantom activations.
- Hall sensor raw & calibrated reads safer when not yet calibrated (avoid mapping divide-by-zero scenario).
- Timer computations more consistent and less error-prone (single source of truth for time left).
- Potential slight change in timing of debounce detection due to per-instance state; edge detection semantics maintained.

## Deferred / Planned Next Steps
1. Extract `screenRender()` logic into `DisplayController` (non-blocking, diff-based updates).
2. Replace blocking `while` calibration & menu loops with a state machine updated each loop/tick.
3. Remove most `delay()` calls in favor of elapsed-time checks; critical for responsive BLE & multitasking.
4. Encapsulate BLE shortcut sequences; add optional macro guards for secrets.
5. Create `config.h` for pin assignments, thresholds, and timing constants.
6. Add lightweight event queue (button press, rotation delta, timer events).
7. Move bitmap data into a dedicated `.cpp` to reduce compile times and header weight.
8. Implement / remove unused `MxgicSin` or rename for clarity.
9. Reintroduce EEPROM calibration persistence behind a robust verification scheme (version + checksum) to prevent crash loops.

## Risk Notes
- Ensure no other translation unit defines `displayText`; only `main.cpp` should.
- If other modules later need ADS instances, keep extern pattern consistent.
- Behavior reliant on previous shared debounce side effects (if any) will change; test each hall button.

## Validation
- Header edits compile clean (no reported errors by static check tool).
- API calls in `main.cpp` updated accordingly.

## Suggested Tests Post-Flash
1. Confirm each hall button triggers distinct action without cross-talk.
2. Verify timer menu still sets and displays correct durations.
3. Check that rotary mapping functions produce expected ranges after precision adjustments.
4. Observe LED brightness mapping (no regression in `ledFadeUp`).
5. Run calibration routine path (force call) to ensure no infinite loops or missed triggers.

---
Feel free to extend this file with future refactor phases.
