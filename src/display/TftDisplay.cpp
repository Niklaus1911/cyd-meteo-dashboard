#include "display/TftDisplay.h"

#include <Arduino.h>

#include "Log.h"
#include "app/UserSettings.h"
#include "display/DisplayConfig.h"

namespace {

TFT_eSPI s_tft;
bool s_initialized = false;
bool s_displayFlipped180 = false;
uint8_t s_rotation = DisplayConfig::NormalDisplayRotation;

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

  s_displayFlipped180 = UserSettingsStore::isDisplayFlipped180();
  s_rotation = s_displayFlipped180 ? DisplayConfig::FlippedDisplayRotation
                                   : DisplayConfig::NormalDisplayRotation;

  s_tft.init();
  s_tft.setRotation(s_rotation);
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
  LOG_TASK("display config driver=%s rotation=%u flipped180=%d inversion=%s rgb_order=%s backlight_pin=%d backlight_active_high=%d",
           DisplayConfig::driverName(),
           s_rotation,
           s_displayFlipped180,
           DisplayConfig::inversionName(),
           DisplayConfig::rgbOrderName(),
           DisplayConfig::BacklightPin,
           DisplayConfig::BacklightActiveHigh);
}

uint8_t rotation() {
  return s_rotation;
}

bool isFlipped180() {
  return s_displayFlipped180;
}

}  // namespace TftDisplay
