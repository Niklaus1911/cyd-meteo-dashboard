#include "ui/TouchInput.h"

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

#include "Log.h"
#include "display/TftDisplay.h"
#include "display/TouchConfig.h"

namespace {

constexpr uint8_t NoIrqPin = 255;

SPIClass s_touchSpi(TouchConfig::SpiBus);
XPT2046_Touchscreen s_touch(TouchConfig::CsPin,
                            TouchConfig::UseIrqPin ? TouchConfig::IrqPin : NoIrqPin);

lv_indev_drv_t s_inputDriver;
lv_indev_t* s_inputDevice = nullptr;
lv_obj_t* s_debugDot = nullptr;
lv_obj_t* s_debugLabel = nullptr;

uint16_t s_screenWidth = 320;
uint16_t s_screenHeight = 240;
lv_point_t s_lastPoint = {0, 0};
uint32_t s_lastDebugLogMs = 0;
bool s_initialized = false;

int16_t mapAxis(int32_t raw, int32_t rawMin, int32_t rawMax, uint16_t outputSize, bool invert) {
  if (outputSize == 0 || rawMin == rawMax) {
    return 0;
  }

  const int32_t outputMax = static_cast<int32_t>(outputSize) - 1;
  int32_t mapped = ((raw - rawMin) * outputMax) / (rawMax - rawMin);
  mapped = constrain(mapped, 0, outputMax);

  if (invert) {
    mapped = outputMax - mapped;
  }

  return static_cast<int16_t>(mapped);
}

lv_point_t mapTouchPoint(const TS_Point& rawPoint) {
  int32_t rawX = rawPoint.x;
  int32_t rawY = rawPoint.y;

  if (TouchConfig::SwapXY) {
    const int32_t swapped = rawX;
    rawX = rawY;
    rawY = swapped;
  }

  lv_point_t point;
  const int32_t maxX = static_cast<int32_t>(s_screenWidth) - 1;
  const int32_t maxY = static_cast<int32_t>(s_screenHeight) - 1;
  point.x = constrain(mapAxis(rawX,
                              TouchConfig::RawMinX,
                              TouchConfig::RawMaxX,
                              s_screenWidth,
                              TouchConfig::InvertX) +
                          TouchConfig::OffsetX,
                      0,
                      maxX);
  point.y = constrain(mapAxis(rawY,
                              TouchConfig::RawMinY,
                              TouchConfig::RawMaxY,
                              s_screenHeight,
                              TouchConfig::InvertY) +
                          TouchConfig::OffsetY,
                      0,
                      maxY);

  if (TftDisplay::isFlipped180()) {
    point.x = maxX - point.x;
    point.y = maxY - point.y;
  }

  return point;
}

bool readAveragedRawPoint(TS_Point& averagePoint) {
  int32_t xTotal = 0;
  int32_t yTotal = 0;
  int32_t zTotal = 0;
  uint8_t samples = 0;

  for (uint8_t i = 0; i < TouchConfig::SampleCount; ++i) {
    if (!s_touch.touched()) {
      continue;
    }

    const TS_Point sample = s_touch.getPoint();
    if (sample.z < TouchConfig::MinPressure) {
      continue;
    }

    xTotal += sample.x;
    yTotal += sample.y;
    zTotal += sample.z;
    ++samples;
  }

  if (samples == 0) {
    return false;
  }

  averagePoint.x = xTotal / samples;
  averagePoint.y = yTotal / samples;
  averagePoint.z = zTotal / samples;
  return true;
}

void updateDebugOverlay(const TS_Point& rawPoint, const lv_point_t& point, bool pressed) {
  if (!TouchConfig::ShowTouchDebugOverlay || s_debugDot == nullptr ||
      s_debugLabel == nullptr) {
    return;
  }

  if (!pressed) {
    lv_obj_add_flag(s_debugDot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_debugLabel, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_set_pos(s_debugDot, point.x - 3, point.y - 3);
  lv_obj_clear_flag(s_debugDot, LV_OBJ_FLAG_HIDDEN);

  char buffer[56] = {};
  snprintf(buffer,
           sizeof(buffer),
           "r:%d,%d m:%d,%d",
           rawPoint.x,
           rawPoint.y,
           point.x,
           point.y);
  lv_label_set_text(s_debugLabel, buffer);
  lv_obj_set_pos(s_debugLabel, 4, 222);
  lv_obj_clear_flag(s_debugLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_debugDot);
  lv_obj_move_foreground(s_debugLabel);
}

void logTouchPoint(const TS_Point& rawPoint, const lv_point_t& point) {
  if (!TouchConfig::DebugLogTouches) {
    return;
  }

  const uint32_t nowMs = millis();
  if (nowMs - s_lastDebugLogMs < TouchConfig::DebugLogPeriodMs) {
    return;
  }

  s_lastDebugLogMs = nowMs;
  LOG_TASK("touch raw=(%d,%d,%d) mapped=(%d,%d)",
           rawPoint.x,
           rawPoint.y,
           rawPoint.z,
           point.x,
           point.y);
}

void readTouch(lv_indev_drv_t*, lv_indev_data_t* data) {
  TS_Point rawPoint;
  if (!s_initialized || !s_touch.touched() || !readAveragedRawPoint(rawPoint)) {
    updateDebugOverlay(rawPoint, s_lastPoint, false);
    data->state = LV_INDEV_STATE_REL;
    data->point = s_lastPoint;
    return;
  }

  s_lastPoint = mapTouchPoint(rawPoint);

  data->state = LV_INDEV_STATE_PR;
  data->point = s_lastPoint;
  updateDebugOverlay(rawPoint, s_lastPoint, true);
  logTouchPoint(rawPoint, s_lastPoint);
}

}  // namespace

namespace TouchInput {

void begin(uint16_t screenWidth, uint16_t screenHeight) {
  if (!TouchConfig::Enabled || s_initialized) {
    return;
  }

  s_screenWidth = screenWidth;
  s_screenHeight = screenHeight;

  if (TouchConfig::UseIrqPin) {
    pinMode(TouchConfig::IrqPin, INPUT);
  }

  pinMode(TouchConfig::CsPin, OUTPUT);
  digitalWrite(TouchConfig::CsPin, HIGH);

  s_touchSpi.begin(TouchConfig::SclkPin,
                   TouchConfig::MisoPin,
                   TouchConfig::MosiPin,
                   TouchConfig::CsPin);

  if (!s_touch.begin(s_touchSpi)) {
    LOG_TASK("touch init failed cs=%d irq=%d spi=%u",
             TouchConfig::CsPin,
             TouchConfig::UseIrqPin ? TouchConfig::IrqPin : -1,
             TouchConfig::SpiBus);
    return;
  }

  lv_indev_drv_init(&s_inputDriver);
  s_inputDriver.type = LV_INDEV_TYPE_POINTER;
  s_inputDriver.read_cb = readTouch;
  s_inputDevice = lv_indev_drv_register(&s_inputDriver);

  if (TouchConfig::ShowTouchDebugOverlay) {
    s_debugDot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_debugDot, 7, 7);
    lv_obj_set_style_radius(s_debugDot, 4, 0);
    lv_obj_set_style_bg_color(s_debugDot, lv_color_hex(0xF4B740), 0);
    lv_obj_set_style_bg_opa(s_debugDot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_debugDot, 0, 0);
    lv_obj_add_flag(s_debugDot, LV_OBJ_FLAG_HIDDEN);

    s_debugLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(s_debugLabel, 180);
    lv_obj_set_style_text_font(s_debugLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_debugLabel, lv_color_hex(0xF4B740), 0);
    lv_obj_add_flag(s_debugLabel, LV_OBJ_FLAG_HIDDEN);
  }

  s_initialized = s_inputDevice != nullptr;
  LOG_TASK("touch initialized cs=%d irq=%d spi=%u pins sclk=%d miso=%d mosi=%d screen=%ux%u rotation=%u",
           TouchConfig::CsPin,
           TouchConfig::UseIrqPin ? TouchConfig::IrqPin : -1,
           TouchConfig::SpiBus,
           TouchConfig::SclkPin,
           TouchConfig::MisoPin,
           TouchConfig::MosiPin,
           s_screenWidth,
           s_screenHeight,
           TftDisplay::rotation());
}

}  // namespace TouchInput
