#include "timer_led_meter.h"

// Physical map for the 75-LED "screen" array is a 5x15 grid.
static constexpr uint8_t METER_COLS = 5;
static constexpr uint8_t METER_ROWS = 15;

static CRGB* g_meterLeds = nullptr;
static int g_meterNumLeds = 0;

static uint32_t g_lastTimerLedUpdateMs = 0;
static uint16_t g_lastTimerMeterLevel256 = 0;
static TimerLedMode g_lastTimerLedMode = TimerLedMode::None;
static TimerLedColorStyle g_colorStyle = TimerLedColorStyle::White;

static uint16_t clampMeterLevel256(int value) {
  if (value < 0) return 0;
  const int maxLevel = (int)METER_ROWS * 256;
  if (value > maxLevel) return (uint16_t)maxLevel;
  return (uint16_t)value;
}

static int meterLedIndexForRowCol(uint8_t rowTop0, uint8_t col0) {
  // Maps the physical grid position to the LED index (0..74).
  // Columns are wired in a serpentine pattern per docs/ARRAY.md.
  if (rowTop0 >= METER_ROWS || col0 >= METER_COLS) {
    return -1;
  }

  int id = -1; // 1-based ID from docs
  switch (col0) {
    case 0: id = 1 + rowTop0; break;          // 1..15 top->bottom
    case 1: id = 30 - rowTop0; break;         // 30..16 top->bottom
    case 2: id = 31 + rowTop0; break;         // 31..45 top->bottom
    case 3: id = 60 - rowTop0; break;         // 60..46 top->bottom
    case 4: id = 61 + rowTop0; break;         // 61..75 top->bottom
    default: return -1;
  }

  const int idx = id - 1; // to 0-based array index
  if (idx < 0 || idx >= g_meterNumLeds) {
    return -1;
  }
  return idx;
}

static CRGB meterColorForRowFromBottom(uint8_t rowFromBottom0, TimerLedMode mode) {
  (void)rowFromBottom0;

  if (g_colorStyle == TimerLedColorStyle::White) {
    (void)mode;
    // Static white meter; brightness is controlled by global FastLED brightness
    // and by per-row scaling for fractional fills.
    return CRGB::White;
  }

  // Gradient style:
  // - Running: VU-style green->yellow->red as it rises
  // - Setting: blue
  // - Paused: yellow
  if (mode == TimerLedMode::Running) {
    const uint8_t maxRow = (METER_ROWS > 0) ? (METER_ROWS - 1) : 1;
    const uint8_t r = (rowFromBottom0 > maxRow) ? maxRow : rowFromBottom0;
    const uint8_t hue = (uint8_t)(96 - ((uint16_t)96 * r) / maxRow);
    return CHSV(hue, 255, 255);
  }
  if (mode == TimerLedMode::Setting) {
    return CRGB::Blue;
  }
  if (mode == TimerLedMode::Paused) {
    return CRGB::Yellow;
  }
  return CRGB::Black;
}

static void setTimerMeterLevel256(uint16_t level256, TimerLedMode mode, bool forceShow = false) {
  if (g_meterLeds == nullptr || g_meterNumLeds <= 0) {
    return;
  }

  level256 = clampMeterLevel256((int)level256);

  if (!forceShow && (level256 == g_lastTimerMeterLevel256) && (mode == g_lastTimerLedMode)) {
    return;
  }

  const uint8_t fullRows = (uint8_t)(level256 / 256);
  const uint8_t partial = (uint8_t)(level256 % 256); // brightness (0..255) for the next row

  // Small grid (75 LEDs total): repaint all rows each time.
  for (uint8_t rowFromBottom = 0; rowFromBottom < METER_ROWS; rowFromBottom++) {
    uint8_t rowBrightness = 0;
    if (rowFromBottom < fullRows) {
      rowBrightness = 255;
    } else if (rowFromBottom == fullRows) {
      rowBrightness = partial;
    } else {
      rowBrightness = 0;
    }

    CRGB color = (rowBrightness > 0) ? meterColorForRowFromBottom(rowFromBottom, mode) : CRGB::Black;
    if (rowBrightness > 0 && rowBrightness < 255) {
      color.nscale8_video(rowBrightness);
    }

    // Convert bottom-based row to top-based row index.
    const uint8_t rowTop0 = (uint8_t)((METER_ROWS - 1) - rowFromBottom);

    for (uint8_t col = 0; col < METER_COLS; col++) {
      const int ledIdx = meterLedIndexForRowCol(rowTop0, col);
      if (ledIdx >= 0) {
        g_meterLeds[ledIdx] = color;
      }
    }
  }

  FastLED.show();
  g_lastTimerMeterLevel256 = level256;
  g_lastTimerLedMode = mode;
}

void timerLedMeterInit(CRGB* leds, int numLeds) {
  g_meterLeds = leds;
  g_meterNumLeds = numLeds;
  g_lastTimerLedUpdateMs = 0;
  g_lastTimerMeterLevel256 = 0;
  g_lastTimerLedMode = TimerLedMode::None;
  g_colorStyle = TimerLedColorStyle::White;
}

void timerLedMeterSetColorStyle(TimerLedColorStyle style) {
  g_colorStyle = style;
}

void timerLedMeterClear(bool forceShow) {
  if (g_meterLeds == nullptr || g_meterNumLeds <= 0) {
    return;
  }

  if (!forceShow && g_lastTimerLedMode == TimerLedMode::None && g_lastTimerMeterLevel256 == 0) {
    return;
  }

  for (int i = 0; i < g_meterNumLeds; i++) {
    g_meterLeds[i] = CRGB::Black;
  }
  FastLED.show();
  g_lastTimerMeterLevel256 = 0;
  g_lastTimerLedMode = TimerLedMode::None;
}

void timerLedMeterUpdateFromRemaining(unsigned long remainingMs, unsigned long totalMs, TimerLedMode mode) {
  if (g_meterLeds == nullptr || g_meterNumLeds <= 0) {
    return;
  }

  const uint32_t now = millis();
  // Rate-limit LED updates so we don't steal time from button reads.
  if ((now - g_lastTimerLedUpdateMs) < 50U) {
    return;
  }
  g_lastTimerLedUpdateMs = now;

  if (totalMs == 0UL) {
    timerLedMeterClear();
    return;
  }

  // Lit rows represent percent of time LEFT.
  // Fixed-point with 8 fractional bits gives us a smooth "in between" fade.
  // Use 64-bit math to avoid overflow.
  const uint64_t level256 = (uint64_t)remainingMs * (uint64_t)METER_ROWS * 256ULL / (uint64_t)totalMs;
  setTimerMeterLevel256((uint16_t)level256, mode);
}

void timerLedMeterUpdateFromMinutes(int minutesSelected) {
  if (g_meterLeds == nullptr || g_meterNumLeds <= 0) {
    return;
  }

  const uint32_t now = millis();
  if ((now - g_lastTimerLedUpdateMs) < 50U) {
    return;
  }
  g_lastTimerLedUpdateMs = now;

  if (minutesSelected < 0) minutesSelected = 0;
  if (minutesSelected > 60) minutesSelected = 60;

  const uint32_t level256 = (uint32_t)minutesSelected * (uint32_t)METER_ROWS * 256U / 60U;
  setTimerMeterLevel256((uint16_t)level256, TimerLedMode::Setting);
}
