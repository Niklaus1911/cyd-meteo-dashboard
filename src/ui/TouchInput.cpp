#include "ui/TouchInput.h"

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

#include "Log.h"
#include "display/DisplayConfig.h"
#include "display/TouchConfig.h"

namespace {

constexpr uint8_t NoIrqPin = 255;

SPIClass s_touchSpi(TouchConfig::SpiBus);
XPT2046_Touchscreen s_touch(TouchConfig::CsPin,
                            TouchConfig::UseIrqPin ? TouchConfig::IrqPin : NoIrqPin);

lv_indev_drv_t s_inputDriver;
lv_indev_t* s_inputDevice = nullptr;

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
  point.x = mapAxis(rawX,
                    TouchConfig::RawMinX,
                    TouchConfig::RawMaxX,
                    s_screenWidth,
                    TouchConfig::InvertX);
  point.y = mapAxis(rawY,
                    TouchConfig::RawMinY,
                    TouchConfig::RawMaxY,
                    s_screenHeight,
                    TouchConfig::InvertY);
  return point;
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
  if (!s_initialized || !s_touch.touched()) {
    data->state = LV_INDEV_STATE_REL;
    data->point = s_lastPoint;
    return;
  }

  const TS_Point rawPoint = s_touch.getPoint();
  s_lastPoint = mapTouchPoint(rawPoint);

  data->state = LV_INDEV_STATE_PR;
  data->point = s_lastPoint;
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
           DisplayConfig::Rotation);
}

}  // namespace TouchInput
