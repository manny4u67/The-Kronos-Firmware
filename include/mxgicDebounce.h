#include <Arduino.h>

class MxgicDebounce
{
  private:
    uint8_t btn;
    uint16_t state = 0;
  public:
    void begin() {
      state = 0;
    }
  bool debounce(bool output) {
    static uint16_t state = 0;
    state = (state << 1) | output | 0xfe00;
    return (state == 0xfe01);
  }
};