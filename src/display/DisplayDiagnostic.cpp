#include "display/DisplayDiagnostic.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Log.h"
#include "display/DisplayConfig.h"

namespace {

TFT_eSPI s_tft;

void setBacklight(bool enabled) {
  if (DisplayConfig::BacklightPin < 0) {
    return;
  }

  pinMode(DisplayConfig::BacklightPin, OUTPUT);
  const bool active = enabled ? DisplayConfig::BacklightActiveHigh : !DisplayConfig::BacklightActiveHigh;
  digitalWrite(DisplayConfig::BacklightPin, active ? HIGH : LOW);
}

void drawCenteredLabel(int16_t x, int16_t y, const char* label, uint16_t color) {
  s_tft.setTextDatum(MC_DATUM);
  s_tft.setTextColor(color);
  s_tft.drawString(label, x, y, 2);
}

void drawCornerLabels() {
  const int16_t width = s_tft.width();
  const int16_t height = s_tft.height();

  s_tft.setTextFont(1);
  s_tft.setTextSize(1);
  s_tft.setTextColor(TFT_WHITE, TFT_BLACK);

  s_tft.setTextDatum(TL_DATUM);
  s_tft.drawString("TOP LEFT", 4, 4);

  s_tft.setTextDatum(TR_DATUM);
  s_tft.drawString("TOP RIGHT", width - 4, 4);

  s_tft.setTextDatum(BL_DATUM);
  s_tft.drawString("BOTTOM LEFT", 4, height - 4);

  s_tft.setTextDatum(BR_DATUM);
  s_tft.drawString("BOTTOM RIGHT", width - 4, height - 4);
}

void drawStatusPanel() {
  const int16_t width = s_tft.width();
  const int16_t height = s_tft.height();
  const int16_t panelWidth = min<int16_t>(width - 24, 260);
  const int16_t panelHeight = 96;
  const int16_t x = (width - panelWidth) / 2;
  const int16_t y = (height - panelHeight) / 2;

  s_tft.fillRoundRect(x, y, panelWidth, panelHeight, 4, TFT_BLACK);
  s_tft.drawRoundRect(x, y, panelWidth, panelHeight, 4, TFT_WHITE);

  s_tft.setTextDatum(TC_DATUM);
  s_tft.setTextColor(TFT_WHITE, TFT_BLACK);
  s_tft.drawString("CYD Display Diagnostic", width / 2, y + 8, 2);

  s_tft.setTextDatum(TL_DATUM);
  s_tft.setTextColor(TFT_CYAN, TFT_BLACK);
  s_tft.drawString(String("Driver: ") + DisplayConfig::driverName(), x + 10, y + 30, 1);
  s_tft.drawString(String("Rotation: ") + DisplayConfig::Rotation, x + 10, y + 43, 1);
  s_tft.drawString(String("Inversion: ") + DisplayConfig::inversionName(), x + 10, y + 56, 1);
  s_tft.drawString(String("RGB order: ") + DisplayConfig::rgbOrderName(), x + 10, y + 69, 1);
  s_tft.drawString(String("Backlight GPIO: ") + DisplayConfig::BacklightPin, x + 10, y + 82, 1);
}

}  // namespace

namespace DisplayDiagnostic {

void begin() {
  setBacklight(false);

  s_tft.init();
  s_tft.setRotation(DisplayConfig::Rotation);
  s_tft.invertDisplay(DisplayConfig::InvertColors);

  setBacklight(true);
  render();
  logConfig();
}

void render() {
  const int16_t width = s_tft.width();
  const int16_t height = s_tft.height();
  const int16_t barWidth = max<int16_t>(1, width / 8);

  struct ColorBar {
    uint16_t color;
    uint16_t textColor;
    const char* label;
  };

  const ColorBar bars[] = {
      {TFT_RED, TFT_WHITE, "RED"},
      {TFT_GREEN, TFT_BLACK, "GREEN"},
      {TFT_BLUE, TFT_WHITE, "BLUE"},
      {TFT_WHITE, TFT_BLACK, "WHITE"},
      {TFT_BLACK, TFT_WHITE, "BLACK"},
      {TFT_YELLOW, TFT_BLACK, "YELLOW"},
      {TFT_CYAN, TFT_BLACK, "CYAN"},
      {TFT_MAGENTA, TFT_WHITE, "MAGENTA"},
  };

  for (uint8_t i = 0; i < 8; ++i) {
    const int16_t x = i * barWidth;
    const int16_t w = (i == 7) ? width - x : barWidth;
    s_tft.fillRect(x, 0, w, height, bars[i].color);
    drawCenteredLabel(x + w / 2, height / 4, bars[i].label, bars[i].textColor);
  }

  drawCornerLabels();
  drawStatusPanel();
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

}  // namespace DisplayDiagnostic
