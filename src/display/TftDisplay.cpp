#include "display/TftDisplay.h"

#include <Arduino.h>

#include "Log.h"
#include "display/DisplayConfig.h"

namespace {

TFT_eSPI s_tft;
bool s_initialized = false;

void setBacklight(bool enabled) {
  if (DisplayConfig::BacklightPin < 0) {
    return;
  }

  pinMode(DisplayConfig::BacklightPin, OUTPUT);
  const bool active = enabled ? DisplayConfig::BacklightActiveHigh : !DisplayConfig::BacklightActiveHigh;
  digitalWrite(DisplayConfig::BacklightPin, active ? HIGH : LOW);
}

}  // namespace

namespace TftDisplay {

void begin() {
  if (s_initialized) {
    return;
  }

  setBacklight(false);

  s_tft.init();
  s_tft.setRotation(DisplayConfig::Rotation);
  s_tft.invertDisplay(DisplayConfig::InvertColors);
  s_tft.fillScreen(TFT_BLACK);

  setBacklight(true);
  s_initialized = true;
  logConfig();
}

TFT_eSPI& tft() {
  return s_tft;
}

void logConfig() {
  LOG_TASK("display config driver=%s rotation=%u inversion=%s rgb_order=%s backlight_pin=%d backlight_active_high=%d",
           DisplayConfig::driverName(),
           DisplayConfig::Rotation,
           DisplayConfig::inversionName(),
           DisplayConfig::rgbOrderName(),
           DisplayConfig::BacklightPin,
           DisplayConfig::BacklightActiveHigh);
}

}  // namespace TftDisplay
