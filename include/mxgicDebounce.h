// MxgicDebounce.h - simple (fast) single-pole debounce helper
// Each instance maintains its own shift-register state (was previously static / shared)

#pragma once
#include <Arduino.h>

class MxgicDebounce {
private:
  uint16_t _state = 0; // 16-bit shift register of historical samples
public:
  void reset() { _state = 0; }

  // Returns true on a clean LOW->HIGH transition (button press) after stability window
  bool update(bool levelHigh) {
    _state = (_state << 1) | (levelHigh ? 1 : 0) | 0xfe00; // preload upper bits to require consecutive highs
    return _state == 0xfe01; // pattern: 1111 1110 0000 0001 -> edge + stable
  }
};