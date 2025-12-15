# Hall Calibration Guru Meditation Fix (LoadProhibited)

## Summary
During hall calibration (`initializeKronos()`), the ESP32-S3 would reboot with a Guru Meditation error:

- `Core 1 panic'ed (LoadProhibited)`
- `EXCVADDR: 0x0000002c`

This was caused by an out-of-bounds access of the `hall[]` pointer array in the calibration loop.

## Symptoms
- Device resets after completing the hall calibration sequence.
- Serial monitor shows a Guru Meditation `LoadProhibited` panic and a backtrace.

## Root Cause
In `initializeKronos()` the code increments `currentHall++` when a hall button finishes calibrating.

However, the inner loop condition was structured like this:

```cpp
while ((hall[currentHall]->checkTrig(0) == 1) && (!arrayCalibrationComplete)) {
  ...
  if (...) {
    arrayCalibrationComplete = true;
    calibrationComplete = true;
    currentHall++; // <-- can become 6 (out of range)
  }
}
```

In C/C++, the left side of `&&` is evaluated first.

So after `currentHall++` reaches `6` (valid indices are `0..5`), the *next* evaluation of the loop condition attempts to read:

- `hall[6]->checkTrig(0)`

That dereferences an invalid pointer and triggers `LoadProhibited`.

## How We Confirmed It
We decoded the ESP32 backtrace addresses against the built ELF using `addr2line`, which pointed into:

- `MxgicHall::checkTrig(int)`
- called from `initializeKronos()`

## Fix
Reorder the while-condition to short-circuit on the completion flag *before* dereferencing `hall[currentHall]`:

```cpp
while (!arrayCalibrationComplete && (hall[currentHall]->checkTrig(0) == 1)) {
  ...
}
```

Now, once `arrayCalibrationComplete` becomes `true`, the loop exits without re-evaluating `hall[currentHall]`.

## Files Changed
- `src/main.cpp`

## Notes
- This fix is behavior-preserving: it doesn’t change the calibration logic or timing, only prevents an unsafe condition re-check.
- There were also small `vTaskDelay(pdMS_TO_TICKS(1))` yields added inside the calibration loops to reduce the chance of watchdog starvation during long waits.
