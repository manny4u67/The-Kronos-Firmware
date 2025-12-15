#include <Arduino.h>

class MxgicDebounce
{
  private:
    bool debouncedState = false;
    uint8_t transitionCount = 0;
  public:
    void begin() {
      debouncedState = false;
      transitionCount = 0;
    }

  // Returns true once per "press" (rising edge) after the input has been
  // stable for `stableSamples` consecutive calls.
  // This is sample-count based (depends on how often you call it).
  bool debounce(bool output, uint8_t stableSamples = 3) {
    if (stableSamples < 1) {
      stableSamples = 1;
    }

    if (output == debouncedState) {
      transitionCount = 0;
      return false;
    }

    // Potential transition; require consecutive samples before accepting.
    if (transitionCount < 255) {
      transitionCount++;
    }
    if (transitionCount < stableSamples) {
      return false;
    }

    transitionCount = 0;
    debouncedState = output;
    return debouncedState;
  }
};