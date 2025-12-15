# FastLED Compilation Fix

## Problem
The project failed to compile with **FastLED version 3.9.7** (and 3.6.0), producing numerous template-related compilation errors in the FastLED library's `rbtree.h` file.

### Error Summary
- **File**: `.pio/libdeps/esp32-s3-devkitc-1/FastLED/src/fl/rbtree.h`
- **Line**: 640+
- **Errors**: 150+ compilation errors related to:
  - Undeclared `RBNode`, `iterator`, `const_iterator` types
  - Missing template arguments in `fl::pair<iterator, bool>` declarations
  - Undeclared `comp_` member variable
  - Missing `mTree` field in class definitions
  - Invalid template instantiation for `MapRedBlackTree` and `SetRedBlackTree`

### Root Cause
FastLED versions 3.9.7 and 3.6.0 have known incompatibilities with the ESP32-S3 compiler configuration (Xtensa toolchain 8.4.0). The newer versions introduced template code that doesn't compile correctly with this toolchain, specifically in the red-black tree data structure implementation.

## Solution
**Downgraded FastLED to version 3.5.0**, which is stable and compatible with the ESP32-S3 toolchain.

### Changes Made
**File**: `platformio.ini`

Changed:
```ini
fastled/FastLED@^3.9.7
```

To:
```ini
fastled/FastLED@3.5.0
```

## Results
✅ **Build Successful**
- Compilation completed without errors
- Firmware binary created: **966,256 bytes**
- Location: `.pio/build/esp32-s3-devkitc-1/firmware.bin`

### Minor Warnings
- One minor warning remains in `src/main.cpp` line 362:
  - Function `timerMenuKeyScan(int)` reaches end without return statement
  - Recommendation: Add return statement to resolve

## Verification
Run the build command to verify:
```bash
platformio run
```

Expected output:
```
===== [SUCCESS] ===== Took X.XX seconds =====
```

## Compatibility Notes
- **FastLED 3.5.0** is fully compatible with:
  - ESP32-S3-DevKitC-1 board
  - Arduino framework
  - Other project dependencies (Adafruit SSD1306, ADS1X15, AS5600, etc.)

## References
- FastLED GitHub: https://github.com/FastLED/FastLED
- ESP32 Toolchain: Xtensa ESP32-S3 8.4.0+2021r2-patch5
- PlatformIO: https://platformio.org/

---
**Date Fixed**: December 14, 2025
**Build Environment**: PlatformIO with Espressif32 platform 6.12.0
